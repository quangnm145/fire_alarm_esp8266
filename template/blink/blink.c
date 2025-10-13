#include "espressif/esp_common.h"
#include "esp/gpio.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define GPIO_PIN 12  // GPIO 12
#define ACTIVE_LOW 0 // Active low: ngắt khi pin LOW
const gpio_inttype_t INT_TYPE = GPIO_INTTYPE_EDGE_NEG; // Ngắt falling edge (HIGH -> LOW)

static QueueHandle_t ts_queue;

void gpio_intr_handler(uint8_t gpio_num) {
    if (gpio_num == GPIO_PIN) {
        uint32_t now = xTaskGetTickCountFromISR(); // Lấy tick từ FreeRTOS
        xQueueSendToBackFromISR(ts_queue, &now, NULL);
    }
}

void button_task(void *pvParameters) {
    uint32_t last_interrupt_time = 0;
    printf("Đang chờ ngắt trên GPIO %d...\r\n", GPIO_PIN);
    
    while (1) {
        uint32_t interrupt_time;
        if (xQueueReceive(ts_queue, &interrupt_time, portMAX_DELAY) == pdTRUE) {
            interrupt_time *= portTICK_PERIOD_MS; // Chuyển sang ms
            if (last_interrupt_time < interrupt_time - 200) { // Debounce
                printf("Ngắt xảy ra tại %lu ms!\r\n", interrupt_time);
                last_interrupt_time = interrupt_time;
            }
        }
    }
}

void user_init(void) {
    uart_set_baud(0, 115200);
    
    ts_queue = xQueueCreate(10, sizeof(uint32_t));
    if (ts_queue == NULL) {
        printf("Lỗi tạo queue!\r\n");
        return;
    }
    
    gpio_enable(GPIO_PIN, GPIO_INPUT);
    gpio_set_interrupt(GPIO_PIN, INT_TYPE, gpio_intr_handler);
    
    xTaskCreate(button_task, "button_task", 1000, NULL, 5, NULL);
    
    printf("Hệ thống khởi động. Kết nối nút bấm vào GPIO 12 để test ngắt.\r\n");
}