#include <stdlib.h>
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "esp8266.h"
#include "led_manager.h"

// Định nghĩa macro DEBUG (1 = bật debug, 0 = tắt debug)
#define DEBUG 1

static led_info_t leds[] = {
    {LED_BLUE,  LED_OFF, 500, 0, BLUE},
    {LED_SIM,   LED_OFF, 500, 0, SIM},
    {LED_FAULT, LED_OFF, 500, 0, FAULT},
    {LED_FIRE,  LED_OFF, 500, 0, FIRE}
};
#define NUM_LEDS (sizeof(leds) / sizeof(leds[0]))

static QueueHandle_t led_command_queue;

static void led_manager_task(void *pvParameters)
{
    led_command_t cmd;

    for (int i = 0; i < NUM_LEDS; i++) {
        gpio_enable(leds[i].pin, GPIO_OUTPUT);
        gpio_write(leds[i].pin, 0);
    }

    while (1) {
        if (xQueueReceive(led_command_queue, &cmd, 100 / portTICK_PERIOD_MS) == pdTRUE) {
            for (int i = 0; i < NUM_LEDS; i++) {
                if (leds[i].name == cmd.led_name) {
                    leds[i].state = cmd.state;
                    leds[i].interval_ms = cmd.interval_ms;
                    leds[i].last_toggle = xTaskGetTickCount();
                    if (cmd.state == LED_ON) {
                        gpio_write(leds[i].pin, 1);
                    } else if (cmd.state == LED_OFF) {
                        gpio_write(leds[i].pin, 0);
                    }
                    break;
                }
            }
        }

        TickType_t current_ticks = xTaskGetTickCount();
        for (int i = 0; i < NUM_LEDS; i++) {
            if (leds[i].state == LED_BLINK) {
                if ((current_ticks - leds[i].last_toggle) * portTICK_PERIOD_MS >= leds[i].interval_ms) {
                    gpio_write(leds[i].pin, !gpio_read(leds[i].pin));
                    leds[i].last_toggle = current_ticks;
                }
            }
        }

#ifdef DEBUG
        UBaseType_t stack_high_water_mark = uxTaskGetStackHighWaterMark(NULL);
        if (stack_high_water_mark < 50) {
            printf("Warning: LED task stack low: %lu words\n", stack_high_water_mark);
        }
#endif
    }
}

void set_led_state(led_name_t led_name, led_state_t state, uint32_t interval_ms)
{
    if (led_command_queue == NULL || led_name < BLUE || led_name > FIRE) {
#ifdef DEBUG
        printf("Invalid LED command: queue=%p, led_name=%d\n", led_command_queue, led_name);
#endif
        return;
    }
    if (state == LED_BLINK && interval_ms == 0) {
        interval_ms = 500;
    }
    led_command_t cmd = {
        .led_name = led_name,
        .state = state,
        .interval_ms = interval_ms
    };
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t result = xQueueSendFromISR(led_command_queue, &cmd, &xHigherPriorityTaskWoken);
#ifdef DEBUG
    if (result != pdTRUE) {
        printf("Failed to send LED command to queue\n");
    }
#endif
}

void led_manager_init(void)
{
#ifdef DEBUG
    printf("Initializing LED Manager, free heap: %u bytes\n", xPortGetFreeHeapSize());
#endif

    led_command_queue = xQueueCreate(3, sizeof(led_command_t));
    if (led_command_queue == NULL) {
#ifdef DEBUG
        printf("Failed to create LED command queue\n");
#endif
        return;
    }

    xTaskCreate(led_manager_task, "led_manager_task", 768, NULL, 2, NULL);
}