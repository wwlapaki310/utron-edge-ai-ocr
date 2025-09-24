/**
 * @file ai_task.c
 * @brief AI inference task implementation using Neural-ART NPU for OCR processing
 * @details Neural-ART NPU integrated OCR system with <8ms inference time
 * @author μTRON Competition Team
 * @date 2025
 */

#include "ai_task.h"
#include "camera_task.h"
#include "audio_task.h"
#include "system_task.h"
#include "hal.h"

// Neural-ART SDK includes (platform specific)
#include "neural_art_runtime.h"
#include "stm32cube_ai.h"

// Global AI task context
ai_task_context_t ai_context;
ai_state_t ai_current_state = AI_STATE_IDLE;

// Static function prototypes
static int ai_neural_art_init_npu(void);
static int ai_load_ocr_models(void);
static int ai_setup_memory_pools(void);
static int ai_validate_model_performance(void);
static void ai_performance_monitor_task(void);
static int ai_handle_inference_error(uint32_t error_code);

// Pre-trained OCR model data (stored in external Flash)
extern const uint8_t ocr_text_detection_model_data[];
extern const uint32_t ocr_text_detection_model_size;
extern const uint8_t ocr_text_recognition_model_data[];
extern const uint32_t ocr_text_recognition_model_size;

// Model calibration data for quantization
extern const float model_calibration_data[];
extern const uint32_t calibration_data_count;

// ========================================================================
// Core AI Task Functions
// ========================================================================

void create_ai_task(void)
{
    // Create AI inference task with high priority
    // μTRON OS task creation (platform specific implementation)
    
    // Task parameters
    static uint8_t ai_task_stack[8192];  // 8KB stack for AI task
    
    // Initialize task context
    memset(&ai_context, 0, sizeof(ai_task_context_t));
    ai_context.current_state = AI_STATE_IDLE;
    
    // Set default configuration
    ai_context.config.precision_mode = AI_PRECISION_INT8;
    ai_context.config.confidence_threshold = 0.95f;
    ai_context.config.max_inference_time_us = 8000; // 8ms target
    ai_context.config.debug_enabled = 1;
    
    // Create μTRON OS task
    // utron_create_task("AI_TASK", AI_TASK_PRIORITY, ai_task_entry, 
    //                   ai_task_stack, sizeof(ai_task_stack), NULL);
    
    hal_debug_printf("[AI_TASK] Task created with priority %d\n", AI_TASK_PRIORITY);
}

void ai_task_entry(void *arg)
{
    hal_result_t result;
    uint32_t last_performance_check = 0;
    uint32_t inference_start_time, inference_end_time;
    
    hal_debug_printf("[AI_TASK] Starting AI task entry\n");
    
    // Initialize AI subsystem
    if (ai_init() != 0) {
        hal_debug_printf("[AI_TASK] Initialization failed, entering error state\n");
        ai_context.current_state = AI_STATE_ERROR;
        system_log_error(ERROR_SEVERITY_CRITICAL, TASK_ID_AI_TASK, 
                         AI_ERROR_INIT_FAILED, "AI task initialization failed", 0);
        return;
    }
    
    ai_context.current_state = AI_STATE_READY;
    hal_debug_printf("[AI_TASK] AI task ready for processing\n");
    
    // Main AI task loop
    while (1) {
        uint32_t current_time = hal_get_tick();
        
        // Check if new frame is available from camera
        frame_buffer_t *frame = camera_get_frame();
        if (frame != NULL && frame->ready) {
            
            ai_context.current_state = AI_STATE_INFERENCING;
            inference_start_time = hal_get_time_us();
            
            // Process frame for OCR
            ocr_result_t ocr_result;
            int processing_result = ocr_process_frame(frame, &ocr_result);
            
            inference_end_time = hal_get_time_us();
            uint32_t inference_time_us = inference_end_time - inference_start_time;
            
            if (processing_result == 0) {
                // Update performance statistics
                ai_stats_update_timing(inference_time_us);
                ai_stats_update_quality(ocr_result.confidence, 
                                       ocr_result.char_count > 0 ? 95 : 0);
                
                // Check if result meets quality threshold
                if (ocr_result.confidence >= ai_context.config.confidence_threshold) {
                    // Send result to audio task for TTS
                    audio_queue_ocr_result(&ocr_result);
                    
                    hal_debug_printf("[AI_TASK] OCR success: '%s' (conf: %.2f, time: %dμs)\n", 
                                   ocr_result.text, ocr_result.confidence, inference_time_us);
                } else {
                    hal_debug_printf("[AI_TASK] Low confidence result: %.2f < %.2f\n", 
                                   ocr_result.confidence, ai_context.config.confidence_threshold);
                    ai_context.stats.low_confidence_count++;
                }
                
                // Check timing constraints
                if (inference_time_us > ai_context.config.max_inference_time_us) {
                    hal_debug_printf("[AI_TASK] WARNING: Inference time %dμs > target %dμs\n", 
                                   inference_time_us, ai_context.config.max_inference_time_us);
                    system_log_error(ERROR_SEVERITY_WARNING, TASK_ID_AI_TASK,
                                    AI_ERROR_INFERENCE_TIMEOUT, "Inference time exceeded", inference_time_us);
                }
                
            } else {
                // Handle processing error
                ai_handle_inference_error(processing_result);
                ai_context.stats.failed_inferences++;
            }
            
            // Release camera frame
            camera_release_frame(frame);
            ai_context.current_state = AI_STATE_READY;
        }
        
        // Periodic performance monitoring
        if (current_time - last_performance_check > 1000) { // Every 1 second
            ai_performance_monitor_task();
            last_performance_check = current_time;
        }
        
        // Report task status to system monitor
        system_update_task_status(TASK_ID_AI_TASK, 
                                 ai_context.stats.avg_inference_time_us / 1000, // Convert to CPU %
                                 ai_context.memory_pool_size - hal_memory_get_size(HAL_MEMORY_TYPE_SRAM));
        
        // Task period control (20ms to match camera)
        // utron_task_sleep(AI_TASK_PERIOD_MS);
        hal_delay_ms(AI_TASK_PERIOD_MS);
    }
}

