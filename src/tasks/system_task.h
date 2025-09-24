/**
 * @file system_task.h
 * @brief System monitoring and management task for comprehensive system control
 * @details Implements system monitoring, performance tracking, and power management using μTRON OS
 * @author μTRON Competition Team
 * @date 2025
 */

#ifndef SYSTEM_TASK_H
#define SYSTEM_TASK_H

#include "utron_config.h"
#include "ai_task.h"
#include "audio_task.h"
#include "camera_task.h"
#include "solenoid_task.h"

// System task configuration
#define SYSTEM_TASK_PERIOD_MS      100    // 100ms monitoring period
#define SYSTEM_TASK_PRIORITY       5      // Lowest priority
#define SYSTEM_WATCHDOG_TIMEOUT_MS 5000   // 5 second watchdog

// Performance monitoring
#define PERFORMANCE_HISTORY_SIZE   60     // 60 samples (6 seconds at 100ms)
#define ERROR_LOG_SIZE            32     // 32 error entries
#define STATISTICS_BUFFER_SIZE    256    // Statistics buffer

// Power management modes
#define POWER_MODE_HIGH_PERFORMANCE 0    // Full performance
#define POWER_MODE_BALANCED        1     // Balanced mode
#define POWER_MODE_POWER_SAVE      2     // Power saving
#define POWER_MODE_EMERGENCY       3     // Emergency mode

// System health thresholds
#define CPU_USAGE_WARNING_PERCENT  80    // CPU usage warning
#define CPU_USAGE_CRITICAL_PERCENT 95    // CPU usage critical
#define MEMORY_WARNING_PERCENT     85    // Memory usage warning
#define MEMORY_CRITICAL_PERCENT    95    // Memory usage critical
#define TEMPERATURE_WARNING_C      75    // Temperature warning
#define TEMPERATURE_CRITICAL_C     85    // Temperature critical

// Task monitoring
#define MAX_MONITORED_TASKS       8      // Maximum tasks to monitor
#define TASK_DEADLINE_TOLERANCE_MS 5     // Deadline miss tolerance

// System states
typedef enum {
    SYSTEM_STATE_INITIALIZING,
    SYSTEM_STATE_NORMAL,
    SYSTEM_STATE_WARNING,
    SYSTEM_STATE_CRITICAL,
    SYSTEM_STATE_EMERGENCY,
    SYSTEM_STATE_RECOVERY,
    SYSTEM_STATE_SHUTDOWN
} system_state_t;

// Error severity levels
typedef enum {
    ERROR_SEVERITY_INFO,
    ERROR_SEVERITY_WARNING,
    ERROR_SEVERITY_ERROR,
    ERROR_SEVERITY_CRITICAL,
    ERROR_SEVERITY_FATAL
} error_severity_t;

// Task status structure
typedef struct {
    uint8_t task_id;
    char task_name[16];
    uint32_t cpu_usage_percent;     // CPU usage percentage
    uint32_t memory_usage_bytes;    // Memory usage
    uint32_t deadline_misses;       // Deadline miss count
    uint32_t error_count;           // Task error count
    uint32_t last_execution_time;   // Last execution timestamp
    uint8_t health_status;          // Task health (0-100)
} task_status_t;

// System performance statistics
typedef struct {
    // CPU statistics
    uint32_t cpu_usage_percent;
    uint32_t cpu_idle_time_percent;
    uint32_t cpu_frequency_mhz;
    uint32_t context_switches_per_sec;
    
    // Memory statistics
    uint32_t total_memory_bytes;
    uint32_t used_memory_bytes;
    uint32_t free_memory_bytes;
    uint32_t peak_memory_usage;
    uint32_t memory_leaks_detected;
    
    // Task statistics
    uint32_t active_task_count;
    uint32_t total_deadline_misses;
    uint32_t task_overrun_count;
    uint32_t scheduler_overhead_us;
    
    // Hardware statistics
    uint32_t temperature_celsius;
    uint32_t voltage_mv;
    uint32_t power_consumption_mw;
    uint32_t npu_utilization_percent;
    
    // Performance metrics
    uint32_t system_uptime_ms;
    uint32_t total_interrupts;
    uint32_t interrupt_latency_max_us;
    uint32_t system_load_average;
} system_performance_t;

// Error log entry
typedef struct {
    uint32_t timestamp;             // Error timestamp
    uint8_t task_id;               // Source task ID
    error_severity_t severity;      // Error severity
    uint32_t error_code;           // Error code
    char description[64];          // Error description
    uint32_t context_data;         // Additional context
} error_log_entry_t;

