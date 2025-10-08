#include <stdlib.h>
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "esp8266.h"
#include "led_manager.h"

// Task demo để thay đổi trạng thái LED
void demo_task(void *pvParameters)
{
    while (1) {
        // Ví dụ: Thay đổi trạng thái LED sau mỗi 5s
        set_led_state(0, LED_OFF, 0);      // LED da trời ON
        set_led_state(1, LED_ON, 0); // LED xanh nhấp nháy 500ms
        set_led_state(2, LED_ON, 0); // LED vàng nhấp nháy 1s
        set_led_state(3, LED_ON, 0);      // LED đỏ OFF
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

void user_init(void)
{
    uart_set_baud(0, 115200);
    printf("LED Manager System Started\n");

    // Khởi tạo LED manager
    led_manager_init();

    // Tạo task demo
    xTaskCreate(demo_task, "demo_task", 256, NULL, 2, NULL);
}