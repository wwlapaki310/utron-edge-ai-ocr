/**
 * @file audio_task.h
 * @brief Real-time audio synthesis task for text-to-speech output
 * @details Implements streaming TTS with <5ms latency using μTRON OS
 * @author μTRON Competition Team
 * @date 2025
 */

#ifndef AUDIO_TASK_H
#define AUDIO_TASK_H

#include "utron_config.h"
#include "ai_task.h"

// Audio hardware configuration
#define AUDIO_SAMPLE_RATE       16000   // 16kHz for voice synthesis
#define AUDIO_BITS_PER_SAMPLE   16      // 16-bit audio
#define AUDIO_CHANNELS          1       // Mono output
#define AUDIO_FRAME_SIZE_MS     10      // 10ms frames for low latency

// Audio buffer configuration
#define AUDIO_BUFFER_SIZE       512000  // 512KB ring buffer
#define AUDIO_DMA_BUFFER_SIZE   1024    // DMA buffer size
#define AUDIO_FRAME_SAMPLES     (AUDIO_SAMPLE_RATE * AUDIO_FRAME_SIZE_MS / 1000)

// Audio task timing
#define AUDIO_TASK_PERIOD_MS    5       // 5ms period for streaming
#define AUDIO_TASK_PRIORITY     3       // Medium-high priority
#define AUDIO_SYNTHESIS_TIMEOUT_MS 50   // TTS synthesis timeout

// TTS configuration
#define TTS_MAX_INPUT_LENGTH    OCR_MAX_TEXT_LENGTH  // Match OCR output
#define TTS_MAX_PHONEMES        1024    // Maximum phoneme buffer
#define TTS_SPEED_MIN           1       // Minimum speech speed (0.5x)
#define TTS_SPEED_MAX           10      // Maximum speech speed (2.0x)  
#define TTS_SPEED_DEFAULT       5       // Default speed (1.0x)

// Language support
#define LANG_JAPANESE           0
#define LANG_ENGLISH            1
#define LANG_AUTO_DETECT        2
#define LANG_MIXED              3

// Audio task states
typedef enum {
    AUDIO_STATE_IDLE,
    AUDIO_STATE_SYNTHESIZING,
    AUDIO_STATE_PLAYING,
    AUDIO_STATE_BUFFERING,
    AUDIO_STATE_ERROR,
    AUDIO_STATE_MUTED
} audio_state_t;

// TTS engine types
typedef enum {
    TTS_ENGINE_SIMPLE,      // Basic concatenative synthesis
    TTS_ENGINE_NEURAL,      // Neural TTS (if resources allow)
    TTS_ENGINE_FORMANT      // Formant synthesis for fallback
} tts_engine_type_t;

// Audio format structure
typedef struct {
    uint32_t sample_rate;
    uint16_t bit_depth;
    uint8_t channels;
    uint32_t frame_size;
} audio_format_t;

// Voice configuration
typedef struct {
    uint8_t language;           // Language mode (JP/EN/AUTO)
    uint8_t speech_speed;       // Speed 1-10 (5=normal)
    uint8_t volume_level;       // Volume 0-100
    uint8_t pitch_shift;        // Pitch adjustment -10 to +10
    tts_engine_type_t engine;   // TTS engine selection
    uint8_t enable_prosody;     // Enable prosodic features
} voice_config_t;

// Audio buffer management
typedef struct {
    uint8_t *data;              // Audio data buffer
    uint32_t size;              // Buffer size
    volatile uint32_t write_pos; // Write position
    volatile uint32_t read_pos;  // Read position
    volatile uint32_t available; // Available data size
    uint8_t overflow;           // Buffer overflow flag
} audio_ring_buffer_t;

// TTS request structure
typedef struct {
    char text[TTS_MAX_INPUT_LENGTH];  // Text to synthesize
    uint8_t language;           // Language hint
    uint8_t priority;           // Request priority
    uint32_t timestamp;         // Request timestamp
    float confidence;           // OCR confidence (affects processing)
    uint8_t emergency;          // Emergency announcement flag
} tts_request_t;

