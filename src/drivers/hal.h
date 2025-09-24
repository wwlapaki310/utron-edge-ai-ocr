/**
 * @file hal.h
 * @brief Hardware Abstraction Layer (HAL) for STM32N6570-DK platform
 * @details Provides unified hardware interface for all system components
 * @author μTRON Competition Team  
 * @date 2025
 */

#ifndef HAL_H
#define HAL_H

#include "utron_config.h"

// Platform identification
#define HAL_PLATFORM_STM32N6570_DK  1
#define HAL_VERSION_MAJOR           1
#define HAL_VERSION_MINOR           0
#define HAL_VERSION_PATCH           0

// Hardware configuration
#define HAL_CPU_FREQUENCY_HZ        800000000U  // 800MHz Cortex-M55
#define HAL_NPU_FREQUENCY_HZ        1000000000U // 1GHz Neural-ART
#define HAL_SYSTEM_CLOCK_HZ         HAL_CPU_FREQUENCY_HZ

// Memory configuration
#define HAL_SRAM_BASE               0x20000000
#define HAL_SRAM_SIZE               0x430000    // 4.2MB embedded SRAM
#define HAL_PSRAM_BASE              0x90000000  // External PSRAM
#define HAL_PSRAM_SIZE              0x2000000   // 32MB external PSRAM
#define HAL_FLASH_BASE              0x70000000  // Octo-SPI Flash
#define HAL_FLASH_SIZE              0x4000000   // 64MB external Flash

// Interrupt priorities (lower number = higher priority)
#define HAL_IRQ_PRIORITY_CRITICAL   0          // Critical system interrupts
#define HAL_IRQ_PRIORITY_HIGH       1          // Camera DMA, Neural-ART
#define HAL_IRQ_PRIORITY_MEDIUM     2          // Audio I2S/SAI
#define HAL_IRQ_PRIORITY_LOW        3          // GPIO, timers
#define HAL_IRQ_PRIORITY_LOWEST     4          // System monitoring

// DMA configuration
#define HAL_DMA_STREAM_COUNT        16         // Available DMA streams
#define HAL_DMA_CHANNEL_COUNT       8          // DMA channels per stream

// GPIO configuration
#define HAL_GPIO_PORTS              11         // GPIO ports A-K
#define HAL_GPIO_PINS_PER_PORT      16         // 16 pins per port

// Timer configuration
#define HAL_TIMER_COUNT             17         // Available timers
#define HAL_PWM_CHANNEL_COUNT       64         // Total PWM channels

// ========================================================================
// Data Types and Structures
// ========================================================================

// HAL result codes
typedef enum {
    HAL_OK = 0,
    HAL_ERROR = -1,
    HAL_BUSY = -2,
    HAL_TIMEOUT = -3,
    HAL_INVALID_PARAM = -4,
    HAL_NOT_SUPPORTED = -5,
    HAL_RESOURCE_BUSY = -6,
    HAL_INSUFFICIENT_MEMORY = -7
} hal_result_t;

// GPIO pin state
typedef enum {
    HAL_GPIO_LOW = 0,
    HAL_GPIO_HIGH = 1
} hal_gpio_state_t;

// GPIO pin mode
typedef enum {
    HAL_GPIO_MODE_INPUT,
    HAL_GPIO_MODE_OUTPUT,
    HAL_GPIO_MODE_ALTERNATE,
    HAL_GPIO_MODE_ANALOG
} hal_gpio_mode_t;

// GPIO pull configuration
typedef enum {
    HAL_GPIO_PULL_NONE,
    HAL_GPIO_PULL_UP,
    HAL_GPIO_PULL_DOWN
} hal_gpio_pull_t;

// GPIO pin configuration
typedef struct {
    uint8_t port;                   // GPIO port (0=A, 1=B, etc.)
    uint8_t pin;                    // Pin number (0-15)
    hal_gpio_mode_t mode;           // Pin mode
    hal_gpio_pull_t pull;           // Pull-up/down configuration
    uint8_t speed;                  // Output speed (0=low, 3=very high)
    uint8_t alternate_function;     // Alternate function number
} hal_gpio_config_t;

// DMA configuration
typedef struct {
    uint8_t stream;                 // DMA stream number
    uint8_t channel;                // DMA channel number
    uint32_t source_addr;           // Source address
    uint32_t dest_addr;             // Destination address
    uint32_t data_length;           // Data length
    uint8_t priority;               // DMA priority
    uint8_t direction;              // Transfer direction
    uint8_t memory_increment;       // Memory address increment
    uint8_t peripheral_increment;   // Peripheral address increment
} hal_dma_config_t;

// Timer configuration
typedef struct {
    uint8_t timer_id;               // Timer identifier
    uint32_t frequency_hz;          // Timer frequency
    uint32_t period;                // Timer period
    uint8_t mode;                   // Timer mode
    void (*callback)(void);         // Timer callback function
} hal_timer_config_t;

