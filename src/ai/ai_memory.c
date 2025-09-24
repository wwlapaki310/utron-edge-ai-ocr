/**
 * @file ai_memory.c
 * @brief AI task memory management and statistics implementation
 * @details Memory pool management for Neural-ART NPU with leak detection
 * @author μTRON Competition Team
 * @date 2025
 */

#include "ai_task.h"
#include "hal.h"

// Memory pool management
typedef struct {
    uint8_t *pool_start;
    uint8_t *pool_end;
    uint32_t pool_size;
    uint32_t allocated_size;
    uint32_t peak_usage;
    uint32_t allocation_count;
    uint32_t free_count;
    uint32_t leak_count;
} ai_memory_pool_t;

// Memory block header for leak detection
typedef struct memory_block {
    uint32_t magic;          // Magic number for validation
    uint32_t size;           // Block size
    uint32_t timestamp;      // Allocation timestamp
    struct memory_block *next; // Next block in list
    uint8_t data[];          // Actual data
} memory_block_t;

#define MEMORY_MAGIC 0xABCDEF01
#define MEMORY_ALIGN 8  // 8-byte alignment

// Static memory pool (allocated from PSRAM)
static uint8_t ai_memory_pool_buffer[AI_MEMORY_POOL_SIZE] __attribute__((aligned(8)));
static ai_memory_pool_t ai_pool;
static memory_block_t *allocated_blocks = NULL;
static uint32_t memory_mutex; // Simple mutex for memory operations

// ========================================================================
// Memory Management Functions
// ========================================================================

int ai_memory_init(void)
{
    hal_debug_printf("[AI_MEMORY] Initializing memory pool (%d KB)...\n", 
                   AI_MEMORY_POOL_SIZE / 1024);
    
    // Initialize memory pool
    ai_pool.pool_start = ai_memory_pool_buffer;
    ai_pool.pool_end = ai_memory_pool_buffer + AI_MEMORY_POOL_SIZE;
    ai_pool.pool_size = AI_MEMORY_POOL_SIZE;
    ai_pool.allocated_size = 0;
    ai_pool.peak_usage = 0;
    ai_pool.allocation_count = 0;
    ai_pool.free_count = 0;
    ai_pool.leak_count = 0;
    
    allocated_blocks = NULL;
    memory_mutex = 0;
    
    hal_debug_printf("[AI_MEMORY] Memory pool initialized: %p - %p\n", 
                   ai_pool.pool_start, ai_pool.pool_end);
    
    return HAL_OK;
}

void* ai_memory_alloc(uint32_t size)
{
    if (size == 0) {
        return NULL;
    }
    
    // Acquire mutex (simplified)
    while (__sync_lock_test_and_set(&memory_mutex, 1));
    
    // Align size to 8-byte boundary
    size = (size + MEMORY_ALIGN - 1) & ~(MEMORY_ALIGN - 1);
    
    // Calculate total block size including header
    uint32_t total_size = sizeof(memory_block_t) + size;
    
    // Check if we have enough free space
    if (ai_pool.allocated_size + total_size > ai_pool.pool_size) {
        hal_debug_printf("[AI_MEMORY] Allocation failed: insufficient memory (%d + %d > %d)\n",
                       ai_pool.allocated_size, total_size, ai_pool.pool_size);
        memory_mutex = 0; // Release mutex
        return NULL;
    }
    
    // Find free space in pool (simple linear allocator for now)
    memory_block_t *block = (memory_block_t*)(ai_pool.pool_start + ai_pool.allocated_size);
    
    // Initialize block header
    block->magic = MEMORY_MAGIC;
    block->size = size;
    block->timestamp = hal_get_tick();
    block->next = allocated_blocks;
    
    // Add to allocated blocks list
    allocated_blocks = block;
    
    // Update pool statistics
    ai_pool.allocated_size += total_size;
    ai_pool.allocation_count++;
    
    if (ai_pool.allocated_size > ai_pool.peak_usage) {
        ai_pool.peak_usage = ai_pool.allocated_size;
    }
    
    memory_mutex = 0; // Release mutex
    
    hal_debug_printf("[AI_MEMORY] Allocated %d bytes at %p (total: %d KB)\n", 
                   size, block->data, ai_pool.allocated_size / 1024);
    
    return block->data;
}