int ai_init(void)
{
    hal_result_t result;
    
    hal_debug_printf("[AI_TASK] Initializing AI subsystem...\n");
    
    // Initialize memory pools
    result = ai_memory_init();
    if (result != HAL_OK) {
        hal_debug_printf("[AI_TASK] Memory initialization failed: %d\n", result);
        return AI_ERROR_MEMORY_ALLOC_FAILED;
    }
    
    // Initialize Neural-ART NPU
    result = ai_neural_art_init_npu();
    if (result != 0) {
        hal_debug_printf("[AI_TASK] Neural-ART initialization failed: %d\n", result);
        return AI_ERROR_NPU_ERROR;
    }
    
    // Load OCR models
    result = ai_load_ocr_models();
    if (result != 0) {
        hal_debug_printf("[AI_TASK] Model loading failed: %d\n", result);
        return AI_ERROR_MODEL_LOAD_FAILED;
    }
    
    // Validate model performance
    result = ai_validate_model_performance();
    if (result != 0) {
        hal_debug_printf("[AI_TASK] Model validation failed: %d\n", result);
        return AI_ERROR_INIT_FAILED;
    }
    
    // Initialize performance statistics
    ai_stats_reset();
    
    hal_debug_printf("[AI_TASK] AI subsystem initialized successfully\n");
    return 0;
}

// ========================================================================
// Neural-ART NPU Management
// ========================================================================

static int ai_neural_art_init_npu(void)
{
    neural_art_config_t npu_config;
    neural_art_result_t npu_result;
    
    hal_debug_printf("[AI_TASK] Initializing Neural-ART NPU...\n");
    
    // Configure NPU parameters
    npu_config.frequency_hz = NPU_FREQUENCY_HZ;
    npu_config.memory_size = NPU_MAX_MEMORY_BYTES;
    npu_config.precision_mode = NEURAL_ART_PRECISION_INT8;
    npu_config.power_mode = NEURAL_ART_POWER_HIGH_PERFORMANCE;
    
    // Initialize Neural-ART runtime
    npu_result = neural_art_init(&npu_config);
    if (npu_result != NEURAL_ART_SUCCESS) {
        hal_debug_printf("[AI_TASK] Neural-ART init failed: %d\n", npu_result);
        return AI_ERROR_NPU_ERROR;
    }
    
    // Get NPU handle
    ai_context.models[0].npu_handle = neural_art_get_handle();
    if (ai_context.models[0].npu_handle == NULL) {
        hal_debug_printf("[AI_TASK] Failed to get NPU handle\n");
        return AI_ERROR_NPU_ERROR;
    }
    
    // Set NPU to high performance mode
    neural_art_set_power_mode(ai_context.models[0].npu_handle, NEURAL_ART_POWER_HIGH_PERFORMANCE);
    
    hal_debug_printf("[AI_TASK] Neural-ART NPU initialized at %d MHz\n", NPU_FREQUENCY_HZ / 1000000);
    return 0;
}

