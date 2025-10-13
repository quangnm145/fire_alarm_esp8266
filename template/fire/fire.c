#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "esp/gpio.h"
#include "led_manager.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "ssid_config.h"

// Định nghĩa macro DEBUG (1 = bật debug, 0 = tắt debug)
#define DEBUG 1

// Định nghĩa chân GPIO
#define FIRE_SENSOR_PIN 12  // GPIO 12: Tín hiệu báo cháy
#define SIM_STATUS_PIN  13  // GPIO 13: LED trạng thái SIM

// Cấu hình HTTP GET
#define WEB_SERVER "httpbin.org"
#define WEB_PORT "80"
#define WEB_PATH "/get"

// Queue để truyền timestamp từ ISR sang task
static QueueHandle_t fire_queue;

// Hàm ISR cho GPIO 12
void fire_isr_handler(uint8_t gpio_num) {
    if (gpio_num == FIRE_SENSOR_PIN) {
        uint32_t now = xTaskGetTickCountFromISR(); // Lấy thời gian từ FreeRTOS
        xQueueSendToBackFromISR(fire_queue, &now, NULL);
        // Vô hiệu hóa ngắt ngay trong ISR để chỉ xử lý một lần
        gpio_set_interrupt(FIRE_SENSOR_PIN, GPIO_INTTYPE_NONE, NULL);
    }
}