// System configuration
typedef struct {
    uint8_t power_mode;            // Current power mode
    uint8_t monitoring_enabled;    // Enable performance monitoring
    uint8_t logging_enabled;       // Enable error logging
    uint8_t watchdog_enabled;      // Enable watchdog
    uint8_t thermal_management;    // Enable thermal management
    uint8_t debug_output_enabled;  // Enable debug output
    uint32_t statistics_interval_ms; // Statistics collection interval
} system_config_t;

// System recovery actions
typedef enum {
    RECOVERY_ACTION_NONE,
    RECOVERY_ACTION_TASK_RESTART,
    RECOVERY_ACTION_SUBSYSTEM_RESET,
    RECOVERY_ACTION_SYSTEM_RESTART,
    RECOVERY_ACTION_EMERGENCY_SHUTDOWN
} recovery_action_t;

// System task context
typedef struct {
    // System state
    system_state_t current_state;
    system_state_t previous_state;
    system_config_t config;
    
    // Performance monitoring
    system_performance_t current_stats;
    system_performance_t previous_stats;
    system_performance_t peak_stats;
    uint32_t performance_history[PERFORMANCE_HISTORY_SIZE];
    uint8_t history_index;
    
    // Task monitoring
    task_status_t monitored_tasks[MAX_MONITORED_TASKS];
    uint8_t task_count;
    
    // Error logging
    error_log_entry_t error_log[ERROR_LOG_SIZE];
    uint8_t error_log_head;
    uint8_t error_log_tail;
    uint8_t error_log_count;
    
    // Recovery management
    recovery_action_t pending_recovery;
    uint32_t recovery_attempts;
    uint32_t last_recovery_time;
    
    // Watchdog
    uint32_t watchdog_last_reset;
    uint8_t watchdog_timeout_count;
    
    // Thermal management
    uint32_t thermal_throttle_count;
    uint8_t thermal_emergency_triggered;
    
    // Debug and diagnostics
    uint8_t diagnostics_running;
    uint32_t last_diagnostics_time;
    char status_message[128];
} system_task_context_t;

// Global variables
extern system_task_context_t system_context;
extern system_state_t system_current_state;

// ========================================================================
// Core System Task Functions
// ========================================================================

/**
 * @brief Create system monitoring task
 * @details Creates μTRON OS task with lowest priority for system monitoring
 */
void create_system_task(void);

/**
 * @brief System task entry point
 * @param arg Task argument (unused)
 * @details Main system monitoring loop
 */
void system_task_entry(void *arg);

/**
 * @brief Initialize system monitoring
 * @return 0 on success, negative on error
 * @details Initialize all monitoring subsystems
 */
int system_init(void);

/**
 * @brief Configure system monitoring
 * @param config System configuration
 * @return 0 on success, negative on error
 */
int system_configure(const system_config_t *config);

/**
 * @brief Shutdown system monitoring
 * @return 0 on success, negative on error
 */
int system_shutdown(void);

// ========================================================================
// Performance Monitoring
// ========================================================================

/**
 * @brief Update system performance statistics
 * @details Collect current system performance data
 */
void system_update_performance_stats(void);

/**
 * @brief Get current performance statistics
 * @return Pointer to current statistics
 */
const system_performance_t* system_get_performance_stats(void);

/**
 * @brief Get performance history
 * @param history Output history buffer
 * @param size Buffer size
 * @return Number of samples returned
 */
uint32_t system_get_performance_history(uint32_t *history, uint32_t size);

/**
 * @brief Reset performance statistics
 */
void system_reset_performance_stats(void);

/**
 * @brief Calculate CPU usage
 * @return CPU usage percentage (0-100)
 */
uint32_t system_calculate_cpu_usage(void);

/**
 * @brief Get memory usage statistics
 * @param total_bytes Output total memory
 * @param used_bytes Output used memory
 * @param free_bytes Output free memory
 * @return 0 on success, negative on error
 */
int system_get_memory_stats(uint32_t *total_bytes, uint32_t *used_bytes, uint32_t *free_bytes);

/**
 * @brief Check for memory leaks
 * @return Number of leaked blocks detected
 */
uint32_t system_check_memory_leaks(void);

// ========================================================================
// Task Monitoring
// ========================================================================

/**
 * @brief Register task for monitoring
 * @param task_id Task identifier
 * @param task_name Task name string
 * @return 0 on success, negative on error
 */
int system_register_task(uint8_t task_id, const char *task_name);

/**
 * @brief Unregister task from monitoring
 * @param task_id Task identifier
 * @return 0 on success, negative on error
 */
int system_unregister_task(uint8_t task_id);