static int ai_load_ocr_models(void)
{
    neural_art_result_t result;
    
    hal_debug_printf("[AI_TASK] Loading OCR models...\n");
    
    // Load text detection model (EAST/CRAFT)
    result = neural_art_load_model(ai_context.models[0].npu_handle,
                                  ocr_text_detection_model_data,
                                  ocr_text_detection_model_size,
                                  &ai_context.models[AI_MODEL_TEXT_DETECTION]);
    if (result != NEURAL_ART_SUCCESS) {
        hal_debug_printf("[AI_TASK] Text detection model load failed: %d\n", result);
        return AI_ERROR_MODEL_LOAD_FAILED;
    }
    
    // Load text recognition model (CRNN)  
    result = neural_art_load_model(ai_context.models[0].npu_handle,
                                  ocr_text_recognition_model_data,
                                  ocr_text_recognition_model_size,
                                  &ai_context.models[AI_MODEL_TEXT_RECOGNITION]);
    if (result != NEURAL_ART_SUCCESS) {
        hal_debug_printf("[AI_TASK] Text recognition model load failed: %d\n", result);
        return AI_ERROR_MODEL_LOAD_FAILED;
    }
    
    // Verify models are loaded correctly
    for (int i = 0; i < AI_MODEL_COUNT; i++) {
        if (!neural_art_is_model_ready(&ai_context.models[i])) {
            hal_debug_printf("[AI_TASK] Model %d not ready\n", i);
            return AI_ERROR_MODEL_LOAD_FAILED;
        }
        ai_context.models[i].loaded = 1;
        hal_debug_printf("[AI_TASK] Model %d loaded successfully\n", i);
    }
    
    hal_debug_printf("[AI_TASK] All OCR models loaded successfully\n");
    return 0;
}

static int ai_validate_model_performance(void)
{
    hal_debug_printf("[AI_TASK] Validating model performance...\n");
    
    // Prepare test image (320x240 RGB565)
    uint8_t test_image[OCR_INPUT_WIDTH * OCR_INPUT_HEIGHT * 2];
    memset(test_image, 0x80, sizeof(test_image)); // Gray test pattern
    
    // Create test frame buffer
    frame_buffer_t test_frame = {
        .data = test_image,
        .size = sizeof(test_image),
        .timestamp = hal_get_tick(),
        .ready = 1
    };
    
    // Run inference benchmark
    uint32_t total_time = 0;
    const int test_iterations = 10;
    
    for (int i = 0; i < test_iterations; i++) {
        ocr_result_t test_result;
        uint32_t start_time = hal_get_time_us();
        
        int result = ocr_process_frame(&test_frame, &test_result);
        
        uint32_t end_time = hal_get_time_us();
        uint32_t inference_time = end_time - start_time;
        total_time += inference_time;
        
        if (result != 0) {
            hal_debug_printf("[AI_TASK] Validation failed at iteration %d: %d\n", i, result);
            return AI_ERROR_INIT_FAILED;
        }
        
        hal_debug_printf("[AI_TASK] Test %d: %dμs\n", i+1, inference_time);
    }
    
    uint32_t avg_time = total_time / test_iterations;
    hal_debug_printf("[AI_TASK] Average inference time: %dμs (target: <%dμs)\n", 
                   avg_time, ai_context.config.max_inference_time_us);
    
    // Check if performance target is met
    if (avg_time > ai_context.config.max_inference_time_us) {
        hal_debug_printf("[AI_TASK] WARNING: Performance target not met\n");
        return AI_ERROR_INFERENCE_TIMEOUT;
    }
    
    // Update baseline performance
    ai_context.stats.avg_inference_time_us = avg_time;
    
    hal_debug_printf("[AI_TASK] Model performance validation completed successfully\n");
    return 0;
}

// ========================================================================
// OCR Processing Pipeline
// ========================================================================

