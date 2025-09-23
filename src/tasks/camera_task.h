/**
 * @file camera_task.h
 * @brief Camera capture task using MIPI CSI-2 interface
 * @details Implements real-time image capture with 20ms period using μTRON OS
 * @author μTRON Competition Team
 * @date 2025
 */

#ifndef CAMERA_TASK_H
#define CAMERA_TASK_H

#include "utron_config.h"

// Camera configuration
#define CAMERA_WIDTH  640
#define CAMERA_HEIGHT 480
#define CAMERA_FORMAT IMAGE_FORMAT_RGB565
#define CAMERA_FRAME_SIZE (CAMERA_WIDTH * CAMERA_HEIGHT * 2)

// Frame buffer management
#define FRAME_BUFFER_COUNT 2  // Double buffering
#define FRAME_BUFFER_SIZE  CAMERA_FRAME_SIZE

// Camera task timing
#define CAMERA_TASK_PERIOD_MS 20  // 50 FPS
#define CAMERA_TASK_TIMEOUT_MS 100

// Camera states
typedef enum {
    CAMERA_STATE_IDLE,
    CAMERA_STATE_CAPTURING,
    CAMERA_STATE_PROCESSING,
    CAMERA_STATE_ERROR
} camera_state_t;

// Camera configuration structure
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t format;
    uint32_t frame_rate;
    uint8_t auto_exposure;
    uint8_t auto_white_balance;
} camera_config_t;

// Frame buffer structure
typedef struct {
    uint8_t *data;
    uint32_t size;
    uint32_t timestamp;
    uint8_t ready;
} frame_buffer_t;

// Global variables
extern frame_buffer_t frame_buffers[FRAME_BUFFER_COUNT];
extern uint8_t current_frame_index;
extern camera_state_t camera_state;

/**
 * @brief Create camera capture task
 * @details Creates μTRON OS task with high priority for real-time capture
 */
void create_camera_task(void);

/**
 * @brief Camera task entry point
 * @param arg Task argument (unused)
 * @details Main camera task loop with 20ms period
 */
void camera_task_entry(void *arg);

/**
 * @brief Initialize camera hardware
 * @return 0 on success, negative on error
 * @details Configure MIPI CSI-2, ISP, and DMA
 */
int camera_init(void);

/**
 * @brief Configure camera parameters
 * @param config Camera configuration structure
 * @return 0 on success, negative on error
 */
int camera_configure(const camera_config_t *config);

/**
 * @brief Start continuous capture
 * @return 0 on success, negative on error
 * @details Begin DMA-based frame capture
 */
int camera_start_capture(void);

/**
 * @brief Stop capture
 * @return 0 on success, negative on error
 */
int camera_stop_capture(void);

/**
 * @brief Capture single frame
 * @param buffer Destination buffer
 * @return 0 on success, negative on error
 * @details Blocking capture for single frame
 */
int camera_capture_frame(uint8_t *buffer);

/**
 * @brief Get next available frame
 * @return Pointer to frame buffer, NULL if none available
 * @details Non-blocking frame retrieval for AI task
 */
frame_buffer_t* camera_get_frame(void);

/**
 * @brief Release frame buffer
 * @param frame Frame buffer to release
 * @details Mark frame buffer as available for reuse
 */
void camera_release_frame(frame_buffer_t *frame);

/**
 * @brief Camera interrupt handler
 * @details Called on DMA transfer complete
 */
void camera_dma_isr_handler(void);

/**
 * @brief Camera error handler
 * @details Handle camera errors and recovery
 */
void camera_error_handler(void);

/**
 * @brief Get camera status
 * @return Current camera state
 */
camera_state_t camera_get_state(void);

/**
 * @brief Camera self-test
 * @return 0 on success, negative on error
 * @details Test camera functionality
 */
int camera_self_test(void);

// ISP (Image Signal Processor) functions

/**
 * @brief Initialize ISP
 * @return 0 on success, negative on error
 */
int isp_init(void);

/**
 * @brief Configure ISP parameters
 * @param exposure_time Exposure time in microseconds
 * @param gain Analog gain value
 * @param white_balance Color temperature
 * @return 0 on success, negative on error
 */
int isp_configure(uint32_t exposure_time, uint32_t gain, uint32_t white_balance);

/**
 * @brief Enable/disable auto exposure
 * @param enable 1 to enable, 0 to disable
 */
void isp_set_auto_exposure(uint8_t enable);

/**
 * @brief Enable/disable auto white balance
 * @param enable 1 to enable, 0 to disable
 */
void isp_set_auto_white_balance(uint8_t enable);

// Debug and monitoring functions

/**
 * @brief Get camera statistics
 * @param fps Current frames per second
 * @param dropped_frames Number of dropped frames
 * @param error_count Error count
 */
void camera_get_stats(uint32_t *fps, uint32_t *dropped_frames, uint32_t *error_count);

/**
 * @brief Reset camera statistics
 */
void camera_reset_stats(void);

/**
 * @brief Camera debug output
 * @param enable 1 to enable debug, 0 to disable
 */
void camera_set_debug(uint8_t enable);

#endif // CAMERA_TASK_H