// Task giám sát tín hiệu báo cháy (ưu tiên cao)
static void fire_monitor_task(void *pvParameters)
{
    // Cấu hình GPIO 12 làm input với pull-up
    gpio_enable(FIRE_SENSOR_PIN, GPIO_INPUT);
    gpio_set_pullup(FIRE_SENSOR_PIN, true, true);
    // In trạng thái ban đầu của GPIO 12
#ifdef DEBUG
    printf("Initial state of GPIO %d: %d\n", FIRE_SENSOR_PIN, gpio_read(FIRE_SENSOR_PIN));
#endif
    // Cấu hình interrupt cho GPIO 12 (falling edge)
    gpio_set_interrupt(FIRE_SENSOR_PIN, GPIO_INTTYPE_EDGE_NEG, fire_isr_handler);

    uint32_t last_interrupt_time = 0;
    bool led_active = false; // Theo dõi trạng thái LED

    while (1) {
        uint32_t interrupt_time;
        // Chờ nhận dữ liệu từ queue
        if (xQueueReceive(fire_queue, &interrupt_time, 100 / portTICK_PERIOD_MS) == pdTRUE) {
            interrupt_time *= portTICK_PERIOD_MS; // Chuyển sang ms
            // Debounce: Chỉ xử lý nếu cách lần ngắt trước > 200ms
            if (last_interrupt_time < interrupt_time - 200) {
#ifdef DEBUG
                printf("Fire detected on GPIO %d at %u ms!\n", FIRE_SENSOR_PIN, interrupt_time); // Sửa %lu thành %u
#endif
                // Bật nhấp nháy LED báo cháy
                set_led_state(FIRE, LED_BLINK, 200);
                led_active = true;
                last_interrupt_time = interrupt_time;
#ifdef DEBUG
                // In stack high water mark khi có ngắt
                UBaseType_t stack_high_water_mark = uxTaskGetStackHighWaterMark(NULL);
                printf("Fire task stack high water mark: %lu words\n", stack_high_water_mark);
#endif
            }
        }

        // Kiểm tra trạng thái GPIO 12 để tắt LED khi tín hiệu trở lại mức cao
        if (gpio_read(FIRE_SENSOR_PIN) == 1 && led_active) {
            // Tín hiệu cao: Tắt LED và bật lại ngắt
            set_led_state(FIRE, LED_OFF, 0);
            gpio_set_interrupt(FIRE_SENSOR_PIN, GPIO_INTTYPE_EDGE_NEG, fire_isr_handler);
            led_active = false;
#ifdef DEBUG
            printf("GPIO %d returned to HIGH, LED turned off\n", FIRE_SENSOR_PIN);
            // In stack high water mark khi trạng thái thay đổi
            UBaseType_t stack_high_water_mark = uxTaskGetStackHighWaterMark(NULL);
            printf("Fire task stack high water mark: %lu words\n", stack_high_water_mark);
#endif
        }

        // Delay để giảm tần suất lặp
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

// Task kiểm tra trạng thái SIM bằng HTTP GET
static void sim_monitor_task(void *pvParameters)
{
    // Cấu hình GPIO 13 làm output cho LED
    gpio_enable(SIM_STATUS_PIN, GPIO_OUTPUT);
    int successes = 0, failures = 0;

    while (1) {
        const struct addrinfo hints = {
            .ai_family = AF_UNSPEC,
            .ai_socktype = SOCK_STREAM,
        };
        struct addrinfo *res;

#ifdef DEBUG
        printf("Running DNS lookup for %s...\r\n", WEB_SERVER);
#endif
        int err = getaddrinfo(WEB_SERVER, WEB_PORT, &hints, &res);

        if (err != 0 || res == NULL) {
#ifdef DEBUG
            printf("DNS lookup failed err=%d res=%p\r\n", err, res);
#endif
            if (res)
                freeaddrinfo(res);
            failures++;
            set_led_state(SIM, LED_BLINK, 250); // Nhấp nháy LED khi thất bại
            vTaskDelay(5000 / portTICK_PERIOD_MS);
            continue;
        }

        struct sockaddr *sa = res->ai_addr;
        if (sa->sa_family == AF_INET) {
#ifdef DEBUG
            printf("DNS lookup succeeded. IP=%s\r\n", inet_ntoa(((struct sockaddr_in *)sa)->sin_addr));
#endif
        }

        int s = socket(res->ai_family, res->ai_socktype, 0);
        if (s < 0) {
#ifdef DEBUG
            printf("... Failed to allocate socket.\r\n");
#endif
            freeaddrinfo(res);
            failures++;
            set_led_state(SIM, LED_BLINK, 250); // Nhấp nháy LED khi thất bại
            vTaskDelay(5000 / portTICK_PERIOD_MS);
            continue;
        }

#ifdef DEBUG
        printf("... allocated socket\r\n");
#endif

        if (connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            close(s);
            freeaddrinfo(res);
#ifdef DEBUG
            printf("... socket connect failed.\r\n");
#endif
            failures++;
            set_led_state(SIM, LED_BLINK, 250); // Nhấp nháy LED khi thất bại
            vTaskDelay(10000 / portTICK_PERIOD_MS);
            continue;
        }

#ifdef DEBUG
        printf("... connected\r\n");
#endif
        freeaddrinfo(res);

        const char *req =
            "GET "WEB_PATH" HTTP/1.1\r\n"
            "Host: "WEB_SERVER"\r\n"
            "User-Agent: esp-open-rtos/0.1 esp8266\r\n"
            "Connection: close\r\n"
            "\r\n";
        if (write(s, req, strlen(req)) < 0) {
#ifdef DEBUG
            printf("... socket send failed\r\n");
#endif
            close(s);
            failures++;
            set_led_state(SIM, LED_BLINK, 250); // Nhấp nháy LED khi thất bại
            vTaskDelay(5000 / portTICK_PERIOD_MS);
            continue;
        }

#ifdef DEBUG
        printf("... socket send success\r\n");
#endif

        static char recv_buf[128];
        int r;
        do {
            bzero(recv_buf, 128);
            r = read(s, recv_buf, 127);
            if (r > 0) {
#ifdef DEBUG
                printf("%s", recv_buf);
#endif
            }
        } while (r > 0);

#ifdef DEBUG
        printf("... done reading from socket. Last read return=%d errno=%d\r\n", r, errno);
#endif
        if (r != 0) {
            failures++;
            set_led_state(SIM, LED_BLINK, 250); // Nhấp nháy LED khi thất bại
        } else {
            successes++;
            set_led_state(SIM, LED_ON, 0); // Bật LED khi thành công
        }
        close(s);

#ifdef DEBUG
        printf("successes = %d failures = %d\r\n", successes, failures);
        UBaseType_t stack_high_water_mark = uxTaskGetStackHighWaterMark(NULL);
        printf("SIM task stack high water mark: %lu words\n", stack_high_water_mark);
#endif

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

// Hàm main của ESP8266
void user_init(void)
{
    // Khởi tạo UART
    uart_set_baud(0, 115200);
#ifdef DEBUG
    printf("System init, free heap: %u bytes\n", xPortGetFreeHeapSize());
#endif

    // Cấu hình Wi-Fi
    struct sdk_station_config config = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASS,
    };
    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_config(&config);

    // Khởi tạo queue cho ISR
    fire_queue = xQueueCreate(5, sizeof(uint32_t));
    if (fire_queue == NULL) {
        printf("Lỗi tạo queue!\n");
        return;
    }

    // Khởi tạo LED Manager
    led_manager_init();

    // Tạo task giám sát báo cháy (ưu tiên cao)
    xTaskCreate(fire_monitor_task, "fire_monitor_task", 512, NULL, 5, NULL);

    // Tạo task kiểm tra SIM bằng HTTP (ưu tiên thấp hơn)
    xTaskCreate(sim_monitor_task, "sim_monitor_task", 512, NULL, 3, NULL);
}