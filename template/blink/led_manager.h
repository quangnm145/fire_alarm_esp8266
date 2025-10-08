#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <stdint.h>

// Enum trạng thái LED
typedef enum {
    LED_OFF,
    LED_ON,
    LED_BLINK
} led_state_t;

// Khai báo hàm
void led_manager_init(void);
void set_led_state(uint8_t led_index, led_state_t state, uint32_t interval_ms);

#endif