/**
 * @brief Update task status
 * @param task_id Task identifier
 * @param cpu_usage CPU usage percentage
 * @param memory_usage Memory usage bytes
 * @return 0 on success, negative on error
 */
int system_update_task_status(uint8_t task_id, uint32_t cpu_usage, uint32_t memory_usage);

/**
 * @brief Report task deadline miss
 * @param task_id Task identifier
 * @param miss_duration_ms Duration of deadline miss
 */
void system_report_deadline_miss(uint8_t task_id, uint32_t miss_duration_ms);

/**
 * @brief Report task error
 * @param task_id Task identifier
 * @param error_code Error code
 */
void system_report_task_error(uint8_t task_id, uint32_t error_code);

/**
 * @brief Get task health score
 * @param task_id Task identifier
 * @return Health score (0-100), negative on error
 */
int system_get_task_health(uint8_t task_id);

/**
 * @brief Get all task statuses
 * @param statuses Output status array
 * @param max_tasks Maximum tasks to return
 * @return Number of tasks returned
 */
uint32_t system_get_all_task_status(task_status_t *statuses, uint32_t max_tasks);

// ========================================================================
// Error Logging and Management
// ========================================================================

/**
 * @brief Log system error
 * @param severity Error severity
 * @param task_id Source task ID
 * @param error_code Error code
 * @param description Error description
 * @param context_data Additional context
 */
void system_log_error(error_severity_t severity, uint8_t task_id, uint32_t error_code,
                     const char *description, uint32_t context_data);

/**
 * @brief Get error log entries
 * @param entries Output entries array
 * @param max_entries Maximum entries to return
 * @return Number of entries returned
 */
uint32_t system_get_error_log(error_log_entry_t *entries, uint32_t max_entries);

/**
 * @brief Clear error log
 */
void system_clear_error_log(void);

/**
 * @brief Get error statistics
 * @param total_errors Total error count
 * @param critical_errors Critical error count
 * @param last_error_time Last error timestamp
 * @return 0 on success, negative on error
 */
int system_get_error_stats(uint32_t *total_errors, uint32_t *critical_errors, 
                          uint32_t *last_error_time);

/**
 * @brief Set error callback
 * @param callback Error callback function
 */
void system_set_error_callback(void (*callback)(const error_log_entry_t *entry));

// ========================================================================
// Power Management
// ========================================================================

/**
 * @brief Set power mode
 * @param mode Power mode (0=high performance, 1=balanced, 2=power save)
 * @return 0 on success, negative on error
 */
int system_set_power_mode(uint8_t mode);

/**
 * @brief Get current power mode
 * @return Current power mode
 */
uint8_t system_get_power_mode(void);

/**
 * @brief Enable/disable dynamic frequency scaling
 * @param enable 1 to enable, 0 to disable
 * @return 0 on success, negative on error
 */
int system_set_dvfs_enabled(uint8_t enable);

/**
 * @brief Get power consumption estimate
 * @return Power consumption in milliwatts
 */
uint32_t system_get_power_consumption(void);

/**
 * @brief Enter low power mode
 * @param duration_ms Duration to stay in low power
 * @return 0 on success, negative on error
 */
int system_enter_low_power(uint32_t duration_ms);

/**
 * @brief Emergency power shutdown
 * @details Immediate shutdown for power emergencies
 */
void system_emergency_power_shutdown(void);

// ========================================================================
// Thermal Management
// ========================================================================

/**
 * @brief Get system temperature
 * @return Temperature in Celsius
 */
uint32_t system_get_temperature(void);

/**
 * @brief Set thermal thresholds
 * @param warning_temp Warning temperature in Celsius
 * @param critical_temp Critical temperature in Celsius
 * @return 0 on success, negative on error
 */
int system_set_thermal_thresholds(uint32_t warning_temp, uint32_t critical_temp);

/**
 * @brief Enable thermal throttling
 * @param enable 1 to enable, 0 to disable
 * @return 0 on success, negative on error
 */
int system_set_thermal_throttling(uint8_t enable);

/**
 * @brief Handle thermal emergency
 * @details Called when temperature exceeds critical threshold
 */
void system_handle_thermal_emergency(void);

// ========================================================================
// System Recovery
// ========================================================================

/**
 * @brief Attempt system recovery
 * @param action Recovery action to take
 * @return 0 on success, negative on failure
 */
int system_attempt_recovery(recovery_action_t action);

/**
 * @brief Restart specific task
 * @param task_id Task to restart
 * @return 0 on success, negative on error
 */
int system_restart_task(uint8_t task_id);

/**
 * @brief Reset subsystem
 * @param subsystem_id Subsystem identifier
 * @return 0 on success, negative on error
 */