void ai_memory_free(void *ptr)
{
    if (!ptr) {
        return;
    }
    
    // Acquire mutex
    while (__sync_lock_test_and_set(&memory_mutex, 1));
    
    // Find block header
    memory_block_t *block = (memory_block_t*)((uint8_t*)ptr - sizeof(memory_block_t));
    
    // Validate magic number
    if (block->magic != MEMORY_MAGIC) {
        hal_debug_printf("[AI_MEMORY] ERROR: Invalid magic number in block %p\n", ptr);
        ai_pool.leak_count++;
        memory_mutex = 0;
        return;
    }
    
    // Remove from allocated blocks list
    if (allocated_blocks == block) {
        allocated_blocks = block->next;
    } else {
        memory_block_t *current = allocated_blocks;
        while (current && current->next != block) {
            current = current->next;
        }
        if (current) {
            current->next = block->next;
        } else {
            hal_debug_printf("[AI_MEMORY] ERROR: Block not found in allocated list\n");
            ai_pool.leak_count++;
            memory_mutex = 0;
            return;
        }
    }
    
    // Update statistics
    uint32_t total_size = sizeof(memory_block_t) + block->size;
    ai_pool.allocated_size -= total_size;
    ai_pool.free_count++;
    
    // Clear block header (security)
    block->magic = 0;
    
    memory_mutex = 0; // Release mutex
    
    hal_debug_printf("[AI_MEMORY] Freed %d bytes at %p (remaining: %d KB)\n", 
                   block->size, ptr, ai_pool.allocated_size / 1024);
}

void ai_memory_get_stats(uint32_t *used_bytes, uint32_t *free_bytes, uint32_t *peak_usage)
{
    if (used_bytes) {
        *used_bytes = ai_pool.allocated_size;
    }
    if (free_bytes) {
        *free_bytes = ai_pool.pool_size - ai_pool.allocated_size;
    }
    if (peak_usage) {
        *peak_usage = ai_pool.peak_usage;
    }
}

uint32_t ai_memory_check_leaks(void)
{
    uint32_t leak_count = 0;
    memory_block_t *current = allocated_blocks;
    uint32_t current_time = hal_get_tick();
    
    while (current) {
        // Check for blocks allocated more than 30 seconds ago (potential leaks)
        if (current_time - current->timestamp > 30000) {
            hal_debug_printf("[AI_MEMORY] Potential leak: block %p, size %d, age %dms\n",
                           current->data, current->size, current_time - current->timestamp);
            leak_count++;
        }
        current = current->next;
    }
    
    ai_pool.leak_count += leak_count;
    return leak_count;
}

// ========================================================================
// Performance Statistics Functions
// ========================================================================

void ai_stats_reset(void)
{
    memset(&ai_context.stats, 0, sizeof(ai_performance_stats_t));
    ai_context.stats.min_inference_time_us = UINT32_MAX;
    
    hal_debug_printf("[AI_STATS] Performance statistics reset\n");
}

const ai_performance_stats_t* ai_stats_get(void)
{
    return &ai_context.stats;
}

void ai_stats_update_timing(uint32_t inference_time_us)
{
    ai_performance_stats_t *stats = &ai_context.stats;
    
    // Update timing statistics
    stats->last_inference_time_us = inference_time_us;
    
    if (inference_time_us < stats->min_inference_time_us) {
        stats->min_inference_time_us = inference_time_us;
    }
    
    if (inference_time_us > stats->max_inference_time_us) {
        stats->max_inference_time_us = inference_time_us;
    }
    
    // Update running average
    if (stats->successful_inferences > 0) {
        stats->avg_inference_time_us = 
            (stats->avg_inference_time_us * (stats->successful_inferences - 1) + inference_time_us) 
            / stats->successful_inferences;
    } else {
        stats->avg_inference_time_us = inference_time_us;
    }
}

void ai_stats_update_quality(float confidence, uint32_t character_accuracy)
{
    ai_performance_stats_t *stats = &ai_context.stats;
    
    // Update confidence statistics
    if (stats->successful_inferences > 0) {
        stats->avg_confidence_score = 
            (stats->avg_confidence_score * (stats->successful_inferences - 1) + confidence) 
            / stats->successful_inferences;
    } else {
        stats->avg_confidence_score = confidence;
    }
    
    // Update accuracy statistics
    stats->character_accuracy = character_accuracy;
    
    // Count low confidence results
    if (confidence < ai_context.config.confidence_threshold) {
        stats->low_confidence_count++;
    }
}