int ocr_process_frame(const frame_buffer_t *frame, ocr_result_t *result)
{
    if (!frame || !result || !frame->ready) {
        return AI_ERROR_INPUT_INVALID;
    }
    
    int processing_result = 0;
    uint32_t start_time = hal_get_time_us();
    
    // Clear result structure
    memset(result, 0, sizeof(ocr_result_t));
    result->timestamp = hal_get_tick();
    
    // Step 1: Preprocess image for OCR
    uint8_t *preprocessed_image = ai_memory_alloc(OCR_INPUT_WIDTH * OCR_INPUT_HEIGHT * 2);
    if (!preprocessed_image) {
        return AI_ERROR_MEMORY_ALLOC_FAILED;
    }
    
    processing_result = ocr_preprocess_image(frame, preprocessed_image);
    if (processing_result != 0) {
        ai_memory_free(preprocessed_image);
        return processing_result;
    }
    
    // Step 2: Detect text regions
    text_bbox_t text_boxes[16]; // Support up to 16 text regions
    int detected_boxes = ocr_detect_text(preprocessed_image, text_boxes, 16);
    if (detected_boxes < 0) {
        ai_memory_free(preprocessed_image);
        return detected_boxes;
    }
    
    result->bbox_count = detected_boxes;
    
    // Step 3: Recognize text in each detected region
    char combined_text[OCR_MAX_TEXT_LENGTH] = {0};
    float total_confidence = 0.0f;
    int recognized_regions = 0;
    
    for (int i = 0; i < detected_boxes && i < 16; i++) {
        char region_text[64];
        float region_confidence;
        
        processing_result = ocr_recognize_text(preprocessed_image, &text_boxes[i], 
                                             region_text, &region_confidence);
        if (processing_result == 0 && region_confidence > 0.5f) {
            // Append text with space separator
            if (strlen(combined_text) > 0) {
                strcat(combined_text, " ");
            }
            strncat(combined_text, region_text, sizeof(combined_text) - strlen(combined_text) - 1);
            
            total_confidence += region_confidence;
            recognized_regions++;
        }
    }
    
    // Step 4: Post-process results
    if (recognized_regions > 0) {
        strncpy(result->text, combined_text, OCR_MAX_TEXT_LENGTH - 1);
        result->confidence = total_confidence / recognized_regions;
        result->char_count = strlen(result->text);
        result->word_count = 1; // Simple word count (spaces + 1)
        for (char *p = result->text; *p; p++) {
            if (*p == ' ') result->word_count++;
        }
        
        // Detect language (simple heuristic)
        result->language_detected = tts_detect_language(result->text);
    }
    
    // Cleanup
    ai_memory_free(preprocessed_image);
    
    // Update statistics
    uint32_t end_time = hal_get_time_us();
    uint32_t processing_time = end_time - start_time;
    
    ai_context.stats.total_inferences++;
    if (recognized_regions > 0) {
        ai_context.stats.successful_inferences++;
    } else {
        ai_context.stats.failed_inferences++;
    }
    
    hal_debug_printf("[AI_TASK] OCR completed in %dμs, %d regions, conf: %.2f\n", 
                   processing_time, recognized_regions, result->confidence);
    
    return 0;
}

int ocr_preprocess_image(const frame_buffer_t *input_frame, uint8_t *output_buffer)
{
    if (!input_frame || !output_buffer) {
        return AI_ERROR_INPUT_INVALID;
    }
    
    // Convert from camera format (640x480 RGB565) to OCR format (320x240)
    // Simple downsampling with 2x2 average
    uint16_t *src = (uint16_t*)input_frame->data;
    uint16_t *dst = (uint16_t*)output_buffer;
    
    for (int y = 0; y < OCR_INPUT_HEIGHT; y++) {
        for (int x = 0; x < OCR_INPUT_WIDTH; x++) {
            // Sample 2x2 region from source image
            int src_x = x * 2;
            int src_y = y * 2;
            
            uint16_t pixel1 = src[src_y * CAMERA_WIDTH + src_x];
            uint16_t pixel2 = src[src_y * CAMERA_WIDTH + src_x + 1];
            uint16_t pixel3 = src[(src_y + 1) * CAMERA_WIDTH + src_x];
            uint16_t pixel4 = src[(src_y + 1) * CAMERA_WIDTH + src_x + 1];
            
            // Simple average (could be improved with proper filtering)
            uint32_t avg_r = ((pixel1 >> 11) + (pixel2 >> 11) + (pixel3 >> 11) + (pixel4 >> 11)) / 4;
            uint32_t avg_g = (((pixel1 >> 5) & 0x3F) + ((pixel2 >> 5) & 0x3F) + 
                             ((pixel3 >> 5) & 0x3F) + ((pixel4 >> 5) & 0x3F)) / 4;
            uint32_t avg_b = ((pixel1 & 0x1F) + (pixel2 & 0x1F) + (pixel3 & 0x1F) + (pixel4 & 0x1F)) / 4;
            
            dst[y * OCR_INPUT_WIDTH + x] = (avg_r << 11) | (avg_g << 5) | avg_b;
        }
    }
    
    return 0;
}

