#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <stdint.h>
#include "FreeRTOS.h" // Required for TickType_t

// Định nghĩa GPIO cho LED
#define LED_BLUE  2   // GPIO pin for blue LED
#define LED_SIM   5  // GPIO pin for SIM status LED
#define LED_FAULT 14   // GPIO pin for fault LED
#define LED_FIRE  4   // GPIO pin for fire detection LED

// Defines possible states for an LED
typedef enum {
    LED_OFF,   // LED is turned off
    LED_BLINK,  // LED is in blinking mode with configurable interval
    LED_ON    // LED is turned on
} led_state_t;

// Enum tên LED
// Defines the names of the LEDs for intuitive control
typedef enum {
    BLUE,   // Blue LED
    SIM,    // SIM status LED
    FAULT,  // Fault LED
    FIRE    // Fire detection LED
} led_name_t;

// Cấu trúc thông tin LED
// Stores configuration and state information for a single LED
typedef struct {
    uint8_t pin;          // GPIO pin number assigned to the LED
    led_state_t state;    // Current state of the LED (OFF, ON, or BLINK)
    uint32_t interval_ms; // Blinking interval in milliseconds (used when state is LED_BLINK)
    TickType_t last_toggle; // Last tick count when the LED was toggled (for blinking timing)
    led_name_t name;      // Name of the LED for identification
} led_info_t;

// Cấu trúc lệnh gửi qua queue
// Represents a command to change the state of an LED, sent via FreeRTOS queue
typedef struct {
    led_name_t led_name;  // Name of the LED (BLUE, SIM, FAULT, FIRE)
    led_state_t state;    // Desired state for the LED (OFF, ON, or BLINK)
    uint32_t interval_ms; // Blinking interval in milliseconds (used when state is LED_BLINK)
} led_command_t;

/**
 * @brief Initializes the LED manager
 * 
 * This function sets up the LED management system by creating a FreeRTOS queue
 * for LED commands and starting a task to handle LED state updates. It must be
 * called before using any other LED manager functions. The function configures
 * the GPIO pins for all LEDs and starts the LED manager task with a priority of 2.
 * 
 * @note This function should be called once during system initialization.
 */
void led_manager_init(void);

/**
 * @brief Sets the state of a specific LED
 * 
 * This function sends a command to the LED manager task via a FreeRTOS queue to
 * change the state of a specified LED. The command includes the LED name, desired
 * state, and blinking interval (if applicable).
 * 
 * @param led_name The name of the LED to control (BLUE, SIM, FAULT, FIRE)
 * @param state The desired state for the LED (LED_OFF, LED_ON, or LED_BLINK)
 * @param interval_ms The blinking interval in milliseconds (used only when state is LED_BLINK)
 * 
 * @note The function is thread-safe and can be called from any task. The command
 *       is queued and processed asynchronously by the LED manager task.
 * @note If led_name is invalid, the command is ignored.
 */
void set_led_state(led_name_t led_name, led_state_t state, uint32_t interval_ms);

#endif