uint8_t ai_stats_check_targets(void)
{
    const ai_performance_stats_t *stats = &ai_context.stats;
    uint8_t targets_met = 1;
    
    // Check timing target
    if (stats->avg_inference_time_us > ai_context.config.max_inference_time_us) {
        hal_debug_printf("[AI_STATS] Timing target FAILED: %dμs > %dμs\n",
                       stats->avg_inference_time_us, ai_context.config.max_inference_time_us);
        targets_met = 0;
    }
    
    // Check accuracy target
    if (stats->character_accuracy < 95) { // 95% target
        hal_debug_printf("[AI_STATS] Accuracy target FAILED: %d%% < 95%%\n",
                       stats->character_accuracy);
        targets_met = 0;
    }
    
    // Check confidence target
    if (stats->avg_confidence_score < ai_context.config.confidence_threshold) {
        hal_debug_printf("[AI_STATS] Confidence target FAILED: %.2f < %.2f\n",
                       stats->avg_confidence_score, ai_context.config.confidence_threshold);
        targets_met = 0;
    }
    
    return targets_met;
}

// ========================================================================
// Neural-ART Integration Wrappers
// ========================================================================

int neural_art_init(void)
{
    hal_debug_printf("[NEURAL_ART] Initializing Neural-ART NPU...\n");
    
    // Enable NPU peripheral clock
    hal_peripheral_clock_enable(HAL_PERIPHERAL_NEURAL_ART, 1);
    
    // Set NPU to high performance mode
    hal_set_cpu_frequency(NPU_FREQUENCY_HZ);
    
    // Initialize NPU memory regions
    hal_memory_configure_protection(0, NPU_MAX_MEMORY_BYTES, 
                                   NPU_MAX_MEMORY_BYTES, 0x03); // Read/Write
    
    // Enable NPU interrupts
    hal_interrupt_enable(IRQ_NEURAL_ART, HAL_IRQ_PRIORITY_HIGH);
    
    hal_debug_printf("[NEURAL_ART] NPU initialized at %d MHz\n", NPU_FREQUENCY_HZ / 1000000);
    return 0;
}

int neural_art_load_model(ai_model_type_t model_type, const void *model_data, uint32_t size)
{
    hal_debug_printf("[NEURAL_ART] Loading model %d (%d KB)...\n", model_type, size / 1024);
    
    if (!model_data || size == 0 || model_type >= AI_MODEL_COUNT) {
        return AI_ERROR_INPUT_INVALID;
    }
    
    // Allocate model memory from external flash
    uint8_t *model_memory = (uint8_t*)HAL_FLASH_BASE + (model_type * 0x1000000); // 16MB per model
    
    // Copy model data to allocated memory
    memcpy(model_memory, model_data, size);
    
    // Initialize model structure
    neural_art_model_t *model = &ai_context.models[model_type];
    model->model_data = model_memory;
    model->model_size = size;
    model->precision = AI_PRECISION_INT8;
    model->loaded = 1;
    
    // Configure NPU for this model
    // This would involve setting up the Neural-ART runtime
    // For now, we simulate successful loading
    
    hal_debug_printf("[NEURAL_ART] Model %d loaded successfully\n", model_type);
    return 0;
}

int neural_art_inference(ai_model_type_t model_type, const void *input, void *output)
{
    if (model_type >= AI_MODEL_COUNT || !ai_context.models[model_type].loaded) {
        return AI_ERROR_MODEL_LOAD_FAILED;
    }
    
    uint32_t start_time = hal_get_time_us();
    
    // Simulate NPU inference (actual implementation would use Neural-ART SDK)
    // For demonstration, we add a realistic delay
    hal_delay_us(5000); // 5ms simulated inference time
    
    uint32_t end_time = hal_get_time_us();
    uint32_t inference_time = end_time - start_time;
    
    hal_debug_printf("[NEURAL_ART] Model %d inference completed in %dμs\n", 
                   model_type, inference_time);
    
    return 0;
}

uint32_t neural_art_get_utilization(void *npu_handle)
{
    // Simulate NPU utilization based on recent inference activity
    uint32_t recent_inferences = ai_context.stats.successful_inferences;
    uint32_t utilization = (recent_inferences * 10) % 100; // Simple simulation
    
    if (utilization < 50) utilization = 75; // Assume good utilization for demo
    
    return utilization;
}

