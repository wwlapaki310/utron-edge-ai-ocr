/**
 * @file ai_task.h
 * @brief AI inference task using Neural-ART NPU for OCR processing
 * @details Implements real-time text recognition with <10ms inference time using μTRON OS
 * @author μTRON Competition Team
 * @date 2025
 */

#ifndef AI_TASK_H
#define AI_TASK_H

#include "utron_config.h"
#include "camera_task.h"

// Neural-ART NPU configuration
#define NPU_FREQUENCY_HZ      1000000000U  // 1GHz
#define NPU_MAX_MEMORY_BYTES  2621440U     // 2.5MB activation memory
#define NPU_MODEL_MEMORY_MB   16U          // Model storage limit

// AI task timing constraints
#define AI_TASK_PERIOD_MS     20    // Match camera period
#define AI_INFERENCE_TIMEOUT_MS 10  // <10ms target
#define AI_TASK_PRIORITY      2     // High priority (after camera)

// OCR model configuration
#define OCR_INPUT_WIDTH       320   // Optimized for NPU
#define OCR_INPUT_HEIGHT      240   // Optimized for NPU
#define OCR_MAX_TEXT_LENGTH   256   // Maximum recognized text
#define OCR_MIN_CONFIDENCE    0.95f // >95% accuracy target

// Memory pool configuration  
#define AI_MEMORY_POOL_SIZE   NPU_MAX_MEMORY_BYTES
#define AI_SCRATCH_BUFFER_SIZE 512000  // 512KB scratch buffer
#define AI_RESULT_BUFFER_SIZE  1024    // OCR result buffer

// Neural-ART model types
typedef enum {
    AI_MODEL_TEXT_DETECTION = 0,    // EAST/CRAFT text detection
    AI_MODEL_TEXT_RECOGNITION,      // CRNN text recognition  
    AI_MODEL_PREPROCESSING,         // Image preprocessing
    AI_MODEL_COUNT
} ai_model_type_t;

// AI task states
typedef enum {
    AI_STATE_IDLE,
    AI_STATE_LOADING,
    AI_STATE_READY,
    AI_STATE_INFERENCING,
    AI_STATE_POSTPROCESSING,
    AI_STATE_ERROR
} ai_state_t;

// Inference precision modes
typedef enum {
    AI_PRECISION_INT8,     // Primary mode for NPU
    AI_PRECISION_INT16,    // Fallback mode
    AI_PRECISION_FLOAT32   // Debug mode only
} ai_precision_t;

// OCR result structure
typedef struct {
    char text[OCR_MAX_TEXT_LENGTH];  // Recognized text
    float confidence;                // Overall confidence score
    uint32_t char_count;            // Number of characters
    uint32_t word_count;            // Number of words
    uint32_t bbox_count;            // Number of bounding boxes
    uint32_t timestamp;             // Inference timestamp
    uint8_t language_detected;      // 0=Japanese, 1=English, 2=Mixed
} ocr_result_t;

// Bounding box structure for detected text
typedef struct {
    uint16_t x, y, width, height;   // Rectangle coordinates
    float confidence;               // Detection confidence
    uint8_t text_direction;         // 0=horizontal, 1=vertical
} text_bbox_t;

// Neural-ART model handle
typedef struct {
    void *model_data;               // Model binary data
    uint32_t model_size;            // Model size in bytes
    void *npu_handle;               // Neural-ART runtime handle
    ai_precision_t precision;       // Current precision mode
    uint32_t input_size;            // Input tensor size
    uint32_t output_size;           // Output tensor size
    uint8_t loaded;                 // Model loaded flag
} neural_art_model_t;

// AI task performance statistics
typedef struct {
    uint32_t total_inferences;      // Total inference count
    uint32_t successful_inferences; // Successful inferences
    uint32_t failed_inferences;     // Failed inferences
    
    // Timing statistics (microseconds)
    uint32_t min_inference_time_us;
    uint32_t max_inference_time_us; 
    uint32_t avg_inference_time_us;
    uint32_t last_inference_time_us;
    
    // Memory usage
    uint32_t current_memory_usage;
    uint32_t peak_memory_usage;
    uint32_t memory_leaks_detected;
    
    // Quality metrics
    float avg_confidence_score;
    uint32_t low_confidence_count;  // Below threshold
    uint32_t character_accuracy;    // Character-level accuracy
} ai_performance_stats_t;

// AI task configuration
typedef struct {
    ai_precision_t precision_mode;
    uint8_t enable_preprocessing;
    uint8_t enable_postprocessing;
    float confidence_threshold;
    uint32_t max_inference_time_us;
    uint8_t debug_enabled;
} ai_task_config_t;