int system_reset_subsystem(uint8_t subsystem_id);

/**
 * @brief Prepare for system restart
 * @details Clean shutdown preparation
 */
void system_prepare_restart(void);

/**
 * @brief System emergency stop
 * @details Immediate stop all operations
 */
void system_emergency_stop(void);

// ========================================================================
// Watchdog Management
// ========================================================================

/**
 * @brief Initialize system watchdog
 * @param timeout_ms Watchdog timeout in milliseconds
 * @return 0 on success, negative on error
 */
int system_watchdog_init(uint32_t timeout_ms);

/**
 * @brief Reset watchdog timer
 * @details Must be called periodically to prevent timeout
 */
void system_watchdog_reset(void);

/**
 * @brief Enable/disable watchdog
 * @param enable 1 to enable, 0 to disable
 * @return 0 on success, negative on error
 */
int system_watchdog_enable(uint8_t enable);

/**
 * @brief Register watchdog callback
 * @param callback Function to call on watchdog timeout
 */
void system_watchdog_register_callback(void (*callback)(void));

// ========================================================================
// Diagnostics and Testing
// ========================================================================

/**
 * @brief Run system diagnostics
 * @return 0 if all tests pass, negative on failure
 */
int system_run_diagnostics(void);

/**
 * @brief Run specific diagnostic test
 * @param test_id Test identifier
 * @return 0 on pass, negative on failure
 */
int system_run_diagnostic_test(uint8_t test_id);

/**
 * @brief Get system health score
 * @return Overall system health (0-100)
 */
uint8_t system_get_health_score(void);

/**
 * @brief Generate system status report
 * @param buffer Output buffer
 * @param buffer_size Buffer size
 * @return Bytes written, negative on error
 */
int system_generate_status_report(char *buffer, uint32_t buffer_size);

/**
 * @brief Stress test system
 * @param duration_ms Test duration
 * @return 0 on success, negative on failure
 */
int system_stress_test(uint32_t duration_ms);

// ========================================================================
// Debug and Monitoring Interface
// ========================================================================

/**
 * @brief Get current system state
 * @return Current system state
 */
system_state_t system_get_state(void);

/**
 * @brief Set system state
 * @param state New system state
 */
void system_set_state(system_state_t state);

/**
 * @brief Enable/disable debug output
 * @param enable 1 to enable, 0 to disable
 */
void system_set_debug(uint8_t enable);

/**
 * @brief Dump complete system state
 * @details Output detailed system information for debugging
 */
void system_dump_state(void);

/**
 * @brief Set status message
 * @param message Status message string
 */
void system_set_status_message(const char *message);

/**
 * @brief Get status message
 * @return Current status message
 */
const char* system_get_status_message(void);

// ========================================================================
// Integration Callbacks
// ========================================================================

/**
 * @brief System state change callback
 * @param old_state Previous state
 * @param new_state New state
 * @details Called when system state changes
 */
void system_state_change_callback(system_state_t old_state, system_state_t new_state);

/**
 * @brief Performance threshold callback
 * @param metric_type Performance metric type
 * @param current_value Current value
 * @param threshold_value Threshold value
 * @details Called when performance threshold is exceeded
 */
void system_performance_threshold_callback(uint8_t metric_type, uint32_t current_value, 
                                          uint32_t threshold_value);

/**
 * @brief Critical error callback
 * @param error Error log entry
 * @details Called on critical system errors
 */
void system_critical_error_callback(const error_log_entry_t *error);

// Error codes
#define SYSTEM_ERROR_NONE                    0
#define SYSTEM_ERROR_INIT_FAILED            -1
#define SYSTEM_ERROR_INVALID_CONFIG         -2
#define SYSTEM_ERROR_TASK_REGISTER_FAILED   -3
#define SYSTEM_ERROR_MEMORY_INSUFFICIENT    -4
#define SYSTEM_ERROR_WATCHDOG_TIMEOUT       -5
#define SYSTEM_ERROR_THERMAL_EMERGENCY      -6
#define SYSTEM_ERROR_POWER_FAILURE          -7
#define SYSTEM_ERROR_RECOVERY_FAILED        -8
#define SYSTEM_ERROR_DIAGNOSTICS_FAILED     -9

// Task IDs for system monitoring
#define TASK_ID_CAMERA_TASK    1
#define TASK_ID_AI_TASK        2
#define TASK_ID_AUDIO_TASK     3
#define TASK_ID_SOLENOID_TASK  4
#define TASK_ID_SYSTEM_TASK    5

#endif // SYSTEM_TASK_H