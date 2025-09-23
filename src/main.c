/**
 * @file main.c
 * @brief μTRON Edge AI OCR - Main application entry point
 * @author μTRON Competition Team
 * @date 2025
 */

#include "utron_config.h"
#include "tasks/camera_task.h"
#include "tasks/ai_task.h"
#include "tasks/audio_task.h"
#include "tasks/solenoid_task.h"
#include "tasks/system_task.h"

/**
 * @brief Main application entry point
 * @return Never returns
 */
int main(void)
{
    // Hardware initialization
    HAL_Init();
    SystemClock_Config();
    
    // μTRON OS initialization
    utron_init();
    
    // Create synchronization objects
    create_semaphores();
    create_message_queues();
    
    // Create application tasks
    create_camera_task();
    create_ai_task();
    create_audio_task();
    create_solenoid_task();
    create_system_task();
    
    // Start μTRON OS scheduler
    utron_start_scheduler();
    
    // Should never reach here
    while(1);
}

/**
 * @brief System clock configuration
 */
void SystemClock_Config(void)
{
    // Configure system clock to 800MHz
    // Configure Neural-ART NPU clock to 1GHz
    // TODO: Implement clock configuration
}

/**
 * @brief Create synchronization objects
 */
void create_semaphores(void)
{
    // Image processing pipeline semaphores
    sem_image_ready = utron_create_semaphore(0, 1);
    sem_inference_done = utron_create_semaphore(0, 1);
    sem_audio_complete = utron_create_semaphore(0, 1);
}

/**
 * @brief Create message queues
 */
void create_message_queues(void)
{
    // OCR results queue
    mq_ocr_results = utron_create_msgqueue(sizeof(ocr_result_t), 8);
    
    // System status queue
    mq_system_status = utron_create_msgqueue(sizeof(system_status_t), 4);
    
    // Error handling queue
    mq_error_handling = utron_create_msgqueue(sizeof(error_info_t), 8);
}

// Task creation functions will be implemented in respective task files