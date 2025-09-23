/**
 * @file utron_config.h
 * @brief μTRON OS configuration for Edge AI OCR project
 */

#ifndef UTRON_CONFIG_H
#define UTRON_CONFIG_H

#include "stm32n6xx_hal.h"
#include "utron.h"

// Task Priority Definitions
#define TASK_PRIORITY_EMERGENCY  1  // Highest priority
#define TASK_PRIORITY_CAMERA     2  // High priority
#define TASK_PRIORITY_AI         3  // Medium priority  
#define TASK_PRIORITY_OUTPUT     4  // Medium-low priority
#define TASK_PRIORITY_SYSTEM     5  // Lowest priority

// Task Stack Sizes
#define CAMERA_TASK_STACK_SIZE   4096
#define AI_TASK_STACK_SIZE       8192  
#define AUDIO_TASK_STACK_SIZE    2048
#define SOLENOID_TASK_STACK_SIZE 1024
#define SYSTEM_TASK_STACK_SIZE   2048

// Memory Allocation
#define IMAGE_BUFFER_SIZE        (640 * 480 * 2)  // VGA RGB565
#define AI_ACTIVATION_SIZE       (2500 * 1024)    // 2.5MB
#define AUDIO_BUFFER_SIZE        (512 * 1024)     // 512KB
#define OCR_RESULT_MAX_LENGTH    256

// System Configuration
#define SYSTEM_TICK_FREQ         1000  // 1ms tick
#define CAMERA_CAPTURE_FREQ      50    // 20ms period
#define SYSTEM_MONITOR_FREQ      10    // 100ms period

// GPIO Pin Definitions
#define SOLENOID_1_PIN           GPIO_PIN_0
#define SOLENOID_1_PORT          GPIOA
#define SOLENOID_2_PIN           GPIO_PIN_1  
#define SOLENOID_2_PORT          GPIOA

// Data Structures
typedef struct {
    char text[OCR_RESULT_MAX_LENGTH];
    float confidence;
    uint32_t timestamp;
} ocr_result_t;

typedef struct {
    uint32_t cpu_usage;
    uint32_t memory_usage;
    uint32_t error_count;
    uint32_t inference_time;
} system_status_t;

typedef struct {
    uint32_t error_code;
    uint32_t timestamp;
    char description[64];
} error_info_t;

// Global Variables (extern declarations)
extern utron_sem_t sem_image_ready;
extern utron_sem_t sem_inference_done;
extern utron_sem_t sem_audio_complete;

extern utron_msgq_t mq_ocr_results;
extern utron_msgq_t mq_system_status;
extern utron_msgq_t mq_error_handling;

// Function Prototypes
void SystemClock_Config(void);
void create_semaphores(void);
void create_message_queues(void);
void create_camera_task(void);
void create_ai_task(void);
void create_audio_task(void);
void create_solenoid_task(void);
void create_system_task(void);

#endif // UTRON_CONFIG_H