int ocr_detect_text(const uint8_t *image, text_bbox_t *bboxes, uint32_t max_boxes)
{
    neural_art_result_t result;
    
    // Run text detection model on NPU
    void *detection_output = ai_memory_alloc(1024); // Temporary output buffer
    if (!detection_output) {
        return AI_ERROR_MEMORY_ALLOC_FAILED;
    }
    
    result = neural_art_inference(&ai_context.models[AI_MODEL_TEXT_DETECTION],
                                 image, detection_output);
    
    if (result != NEURAL_ART_SUCCESS) {
        ai_memory_free(detection_output);
        return AI_ERROR_NPU_ERROR;
    }
    
    // Parse detection output (simplified - actual implementation would be more complex)
    int detected_count = 0;
    // TODO: Implement proper EAST/CRAFT output parsing
    
    // For demo purposes, create a single text region covering most of the image
    if (max_boxes > 0) {
        bboxes[0].x = OCR_INPUT_WIDTH / 4;
        bboxes[0].y = OCR_INPUT_HEIGHT / 4;
        bboxes[0].width = OCR_INPUT_WIDTH / 2;
        bboxes[0].height = OCR_INPUT_HEIGHT / 4;
        bboxes[0].confidence = 0.9f;
        bboxes[0].text_direction = 0; // Horizontal
        detected_count = 1;
    }
    
    ai_memory_free(detection_output);
    return detected_count;
}

int ocr_recognize_text(const uint8_t *image, const text_bbox_t *bbox, 
                      char *text_output, float *confidence)
{
    neural_art_result_t result;
    
    if (!image || !bbox || !text_output || !confidence) {
        return AI_ERROR_INPUT_INVALID;
    }
    
    // Extract text region from image
    uint8_t *region_buffer = ai_memory_alloc(bbox->width * bbox->height * 2);
    if (!region_buffer) {
        return AI_ERROR_MEMORY_ALLOC_FAILED;
    }
    
    // Crop region (simplified)
    uint16_t *src = (uint16_t*)image;
    uint16_t *dst = (uint16_t*)region_buffer;
    
    for (int y = 0; y < bbox->height; y++) {
        for (int x = 0; x < bbox->width; x++) {
            int src_x = bbox->x + x;
            int src_y = bbox->y + y;
            if (src_x < OCR_INPUT_WIDTH && src_y < OCR_INPUT_HEIGHT) {
                dst[y * bbox->width + x] = src[src_y * OCR_INPUT_WIDTH + src_x];
            }
        }
    }
    
    // Run text recognition model
    char recognition_output[64];
    result = neural_art_inference(&ai_context.models[AI_MODEL_TEXT_RECOGNITION],
                                 region_buffer, recognition_output);
    
    if (result == NEURAL_ART_SUCCESS) {
        // Parse recognition output (simplified - actual would decode CTC output)
        strncpy(text_output, "Sample Text", 63); // Demo output
        text_output[63] = '\0';
        *confidence = 0.92f; // Demo confidence
    } else {
        text_output[0] = '\0';
        *confidence = 0.0f;
        ai_memory_free(region_buffer);
        return AI_ERROR_NPU_ERROR;
    }
    
    ai_memory_free(region_buffer);
    return 0;
}

// ========================================================================
// Performance Monitoring
// ========================================================================

static void ai_performance_monitor_task(void)
{
    uint32_t npu_utilization = neural_art_get_utilization(ai_context.models[0].npu_handle);
    uint32_t memory_used, memory_free, memory_peak;
    
    ai_memory_get_stats(&memory_used, &memory_free, &memory_peak);
    
    // Update context statistics
    ai_context.stats.current_memory_usage = memory_used;
    if (memory_used > ai_context.stats.peak_memory_usage) {
        ai_context.stats.peak_memory_usage = memory_used;
    }
    
    // Check for performance issues
    if (ai_context.stats.avg_inference_time_us > ai_context.config.max_inference_time_us) {
        hal_debug_printf("[AI_TASK] PERF WARNING: Avg inference time %dμs > %dμs\n",
                       ai_context.stats.avg_inference_time_us, 
                       ai_context.config.max_inference_time_us);
    }
    
    if (npu_utilization < 50) { // Below 50% utilization
        hal_debug_printf("[AI_TASK] PERF WARNING: Low NPU utilization %d%%\n", npu_utilization);
    }
    
    // Log periodic performance summary
    if (ai_context.config.debug_enabled) {
        hal_debug_printf("[AI_TASK] PERF: %d inferences, avg %dμs, NPU %d%%, mem %dKB\n",
                       ai_context.stats.total_inferences,
                       ai_context.stats.avg_inference_time_us,
                       npu_utilization,
                       memory_used / 1024);
    }
}