// PWM configuration  
typedef struct {
    uint8_t timer_id;               // Timer identifier
    uint8_t channel;                // PWM channel
    uint32_t frequency_hz;          // PWM frequency
    uint32_t duty_cycle_percent;    // Duty cycle (0-100)
    uint8_t polarity;               // Output polarity
} hal_pwm_config_t;

// ========================================================================
// System Initialization and Configuration
// ========================================================================

/**
 * @brief Initialize Hardware Abstraction Layer
 * @return HAL_OK on success, error code on failure
 * @details Initialize system clocks, interrupts, and core peripherals
 */
hal_result_t hal_init(void);

/**
 * @brief Configure system clocks
 * @param cpu_freq_hz CPU frequency in Hz
 * @param npu_freq_hz NPU frequency in Hz
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_configure_clocks(uint32_t cpu_freq_hz, uint32_t npu_freq_hz);

/**
 * @brief Get system clock frequency
 * @return System clock frequency in Hz
 */
uint32_t hal_get_system_clock(void);

/**
 * @brief Reset specific peripheral
 * @param peripheral_id Peripheral identifier
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_reset_peripheral(uint32_t peripheral_id);

/**
 * @brief Enable/disable peripheral clock
 * @param peripheral_id Peripheral identifier
 * @param enable 1 to enable, 0 to disable
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_peripheral_clock_enable(uint32_t peripheral_id, uint8_t enable);

// ========================================================================
// GPIO Functions
// ========================================================================

/**
 * @brief Configure GPIO pin
 * @param config GPIO pin configuration
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_gpio_configure(const hal_gpio_config_t *config);

/**
 * @brief Set GPIO pin state
 * @param port GPIO port (0=A, 1=B, etc.)
 * @param pin Pin number (0-15)
 * @param state Pin state (HIGH/LOW)
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_gpio_set(uint8_t port, uint8_t pin, hal_gpio_state_t state);

/**
 * @brief Get GPIO pin state
 * @param port GPIO port (0=A, 1=B, etc.)
 * @param pin Pin number (0-15)
 * @return Pin state (HIGH/LOW), negative on error
 */
hal_gpio_state_t hal_gpio_get(uint8_t port, uint8_t pin);

/**
 * @brief Toggle GPIO pin state
 * @param port GPIO port (0=A, 1=B, etc.)
 * @param pin Pin number (0-15)
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_gpio_toggle(uint8_t port, uint8_t pin);

/**
 * @brief Configure GPIO interrupt
 * @param port GPIO port (0=A, 1=B, etc.)
 * @param pin Pin number (0-15)
 * @param trigger_mode Interrupt trigger mode
 * @param callback Interrupt callback function
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_gpio_configure_interrupt(uint8_t port, uint8_t pin, uint8_t trigger_mode,
                                         void (*callback)(uint8_t port, uint8_t pin));

// ========================================================================
// DMA Functions
// ========================================================================

/**
 * @brief Initialize DMA controller
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_dma_init(void);

/**
 * @brief Configure DMA transfer
 * @param config DMA configuration
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_dma_configure(const hal_dma_config_t *config);

/**
 * @brief Start DMA transfer
 * @param stream DMA stream number
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_dma_start(uint8_t stream);

/**
 * @brief Stop DMA transfer
 * @param stream DMA stream number
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_dma_stop(uint8_t stream);

/**
 * @brief Check DMA transfer status
 * @param stream DMA stream number
 * @return 1 if transfer complete, 0 if in progress, negative on error
 */
int hal_dma_is_complete(uint8_t stream);

/**
 * @brief Set DMA callback
 * @param stream DMA stream number
 * @param callback Completion callback function
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_dma_set_callback(uint8_t stream, void (*callback)(uint8_t stream));

// ========================================================================
// Timer Functions
// ========================================================================

/**
 * @brief Configure timer
 * @param config Timer configuration
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_timer_configure(const hal_timer_config_t *config);

/**
 * @brief Start timer
 * @param timer_id Timer identifier
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_timer_start(uint8_t timer_id);

/**
 * @brief Stop timer
 * @param timer_id Timer identifier
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_timer_stop(uint8_t timer_id);

/**
 * @brief Get timer value
 * @param timer_id Timer identifier
 * @return Current timer value, negative on error
 */
int32_t hal_timer_get_value(uint8_t timer_id);

