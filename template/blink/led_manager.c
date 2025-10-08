#include <stdlib.h>
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"  // Thêm để sử dụng QueueHandle_t, xQueueCreate, xQueueSend, xQueueReceive
#include "esp8266.h"

// Định nghĩa GPIO cho LED
#define LED_SKY_BLUE 2  // Màu da trời
#define LED_GREEN    14  // Xanh
#define LED_YELLOW   5  // Vàng
#define LED_RED      4  // Đỏ

// Enum trạng thái LED
typedef enum {
    LED_OFF,
    LED_ON,
    LED_BLINK
} led_state_t;

// Cấu trúc thông tin LED
typedef struct {
    uint8_t pin;          // GPIO pin
    led_state_t state;    // Trạng thái
    uint32_t interval_ms; // Chu kỳ nhấp nháy (ms)
    TickType_t last_toggle; // Thời điểm toggle cuối
} led_info_t;

// Cấu trúc lệnh gửi qua queue
typedef struct {
    uint8_t led_index;    // 0: da trời, 1: xanh, 2: vàng, 3: đỏ
    led_state_t state;
    uint32_t interval_ms; // Chỉ dùng cho BLINK
} led_command_t;

// Mảng thông tin LED
static led_info_t leds[] = {
    {LED_SKY_BLUE, LED_OFF, 500, 0},
    {LED_GREEN,    LED_OFF, 500, 0},
    {LED_YELLOW,   LED_OFF, 500, 0},
    {LED_RED,      LED_OFF, 500, 0}
};
#define NUM_LEDS (sizeof(leds) / sizeof(leds[0]))

// Queue để nhận lệnh
static QueueHandle_t led_command_queue;

// Task quản lý tất cả LED
static void led_manager_task(void *pvParameters)
{
    led_command_t cmd;

    // Khởi tạo GPIO
    for (int i = 0; i < NUM_LEDS; i++) {
        gpio_enable(leds[i].pin, GPIO_OUTPUT);
        gpio_write(leds[i].pin, 0); // OFF ban đầu (giả định active-high)
    }

    while (1) {
        // Kiểm tra queue để nhận lệnh
        if (xQueueReceive(led_command_queue, &cmd, 0) == pdTRUE) {
            if (cmd.led_index < NUM_LEDS) {
                leds[cmd.led_index].state = cmd.state;
                leds[cmd.led_index].interval_ms = cmd.interval_ms;
                leds[cmd.led_index].last_toggle = xTaskGetTickCount();

                // Áp dụng ngay nếu không phải BLINK
                if (cmd.state == LED_ON) {
                    gpio_write(leds[cmd.led_index].pin, 1);
                } else if (cmd.state == LED_OFF) {
                    gpio_write(leds[cmd.led_index].pin, 0);
                }
            }
        }

        // Cập nhật trạng thái LED
        TickType_t current_ticks = xTaskGetTickCount();
        for (int i = 0; i < NUM_LEDS; i++) {
            if (leds[i].state == LED_BLINK) {
                if ((current_ticks - leds[i].last_toggle) * portTICK_PERIOD_MS >= leds[i].interval_ms) {
                    gpio_write(leds[i].pin, !gpio_read(leds[i].pin));
                    leds[i].last_toggle = current_ticks;
                }
            }
        }

        // Delay nhẹ để nhường CPU
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// Hàm gửi lệnh thay đổi trạng thái LED
void set_led_state(uint8_t led_index, led_state_t state, uint32_t interval_ms)
{
    led_command_t cmd = {
        .led_index = led_index,
        .state = state,
        .interval_ms = interval_ms
    };
    xQueueSend(led_command_queue, &cmd, portMAX_DELAY);
}

// Hàm khởi tạo LED manager
void led_manager_init(void)
{
    // Tạo queue
    led_command_queue = xQueueCreate(10, sizeof(led_command_t));

    // Tạo task
    xTaskCreate(led_manager_task, "led_manager_task", 256, NULL, 2, NULL);
}