// AI task context structure
typedef struct {
    // Task state
    ai_state_t current_state;
    ai_task_config_t config;
    
    // Neural-ART models
    neural_art_model_t models[AI_MODEL_COUNT];
    
    // Memory management
    uint8_t *memory_pool;
    uint32_t memory_pool_size;
    uint8_t *scratch_buffer;
    
    // Input/Output buffers
    uint8_t *input_buffer;          // Preprocessed image
    uint8_t *output_buffer;         // Raw inference output
    ocr_result_t *result_buffer;    // Processed OCR result
    text_bbox_t *bbox_buffer;       // Detected text boxes
    
    // Performance monitoring
    ai_performance_stats_t stats;
    uint32_t last_frame_timestamp;
    
    // Error handling
    uint32_t error_code;
    uint32_t consecutive_errors;
    uint8_t recovery_needed;
} ai_task_context_t;

// Global variables
extern ai_task_context_t ai_context;
extern ai_state_t ai_current_state;

// ========================================================================
// Core AI Task Functions
// ========================================================================

/**
 * @brief Create AI inference task
 * @details Creates μTRON OS task with high priority for real-time OCR
 */
void create_ai_task(void);

/**
 * @brief AI task entry point
 * @param arg Task argument (unused) 
 * @details Main AI inference loop with timing guarantees
 */
void ai_task_entry(void *arg);

/**
 * @brief Initialize AI subsystem
 * @return 0 on success, negative on error
 * @details Initialize Neural-ART NPU and load OCR models
 */
int ai_init(void);

/**
 * @brief Configure AI task parameters
 * @param config AI task configuration
 * @return 0 on success, negative on error
 */
int ai_configure(const ai_task_config_t *config);

/**
 * @brief Shutdown AI subsystem
 * @return 0 on success, negative on error
 * @details Clean shutdown and resource deallocation
 */
int ai_shutdown(void);

// ========================================================================
// Neural-ART NPU Management
// ========================================================================

/**
 * @brief Initialize Neural-ART NPU
 * @return 0 on success, negative on error
 * @details Configure NPU clock, memory, and runtime
 */
int neural_art_init(void);

/**
 * @brief Load OCR model to NPU
 * @param model_type Type of model to load
 * @param model_data Binary model data
 * @param size Model size in bytes
 * @return 0 on success, negative on error
 */
int neural_art_load_model(ai_model_type_t model_type, const void *model_data, uint32_t size);

/**
 * @brief Unload model from NPU
 * @param model_type Type of model to unload
 * @return 0 on success, negative on error
 */
int neural_art_unload_model(ai_model_type_t model_type);

/**
 * @brief Execute NPU inference
 * @param model_type Model to use for inference
 * @param input Input tensor data
 * @param output Output tensor data
 * @return 0 on success, negative on error
 * @details Blocking inference call with timeout
 */
int neural_art_inference(ai_model_type_t model_type, const void *input, void *output);

/**
 * @brief Get NPU status
 * @return Current NPU utilization percentage
 */
uint32_t neural_art_get_utilization(void);

// ========================================================================
// OCR Processing Pipeline
// ========================================================================

/**
 * @brief Process frame for OCR
 * @param frame Input frame from camera
 * @param result OCR result output
 * @return 0 on success, negative on error
 * @details Complete OCR pipeline with <10ms guarantee
 */
int ocr_process_frame(const frame_buffer_t *frame, ocr_result_t *result);

/**
 * @brief Preprocess image for OCR
 * @param input_frame Raw camera frame
 * @param output_buffer Preprocessed image buffer
 * @return 0 on success, negative on error
 * @details Resize, normalize, and format for NPU
 */
int ocr_preprocess_image(const frame_buffer_t *input_frame, uint8_t *output_buffer);

/**
 * @brief Detect text regions in image
 * @param image Preprocessed image
 * @param bboxes Detected text bounding boxes
 * @param max_boxes Maximum number of boxes
 * @return Number of detected boxes, negative on error
 */
int ocr_detect_text(const uint8_t *image, text_bbox_t *bboxes, uint32_t max_boxes);

/**
 * @brief Recognize text in bounding box
 * @param image Source image
 * @param bbox Text bounding box
 * @param text_output Recognized text output
 * @param confidence Confidence score output
 * @return 0 on success, negative on error
 */
int ocr_recognize_text(const uint8_t *image, const text_bbox_t *bbox, 
                      char *text_output, float *confidence);