/**
 * @brief Set timer period
 * @param timer_id Timer identifier
 * @param period_us Period in microseconds
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_timer_set_period(uint8_t timer_id, uint32_t period_us);

// ========================================================================
// PWM Functions
// ========================================================================

/**
 * @brief Configure PWM channel
 * @param config PWM configuration
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_pwm_configure(const hal_pwm_config_t *config);

/**
 * @brief Start PWM output
 * @param timer_id Timer identifier
 * @param channel PWM channel
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_pwm_start(uint8_t timer_id, uint8_t channel);

/**
 * @brief Stop PWM output
 * @param timer_id Timer identifier
 * @param channel PWM channel
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_pwm_stop(uint8_t timer_id, uint8_t channel);

/**
 * @brief Set PWM duty cycle
 * @param timer_id Timer identifier
 * @param channel PWM channel
 * @param duty_percent Duty cycle percentage (0-100)
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_pwm_set_duty_cycle(uint8_t timer_id, uint8_t channel, uint32_t duty_percent);

/**
 * @brief Set PWM frequency
 * @param timer_id Timer identifier
 * @param channel PWM channel
 * @param frequency_hz Frequency in Hz
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_pwm_set_frequency(uint8_t timer_id, uint8_t channel, uint32_t frequency_hz);

// ========================================================================
// Interrupt Management
// ========================================================================

/**
 * @brief Initialize interrupt controller
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_interrupt_init(void);

/**
 * @brief Enable interrupt
 * @param irq_number Interrupt number
 * @param priority Interrupt priority
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_interrupt_enable(uint32_t irq_number, uint8_t priority);

/**
 * @brief Disable interrupt
 * @param irq_number Interrupt number
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_interrupt_disable(uint32_t irq_number);

/**
 * @brief Set interrupt priority
 * @param irq_number Interrupt number
 * @param priority New priority
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_interrupt_set_priority(uint32_t irq_number, uint8_t priority);

/**
 * @brief Disable all interrupts
 * @return Previous interrupt state
 */
uint32_t hal_interrupt_disable_all(void);

/**
 * @brief Restore interrupt state
 * @param state Previous interrupt state
 */
void hal_interrupt_restore(uint32_t state);

// ========================================================================
// Memory Management
// ========================================================================

/**
 * @brief Initialize memory subsystem
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_memory_init(void);

/**
 * @brief Configure memory protection
 * @param region_id Memory region identifier
 * @param base_addr Base address
 * @param size Region size
 * @param permissions Access permissions
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_memory_configure_protection(uint8_t region_id, uint32_t base_addr,
                                            uint32_t size, uint32_t permissions);

/**
 * @brief Get total memory size
 * @param memory_type Memory type (SRAM/PSRAM/Flash)
 * @return Memory size in bytes, 0 on error
 */
uint32_t hal_memory_get_size(uint8_t memory_type);

/**
 * @brief Get memory base address
 * @param memory_type Memory type (SRAM/PSRAM/Flash)
 * @return Base address, 0 on error
 */
uint32_t hal_memory_get_base_address(uint8_t memory_type);

/**
 * @brief Cache control
 * @param cache_type Cache type (I-Cache/D-Cache)
 * @param enable 1 to enable, 0 to disable
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_cache_control(uint8_t cache_type, uint8_t enable);

/**
 * @brief Invalidate cache
 * @param cache_type Cache type
 * @param addr Start address (0 for all)
 * @param size Size to invalidate (0 for all)
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_cache_invalidate(uint8_t cache_type, uint32_t addr, uint32_t size);

// ========================================================================
// Time and Delay Functions
// ========================================================================

/**
 * @brief Get system tick counter
 * @return Current tick count
 */
uint32_t hal_get_tick(void);

/**
 * @brief Get system time in microseconds
 * @return Time in microseconds since boot
 */
uint64_t hal_get_time_us(void);

/**
 * @brief Delay for specified microseconds
 * @param delay_us Delay in microseconds
 */
void hal_delay_us(uint32_t delay_us);

/**
 * @brief Delay for specified milliseconds
 * @param delay_ms Delay in milliseconds
 */
void hal_delay_ms(uint32_t delay_ms);

/**
 * @brief Configure high-precision timer
 * @param resolution_us Timer resolution in microseconds
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_precision_timer_init(uint32_t resolution_us);

/**
 * @brief Get high-precision timestamp
 * @return Timestamp in nanoseconds
 */
uint64_t hal_get_precise_time_ns(void);

// ========================================================================
// Power Management
// ========================================================================

/**
 * @brief Set CPU frequency
 * @param frequency_hz Target frequency in Hz
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_set_cpu_frequency(uint32_t frequency_hz);

/**
 * @brief Get CPU frequency
 * @return Current CPU frequency in Hz
 */
uint32_t hal_get_cpu_frequency(void);

/**
 * @brief Enter low power mode
 * @param mode Power mode identifier
 * @param wakeup_sources Wakeup source mask
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_enter_low_power_mode(uint8_t mode, uint32_t wakeup_sources);

/**
 * @brief Enable/disable peripheral power
 * @param peripheral_id Peripheral identifier
 * @param enable 1 to enable, 0 to disable
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_peripheral_power_control(uint32_t peripheral_id, uint8_t enable);

/**
 * @brief Get power consumption estimate
 * @return Power consumption in milliwatts
 */