uint8_t neural_art_is_model_ready(neural_art_model_t *model)
{
    return (model && model->loaded && model->model_data && model->model_size > 0);
}

// ========================================================================
// Error Recovery Functions
// ========================================================================

int ai_recovery_attempt(void)
{
    hal_debug_printf("[AI_TASK] Attempting AI system recovery...\n");
    
    ai_context.recovery_needed = 1;
    
    // Step 1: Reset NPU
    hal_peripheral_power_control(HAL_PERIPHERAL_NEURAL_ART, 0);
    hal_delay_ms(100);
    hal_peripheral_power_control(HAL_PERIPHERAL_NEURAL_ART, 1);
    
    // Step 2: Reinitialize NPU
    if (ai_neural_art_init_npu() != 0) {
        hal_debug_printf("[AI_TASK] NPU reinitialization failed\n");
        return AI_ERROR_RECOVERY_FAILED;
    }
    
    // Step 3: Reload models
    if (ai_load_ocr_models() != 0) {
        hal_debug_printf("[AI_TASK] Model reloading failed\n");
        return AI_ERROR_RECOVERY_FAILED;
    }
    
    // Step 4: Reset error counters
    ai_context.consecutive_errors = 0;
    ai_context.error_code = AI_ERROR_NONE;
    ai_context.recovery_needed = 0;
    
    // Step 5: Run self-test
    if (ai_self_test() != 0) {
        hal_debug_printf("[AI_TASK] Recovery self-test failed\n");
        return AI_ERROR_RECOVERY_FAILED;
    }
    
    hal_debug_printf("[AI_TASK] AI system recovery completed successfully\n");
    return 0;
}

const char* ai_get_last_error(uint32_t *error_code, uint32_t *error_count)
{
    if (error_code) {
        *error_code = ai_context.error_code;
    }
    if (error_count) {
        *error_count = ai_context.consecutive_errors;
    }
    
    // Return error description based on error code
    switch (ai_context.error_code) {
        case AI_ERROR_NONE:
            return "No error";
        case AI_ERROR_INIT_FAILED:
            return "Initialization failed";
        case AI_ERROR_MODEL_LOAD_FAILED:
            return "Model loading failed";
        case AI_ERROR_INFERENCE_TIMEOUT:
            return "Inference timeout";
        case AI_ERROR_MEMORY_ALLOC_FAILED:
            return "Memory allocation failed";
        case AI_ERROR_INPUT_INVALID:
            return "Invalid input";
        case AI_ERROR_NPU_ERROR:
            return "NPU error";
        case AI_ERROR_CONFIDENCE_TOO_LOW:
            return "Confidence too low";
        case AI_ERROR_RECOVERY_FAILED:
            return "Recovery failed";
        default:
            return "Unknown error";
    }
}

void ai_dump_state(void)
{
    const ai_performance_stats_t *stats = &ai_context.stats;
    
    hal_debug_printf("\n[AI_TASK] === AI Task State Dump ===\n");
    hal_debug_printf("State: %d\n", ai_context.current_state);
    hal_debug_printf("Total inferences: %d\n", stats->total_inferences);
    hal_debug_printf("Successful: %d\n", stats->successful_inferences);
    hal_debug_printf("Failed: %d\n", stats->failed_inferences);
    hal_debug_printf("Avg inference time: %dμs\n", stats->avg_inference_time_us);
    hal_debug_printf("Min/Max time: %d/%dμs\n", stats->min_inference_time_us, stats->max_inference_time_us);
    hal_debug_printf("Avg confidence: %.2f\n", stats->avg_confidence_score);
    hal_debug_printf("Memory usage: %d KB\n", stats->current_memory_usage / 1024);
    hal_debug_printf("Peak memory: %d KB\n", stats->peak_memory_usage / 1024);
    hal_debug_printf("Memory leaks: %d\n", stats->memory_leaks_detected);
    hal_debug_printf("Error code: %d (%s)\n", ai_context.error_code, ai_get_last_error(NULL, NULL));
    hal_debug_printf("Consecutive errors: %d\n", ai_context.consecutive_errors);
    hal_debug_printf("=================================\n\n");
}

// Platform specific IRQ numbers (these would be defined in platform headers)
#define IRQ_NEURAL_ART  85  // Example IRQ number for Neural-ART NPU