// Audio synthesis statistics
typedef struct {
    uint32_t total_requests;        // Total TTS requests
    uint32_t successful_synthesis;  // Successful synthesis
    uint32_t failed_synthesis;      // Failed synthesis
    uint32_t buffer_underruns;      // Audio underruns
    uint32_t buffer_overruns;       // Audio overruns
    
    // Timing statistics (microseconds)
    uint32_t min_synthesis_time_us;
    uint32_t max_synthesis_time_us;
    uint32_t avg_synthesis_time_us;
    uint32_t last_synthesis_time_us;
    
    // Quality metrics
    uint32_t characters_synthesized;
    uint32_t words_synthesized;
    uint32_t language_switches;     // JP/EN switches
    
    // Hardware statistics
    uint32_t dma_interrupts;
    uint32_t i2s_errors;
    uint32_t codec_resets;
} audio_performance_stats_t;

// Audio task configuration
typedef struct {
    audio_format_t format;
    voice_config_t voice;
    uint32_t buffer_size;
    uint8_t enable_echo_cancel;
    uint8_t enable_noise_gate;
    uint8_t debug_enabled;
} audio_task_config_t;

// Audio task context
typedef struct {
    // Task state
    audio_state_t current_state;
    audio_task_config_t config;
    
    // Audio hardware
    void *i2s_handle;           // I2S/SAI interface handle
    void *dma_handle;           // DMA handle
    void *codec_handle;         // Audio codec handle
    
    // TTS engine
    tts_engine_type_t current_engine;
    void *tts_context;          // TTS engine context
    uint8_t *phoneme_buffer;    // Phoneme processing buffer
    
    // Buffer management
    audio_ring_buffer_t ring_buffer;
    uint8_t *dma_buffers[2];    // Double buffering for DMA
    volatile uint8_t active_dma_buffer;
    
    // Request queue
    tts_request_t request_queue[8];  // Request queue
    volatile uint8_t queue_head;
    volatile uint8_t queue_tail;
    volatile uint8_t queue_count;
    
    // Performance monitoring
    audio_performance_stats_t stats;
    uint32_t last_request_time;
    
    // Error handling
    uint32_t error_code;
    uint32_t consecutive_errors;
    uint8_t mute_on_error;
} audio_task_context_t;

// Global variables
extern audio_task_context_t audio_context;
extern audio_state_t audio_current_state;

// ========================================================================
// Core Audio Task Functions
// ========================================================================

/**
 * @brief Create audio synthesis task
 * @details Creates μTRON OS task for real-time TTS
 */
void create_audio_task(void);

/**
 * @brief Audio task entry point
 * @param arg Task argument (unused)
 * @details Main audio processing loop with streaming
 */
void audio_task_entry(void *arg);

/**
 * @brief Initialize audio subsystem
 * @return 0 on success, negative on error
 * @details Initialize I2S, DMA, codec, and TTS engine
 */
int audio_init(void);

/**
 * @brief Configure audio system
 * @param config Audio configuration
 * @return 0 on success, negative on error
 */
int audio_configure(const audio_task_config_t *config);

/**
 * @brief Shutdown audio subsystem
 * @return 0 on success, negative on error
 */
int audio_shutdown(void);

// ========================================================================
// Text-to-Speech Functions
// ========================================================================

/**
 * @brief Initialize TTS engine
 * @param engine_type TTS engine to initialize
 * @return 0 on success, negative on error
 */
int tts_init(tts_engine_type_t engine_type);

/**
 * @brief Synthesize text to speech
 * @param text Input text
 * @param language Language hint (0=JP, 1=EN, 2=AUTO)
 * @param audio_buffer Output audio buffer
 * @param buffer_size Buffer size
 * @return Generated audio size, negative on error
 * @details Blocking synthesis with timeout protection
 */
int tts_synthesize(const char *text, uint8_t language, 
                  uint8_t *audio_buffer, uint32_t buffer_size);

/**
 * @brief Start streaming synthesis
 * @param text Input text
 * @param language Language hint
 * @return 0 on success, negative on error
 * @details Non-blocking streaming synthesis
 */
int tts_synthesize_streaming(const char *text, uint8_t language);

/**
 * @brief Detect text language
 * @param text Input text
 * @return Language code (0=JP, 1=EN, 3=Mixed)
 */
uint8_t tts_detect_language(const char *text);

/**
 * @brief Convert text to phonemes
 * @param text Input text
 * @param language Language code
 * @param phonemes Output phoneme buffer
 * @param max_phonemes Maximum phonemes
 * @return Number of phonemes, negative on error
 */
int tts_text_to_phonemes(const char *text, uint8_t language, 
                        char *phonemes, uint32_t max_phonemes);

