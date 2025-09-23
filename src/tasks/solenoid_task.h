/**
 * @file solenoid_task.h
 * @brief Solenoid control for tactile feedback using Morse code
 * @details Implements precise timing control for accessibility features
 * @author μTRON Competition Team
 * @date 2025
 */

#ifndef SOLENOID_TASK_H
#define SOLENOID_TASK_H

#include "utron_config.h"

// Morse code timing (milliseconds) - Standard International Morse Code
#define MORSE_DOT_DURATION    200   // Dit duration
#define MORSE_DASH_DURATION   600   // Dah duration (3x dot)
#define MORSE_SYMBOL_GAP      200   // Gap between dots/dashes (1x dot)
#define MORSE_LETTER_GAP      600   // Gap between letters (3x dot)
#define MORSE_WORD_GAP        1400  // Gap between words (7x dot)

// Solenoid hardware configuration
#define SOLENOID_COUNT        2     // Number of available solenoids
#define SOLENOID_VOLTAGE      12    // Operating voltage (V)
#define SOLENOID_CURRENT_MAX  500   // Maximum current (mA)
#define SOLENOID_PULSE_MAX    2000  // Maximum pulse duration (ms)

// Solenoid identifiers
typedef enum {
    SOLENOID_1 = 0,
    SOLENOID_2 = 1,
    SOLENOID_INVALID = 0xFF
} solenoid_id_t;

// Solenoid states
typedef enum {
    SOLENOID_STATE_IDLE,
    SOLENOID_STATE_ACTIVE,
    SOLENOID_STATE_COOLDOWN,
    SOLENOID_STATE_ERROR
} solenoid_state_t;

// Morse code message structure
typedef struct {
    char text[OCR_RESULT_MAX_LENGTH];
    uint8_t priority;
    uint32_t timestamp;
} morse_message_t;

// Solenoid control structure
typedef struct {
    solenoid_id_t id;
    solenoid_state_t state;
    uint32_t pulse_duration;
    uint32_t last_activation;
    uint32_t total_activations;
    uint8_t error_count;
} solenoid_control_t;

// Global variables
extern solenoid_control_t solenoids[SOLENOID_COUNT];

/**
 * @brief Create solenoid control task
 * @details Creates μTRON OS task for tactile feedback management
 */
void create_solenoid_task(void);

/**
 * @brief Solenoid task entry point
 * @param arg Task argument (unused)
 * @details Main solenoid control loop with message processing
 */
void solenoid_task_entry(void *arg);

/**
 * @brief Initialize solenoid hardware
 * @return 0 on success, negative on error
 * @details Configure GPIO, timers, and safety systems
 */
int solenoid_init(void);

/**
 * @brief Configure solenoid parameters
 * @param id Solenoid identifier
 * @param max_duration Maximum pulse duration (ms)
 * @param cooldown_time Minimum time between pulses (ms)
 * @return 0 on success, negative on error
 */
int solenoid_configure(solenoid_id_t id, uint32_t max_duration, uint32_t cooldown_time);

/**
 * @brief Activate solenoid with specified duration
 * @param id Solenoid identifier
 * @param duration Pulse duration in milliseconds
 * @return 0 on success, negative on error
 * @details Provides safety checks and precise timing
 */
int solenoid_pulse(solenoid_id_t id, uint32_t duration);

/**
 * @brief Emergency stop all solenoids
 * @details Immediately deactivate all solenoids
 */
void solenoid_emergency_stop(void);

/**
 * @brief Get solenoid state
 * @param id Solenoid identifier
 * @return Current solenoid state
 */
solenoid_state_t solenoid_get_state(solenoid_id_t id);

// Morse Code Functions

/**
 * @brief Convert text to Morse code output
 * @param text Input text string
 * @return 0 on success, negative on error
 * @details Converts text and queues for tactile output
 */
int solenoid_morse_text(const char *text);

/**
 * @brief Output single character in Morse code
 * @param c Character to convert
 * @param solenoid_id Which solenoid to use
 * @return 0 on success, negative on error
 */
int solenoid_morse_char(char c, solenoid_id_t solenoid_id);

/**
 * @brief Convert character to Morse code pattern
 * @param c Input character
 * @return Morse code string (dots and dashes), NULL if invalid
 * @details Returns pointer to static string with Morse pattern
 */
const char* char_to_morse(char c);

/**
 * @brief Output Morse dot (dit)
 * @param solenoid_id Which solenoid to use
 * @return 0 on success, negative on error
 */
int morse_output_dot(solenoid_id_t solenoid_id);

/**
 * @brief Output Morse dash (dah)
 * @param solenoid_id Which solenoid to use
 * @return 0 on success, negative on error
 */
int morse_output_dash(solenoid_id_t solenoid_id);

/**
 * @brief Insert appropriate Morse timing gap
 * @param gap_type Type of gap (symbol, letter, word)
 */
void morse_gap(uint32_t gap_duration);

// Advanced Features

/**
 * @brief Two-solenoid Morse output
 * @param text Input text
 * @return 0 on success, negative on error
 * @details Use both solenoids for enhanced tactile feedback
 */
int solenoid_dual_morse(const char *text);

/**
 * @brief Priority message handling
 * @param message High-priority message structure
 * @return 0 on success, negative on error
 * @details Interrupt current output for urgent messages
 */
int solenoid_priority_message(const morse_message_t *message);

/**
 * @brief Pattern-based output
 * @param pattern Custom pulse pattern
 * @param pattern_length Number of elements in pattern
 * @param solenoid_id Which solenoid to use
 * @return 0 on success, negative on error
 */
int solenoid_custom_pattern(const uint32_t *pattern, uint32_t pattern_length, solenoid_id_t solenoid_id);

// Safety and Monitoring

/**
 * @brief Check solenoid thermal status
 * @param id Solenoid identifier
 * @return 0 if OK, positive if warning, negative if error
 */
int solenoid_thermal_check(solenoid_id_t id);

/**
 * @brief Self-test all solenoids
 * @return 0 on success, negative on error
 * @details Brief activation test for functionality
 */
int solenoid_self_test(void);

/**
 * @brief Get solenoid statistics
 * @param id Solenoid identifier
 * @param total_activations Total number of activations
 * @param total_time Total active time in milliseconds
 * @param error_count Number of errors encountered
 */
void solenoid_get_stats(solenoid_id_t id, uint32_t *total_activations, 
                       uint32_t *total_time, uint32_t *error_count);

/**
 * @brief Reset solenoid statistics
 * @param id Solenoid identifier (or SOLENOID_INVALID for all)
 */
void solenoid_reset_stats(solenoid_id_t id);

// Timer interrupt handlers

/**
 * @brief Solenoid pulse timer interrupt
 * @details Called when pulse duration expires
 */
void solenoid_pulse_timer_isr(void);

/**
 * @brief Solenoid cooldown timer interrupt
 * @details Called when cooldown period expires
 */
void solenoid_cooldown_timer_isr(void);

// Debug and configuration

/**
 * @brief Set Morse code speed
 * @param wpm Words per minute (5-40 typical range)
 * @details Adjusts all timing parameters proportionally
 */
void morse_set_speed(uint32_t wpm);

/**
 * @brief Get current Morse code speed
 * @return Words per minute
 */
uint32_t morse_get_speed(void);

/**
 * @brief Enable/disable solenoid debug output
 * @param enable 1 to enable, 0 to disable
 */
void solenoid_set_debug(uint8_t enable);

#endif // SOLENOID_TASK_H