uint32_t hal_get_power_consumption(void);

// ========================================================================
// Temperature and Voltage Monitoring
// ========================================================================

/**
 * @brief Initialize temperature sensor
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_temperature_sensor_init(void);

/**
 * @brief Get CPU temperature
 * @return Temperature in Celsius, negative on error
 */
int32_t hal_get_temperature(void);

/**
 * @brief Get supply voltage
 * @return Voltage in millivolts, 0 on error
 */
uint32_t hal_get_voltage(void);

/**
 * @brief Set temperature threshold
 * @param threshold_celsius Temperature threshold
 * @param callback Callback function for threshold breach
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_set_temperature_threshold(int32_t threshold_celsius,
                                          void (*callback)(int32_t temp));

// ========================================================================
// Debug and Trace Functions
// ========================================================================

/**
 * @brief Initialize debug interface
 * @param interface_type Debug interface type (SWO/RTT/UART)
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_debug_init(uint8_t interface_type);

/**
 * @brief Output debug string
 * @param str Debug string
 * @return Number of characters sent, negative on error
 */
int hal_debug_printf(const char *str, ...);

/**
 * @brief Enable/disable instruction trace
 * @param enable 1 to enable, 0 to disable
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_trace_enable(uint8_t enable);

/**
 * @brief Add trace marker
 * @param marker_id Marker identifier
 */
void hal_trace_marker(uint8_t marker_id);

/**
 * @brief Get debug interface status
 * @return 1 if debug interface is active, 0 otherwise
 */
uint8_t hal_debug_is_active(void);

// ========================================================================
// Platform-Specific Functions
// ========================================================================

/**
 * @brief Get hardware version
 * @param major Output major version
 * @param minor Output minor version
 * @param revision Output revision
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_get_hardware_version(uint8_t *major, uint8_t *minor, uint8_t *revision);

/**
 * @brief Get unique device ID
 * @param id_buffer Buffer for device ID (12 bytes)
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_get_device_id(uint8_t *id_buffer);

/**
 * @brief System reset
 * @param reset_type Reset type (soft/hard)
 */
void hal_system_reset(uint8_t reset_type);

/**
 * @brief Enter bootloader mode
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_enter_bootloader(void);

// ========================================================================
// Error Handling and Diagnostics
// ========================================================================

/**
 * @brief Get last HAL error
 * @return Last error code
 */
hal_result_t hal_get_last_error(void);

/**
 * @brief Get error description
 * @param error_code Error code
 * @return Error description string
 */
const char* hal_get_error_string(hal_result_t error_code);

/**
 * @brief Run hardware self-test
 * @return HAL_OK if all tests pass, error code on failure
 */
hal_result_t hal_self_test(void);

/**
 * @brief Get HAL version
 * @param version_string Output version string buffer
 * @param buffer_size Buffer size
 * @return HAL_OK on success, error code on failure
 */
hal_result_t hal_get_version(char *version_string, uint32_t buffer_size);

// Memory type definitions
#define HAL_MEMORY_TYPE_SRAM    0
#define HAL_MEMORY_TYPE_PSRAM   1  
#define HAL_MEMORY_TYPE_FLASH   2

// Cache type definitions
#define HAL_CACHE_TYPE_INSTRUCTION  0
#define HAL_CACHE_TYPE_DATA        1

// Power mode definitions
#define HAL_POWER_MODE_RUN         0
#define HAL_POWER_MODE_SLEEP       1
#define HAL_POWER_MODE_STOP        2
#define HAL_POWER_MODE_STANDBY     3

// Debug interface types
#define HAL_DEBUG_INTERFACE_SWO    0
#define HAL_DEBUG_INTERFACE_RTT    1
#define HAL_DEBUG_INTERFACE_UART   2

// Reset types
#define HAL_RESET_TYPE_SOFT        0
#define HAL_RESET_TYPE_HARD        1

// Peripheral IDs (platform specific)
#define HAL_PERIPHERAL_CAMERA      0x01
#define HAL_PERIPHERAL_NEURAL_ART  0x02
#define HAL_PERIPHERAL_I2S         0x03
#define HAL_PERIPHERAL_SAI         0x04
#define HAL_PERIPHERAL_DMA         0x05
#define HAL_PERIPHERAL_GPIO        0x06
#define HAL_PERIPHERAL_TIMER       0x07
#define HAL_PERIPHERAL_ADC         0x08
#define HAL_PERIPHERAL_UART        0x09
#define HAL_PERIPHERAL_I2C         0x0A
#define HAL_PERIPHERAL_SPI         0x0B

#endif // HAL_H