/**
 * @brief Generate speech from phonemes
 * @param phonemes Input phonemes
 * @param audio_buffer Output buffer
 * @param buffer_size Buffer size
 * @return Generated size, negative on error
 */
int tts_phonemes_to_audio(const char *phonemes, 
                         uint8_t *audio_buffer, uint32_t buffer_size);

// ========================================================================
// Audio Hardware Interface
// ========================================================================

/**
 * @brief Initialize audio hardware
 * @return 0 on success, negative on error
 * @details Setup I2S/SAI, DMA, and codec
 */
int audio_hw_init(void);

/**
 * @brief Start audio playback
 * @return 0 on success, negative on error
 */
int audio_hw_start_playback(void);

/**
 * @brief Stop audio playback
 * @return 0 on success, negative on error
 */
int audio_hw_stop_playback(void);

/**
 * @brief Set output volume
 * @param volume Volume level 0-100
 * @return 0 on success, negative on error
 */
int audio_hw_set_volume(uint8_t volume);

/**
 * @brief Mute/unmute audio output
 * @param mute 1 to mute, 0 to unmute
 * @return 0 on success, negative on error
 */
int audio_hw_set_mute(uint8_t mute);

/**
 * @brief Audio DMA completion callback
 * @param buffer_index Completed buffer index
 * @details Called from DMA interrupt
 */
void audio_dma_complete_callback(uint8_t buffer_index);

/**
 * @brief Audio hardware error callback
 * @param error_code Hardware error code
 */
void audio_hw_error_callback(uint32_t error_code);

// ========================================================================
// Audio Buffer Management
// ========================================================================

/**
 * @brief Initialize audio ring buffer
 * @param buffer Ring buffer structure
 * @param data Data buffer
 * @param size Buffer size
 * @return 0 on success, negative on error
 */
int audio_buffer_init(audio_ring_buffer_t *buffer, uint8_t *data, uint32_t size);

/**
 * @brief Write data to ring buffer
 * @param buffer Ring buffer
 * @param data Input data
 * @param size Data size
 * @return Bytes written, negative on error
 */
int audio_buffer_write(audio_ring_buffer_t *buffer, const uint8_t *data, uint32_t size);

/**
 * @brief Read data from ring buffer
 * @param buffer Ring buffer
 * @param data Output data
 * @param size Requested size
 * @return Bytes read, negative on error
 */
int audio_buffer_read(audio_ring_buffer_t *buffer, uint8_t *data, uint32_t size);

/**
 * @brief Get available data in buffer
 * @param buffer Ring buffer
 * @return Available bytes
 */
uint32_t audio_buffer_available(const audio_ring_buffer_t *buffer);

/**
 * @brief Get free space in buffer
 * @param buffer Ring buffer
 * @return Free bytes
 */
uint32_t audio_buffer_free(const audio_ring_buffer_t *buffer);

/**
 * @brief Clear ring buffer
 * @param buffer Ring buffer to clear
 */
void audio_buffer_clear(audio_ring_buffer_t *buffer);

// ========================================================================
// Request Queue Management
// ========================================================================

/**
 * @brief Queue TTS request
 * @param text Text to synthesize
 * @param language Language hint
 * @param priority Request priority (0=highest)
 * @return 0 on success, negative on error
 */
int audio_queue_tts_request(const char *text, uint8_t language, uint8_t priority);

/**
 * @brief Queue OCR result for TTS
 * @param ocr_result OCR result structure
 * @return 0 on success, negative on error
 * @details Convenience function for AI task integration
 */
int audio_queue_ocr_result(const ocr_result_t *ocr_result);

/**
 * @brief Get next TTS request
 * @param request Output request structure
 * @return 0 if request available, negative if queue empty
 */
int audio_dequeue_request(tts_request_t *request);

/**
 * @brief Clear request queue
 */
void audio_clear_request_queue(void);

/**
 * @brief Get queue status
 * @param count Output queue count
 * @param capacity Output queue capacity
 * @return 1 if queue full, 0 otherwise
 */
uint8_t audio_get_queue_status(uint8_t *count, uint8_t *capacity);

// ========================================================================
// Voice Configuration
// ========================================================================

/**
 * @brief Set voice parameters
 * @param config Voice configuration
 * @return 0 on success, negative on error
 */
int audio_set_voice_config(const voice_config_t *config);

/**
 * @brief Get current voice configuration
 * @param config Output configuration
 * @return 0 on success, negative on error
 */