/**
 * @brief Post-process OCR results
 * @param raw_result Raw OCR output
 * @param processed_result Processed result
 * @return 0 on success, negative on error
 * @details Apply language detection, spell check, formatting
 */
int ocr_postprocess_result(const void *raw_result, ocr_result_t *processed_result);

// ========================================================================
// Memory Management
// ========================================================================

/**
 * @brief Initialize AI memory pools
 * @return 0 on success, negative on error
 * @details Setup memory pools for NPU and processing
 */
int ai_memory_init(void);

/**
 * @brief Allocate memory from AI pool
 * @param size Size in bytes
 * @return Pointer to allocated memory, NULL on failure
 */
void* ai_memory_alloc(uint32_t size);

/**
 * @brief Free memory to AI pool
 * @param ptr Pointer to free
 */
void ai_memory_free(void *ptr);

/**
 * @brief Get memory usage statistics
 * @param used_bytes Currently used bytes
 * @param free_bytes Available bytes
 * @param peak_usage Peak usage since reset
 */
void ai_memory_get_stats(uint32_t *used_bytes, uint32_t *free_bytes, uint32_t *peak_usage);

/**
 * @brief Check for memory leaks
 * @return Number of leaked blocks
 */
uint32_t ai_memory_check_leaks(void);

// ========================================================================
// Performance Monitoring
// ========================================================================

/**
 * @brief Reset performance statistics
 */
void ai_stats_reset(void);

/**
 * @brief Get current performance statistics
 * @return Pointer to statistics structure
 */
const ai_performance_stats_t* ai_stats_get(void);

/**
 * @brief Update inference timing
 * @param inference_time_us Inference time in microseconds
 */
void ai_stats_update_timing(uint32_t inference_time_us);

/**
 * @brief Update quality metrics
 * @param confidence Confidence score
 * @param character_accuracy Character accuracy percentage
 */
void ai_stats_update_quality(float confidence, uint32_t character_accuracy);

/**
 * @brief Check if performance targets are met
 * @return 1 if targets met, 0 otherwise
 */
uint8_t ai_stats_check_targets(void);

// ========================================================================
// Error Handling and Recovery
// ========================================================================

/**
 * @brief AI error handler
 * @param error_code Error code
 * @details Handle AI errors and attempt recovery
 */
void ai_error_handler(uint32_t error_code);

/**
 * @brief Attempt AI system recovery
 * @return 0 on success, negative on failure
 */
int ai_recovery_attempt(void);

/**
 * @brief Get last error information
 * @param error_code Output error code
 * @param error_count Output consecutive error count
 * @return Error description string
 */
const char* ai_get_last_error(uint32_t *error_code, uint32_t *error_count);

// ========================================================================
// Debug and Testing
// ========================================================================

/**
 * @brief AI self-test
 * @return 0 on success, negative on error
 * @details Comprehensive AI subsystem test
 */
int ai_self_test(void);

/**
 * @brief Enable/disable AI debug output
 * @param enable 1 to enable, 0 to disable
 */
void ai_set_debug(uint8_t enable);

/**
 * @brief Dump AI task state for debugging
 */
void ai_dump_state(void);

/**
 * @brief Run performance benchmark
 * @param iterations Number of test iterations
 * @return Average inference time in microseconds
 */
uint32_t ai_benchmark(uint32_t iterations);

// ========================================================================
// Integration Interface
// ========================================================================

/**
 * @brief Get current AI task state
 * @return Current AI state
 */
ai_state_t ai_get_state(void);

/**
 * @brief Request AI processing
 * @param priority Request priority (0=highest)
 * @return 0 on success, negative on error
 * @note Non-blocking call for integration with other tasks
 */
int ai_request_processing(uint8_t priority);

/**
 * @brief Check if OCR result is ready
 * @return 1 if result ready, 0 otherwise
 */
uint8_t ai_is_result_ready(void);

/**
 * @brief Get latest OCR result
 * @param result Output result buffer
 * @return 0 on success, negative on error
 * @details Non-blocking result retrieval for audio task
 */
int ai_get_result(ocr_result_t *result);

// Error codes
#define AI_ERROR_NONE                 0
#define AI_ERROR_INIT_FAILED         -1
#define AI_ERROR_MODEL_LOAD_FAILED   -2
#define AI_ERROR_INFERENCE_TIMEOUT   -3
#define AI_ERROR_MEMORY_ALLOC_FAILED -4
#define AI_ERROR_INPUT_INVALID       -5
#define AI_ERROR_NPU_ERROR           -6
#define AI_ERROR_CONFIDENCE_TOO_LOW  -7
#define AI_ERROR_RECOVERY_FAILED     -8

#endif // AI_TASK_H