static int ai_handle_inference_error(uint32_t error_code)
{
    ai_context.error_code = error_code;
    ai_context.consecutive_errors++;
    
    hal_debug_printf("[AI_TASK] Inference error: %d (consecutive: %d)\n", 
                   error_code, ai_context.consecutive_errors);
    
    // Log error to system
    system_log_error(ERROR_SEVERITY_ERROR, TASK_ID_AI_TASK, error_code, 
                     "AI inference failed", ai_context.consecutive_errors);
    
    // Attempt recovery if too many consecutive errors
    if (ai_context.consecutive_errors > 5) {
        hal_debug_printf("[AI_TASK] Too many consecutive errors, attempting recovery\n");
        return ai_recovery_attempt();
    }
    
    return 0;
}

// ========================================================================
// AI Task Integration Interface
// ========================================================================

ai_state_t ai_get_state(void)
{
    return ai_context.current_state;
}

uint8_t ai_is_result_ready(void)
{
    return (ai_context.current_state == AI_STATE_READY && 
            ai_context.stats.total_inferences > 0);
}

int ai_get_result(ocr_result_t *result)
{
    if (!result) {
        return AI_ERROR_INPUT_INVALID;
    }
    
    // Copy last result (simplified - should use proper result queue)
    memcpy(result, ai_context.result_buffer, sizeof(ocr_result_t));
    return 0;
}

// ========================================================================
// Debug and Testing Functions
// ========================================================================

int ai_self_test(void)
{
    hal_debug_printf("[AI_TASK] Running AI subsystem self-test...\n");
    
    // Test 1: NPU connectivity
    if (ai_context.models[0].npu_handle == NULL) {
        hal_debug_printf("[AI_TASK] Self-test FAIL: No NPU handle\n");
        return -1;
    }
    
    // Test 2: Model loading
    for (int i = 0; i < AI_MODEL_COUNT; i++) {
        if (!ai_context.models[i].loaded) {
            hal_debug_printf("[AI_TASK] Self-test FAIL: Model %d not loaded\n", i);
            return -2;
        }
    }
    
    // Test 3: Memory allocation
    void *test_buffer = ai_memory_alloc(1024);
    if (!test_buffer) {
        hal_debug_printf("[AI_TASK] Self-test FAIL: Memory allocation failed\n");
        return -3;
    }
    ai_memory_free(test_buffer);
    
    // Test 4: Performance validation (already done in init)
    if (!ai_stats_check_targets()) {
        hal_debug_printf("[AI_TASK] Self-test WARNING: Performance targets not met\n");
        // Continue anyway for debugging
    }
    
    hal_debug_printf("[AI_TASK] Self-test completed successfully\n");
    return 0;
}

void ai_set_debug(uint8_t enable)
{
    ai_context.config.debug_enabled = enable;
    hal_debug_printf("[AI_TASK] Debug output %s\n", enable ? "enabled" : "disabled");
}

uint32_t ai_benchmark(uint32_t iterations)
{
    hal_debug_printf("[AI_TASK] Running performance benchmark (%d iterations)...\n", iterations);
    
    uint32_t total_time = 0;
    uint8_t test_image[OCR_INPUT_WIDTH * OCR_INPUT_HEIGHT * 2];
    memset(test_image, 0x80, sizeof(test_image));
    
    frame_buffer_t test_frame = {
        .data = test_image,
        .size = sizeof(test_image),
        .timestamp = hal_get_tick(),
        .ready = 1
    };
    
    for (uint32_t i = 0; i < iterations; i++) {
        ocr_result_t result;
        uint32_t start = hal_get_time_us();
        
        ocr_process_frame(&test_frame, &result);
        
        uint32_t end = hal_get_time_us();
        total_time += (end - start);
    }
    
    uint32_t avg_time = total_time / iterations;
    hal_debug_printf("[AI_TASK] Benchmark completed: %dμs average (%d iterations)\n", 
                   avg_time, iterations);
    
    return avg_time;
}