int audio_get_voice_config(voice_config_t *config);

/**
 * @brief Set speech speed
 * @param speed Speed level 1-10 (5=normal)
 * @return 0 on success, negative on error
 */
int audio_set_speech_speed(uint8_t speed);

/**
 * @brief Set language mode
 * @param language Language mode (0=JP, 1=EN, 2=AUTO)
 * @return 0 on success, negative on error
 */
int audio_set_language_mode(uint8_t language);

/**
 * @brief Set TTS engine
 * @param engine TTS engine type
 * @return 0 on success, negative on error
 */
int audio_set_tts_engine(tts_engine_type_t engine);

// ========================================================================
// Performance Monitoring
// ========================================================================

/**
 * @brief Reset audio statistics
 */
void audio_stats_reset(void);

/**
 * @brief Get performance statistics
 * @return Pointer to statistics structure
 */
const audio_performance_stats_t* audio_stats_get(void);

/**
 * @brief Update synthesis timing
 * @param synthesis_time_us Synthesis time in microseconds
 */
void audio_stats_update_timing(uint32_t synthesis_time_us);

/**
 * @brief Update buffer statistics
 * @param underrun Buffer underrun occurred
 * @param overrun Buffer overrun occurred
 */
void audio_stats_update_buffer(uint8_t underrun, uint8_t overrun);

/**
 * @brief Check performance targets
 * @return 1 if targets met, 0 otherwise
 */
uint8_t audio_stats_check_targets(void);

// ========================================================================
// Error Handling
// ========================================================================

/**
 * @brief Audio error handler
 * @param error_code Error code
 */
void audio_error_handler(uint32_t error_code);

/**
 * @brief Attempt audio recovery
 * @return 0 on success, negative on failure
 */
int audio_recovery_attempt(void);

/**
 * @brief Get last error information
 * @param error_code Output error code
 * @param error_count Output consecutive errors
 * @return Error description string
 */
const char* audio_get_last_error(uint32_t *error_code, uint32_t *error_count);

// ========================================================================
// Integration Interface
// ========================================================================

/**
 * @brief Get current audio state
 * @return Current audio state
 */
audio_state_t audio_get_state(void);

/**
 * @brief Check if audio is ready for new request
 * @return 1 if ready, 0 if busy
 */
uint8_t audio_is_ready(void);

/**
 * @brief Emergency stop audio
 * @details Immediately stop all audio processing
 */
void audio_emergency_stop(void);

/**
 * @brief Emergency announcement
 * @param message Emergency message
 * @param interrupt_current 1 to interrupt current playback
 * @return 0 on success, negative on error
 */
int audio_emergency_announce(const char *message, uint8_t interrupt_current);

// ========================================================================
// Debug and Testing
// ========================================================================

/**
 * @brief Audio self-test
 * @return 0 on success, negative on error
 */
int audio_self_test(void);

/**
 * @brief Play test tone
 * @param frequency Tone frequency in Hz
 * @param duration_ms Duration in milliseconds
 * @return 0 on success, negative on error
 */
int audio_play_test_tone(uint32_t frequency, uint32_t duration_ms);

/**
 * @brief Enable/disable audio debug
 * @param enable 1 to enable, 0 to disable
 */
void audio_set_debug(uint8_t enable);

/**
 * @brief Dump audio task state
 */
void audio_dump_state(void);

/**
 * @brief TTS benchmark test
 * @param test_text Text for benchmarking
 * @param iterations Number of iterations
 * @return Average synthesis time in microseconds
 */
uint32_t audio_benchmark_tts(const char *test_text, uint32_t iterations);

// Error codes
#define AUDIO_ERROR_NONE                0
#define AUDIO_ERROR_INIT_FAILED        -1
#define AUDIO_ERROR_HARDWARE_FAULT     -2
#define AUDIO_ERROR_TTS_FAILED         -3
#define AUDIO_ERROR_BUFFER_OVERFLOW    -4
#define AUDIO_ERROR_BUFFER_UNDERRUN    -5
#define AUDIO_ERROR_QUEUE_FULL         -6
#define AUDIO_ERROR_INVALID_LANGUAGE   -7
#define AUDIO_ERROR_SYNTHESIS_TIMEOUT  -8
#define AUDIO_ERROR_CODEC_ERROR        -9
#define AUDIO_ERROR_DMA_ERROR          -10

#endif // AUDIO_TASK_H