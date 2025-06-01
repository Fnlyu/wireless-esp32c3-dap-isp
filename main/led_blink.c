#include "led_blink.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *LED_TAG = "LED_BLINK";

// 合宙ESP32C3开发板通常使用GPIO12作为LED引脚
#define LED_GPIO_PIN GPIO_NUM_12

// LED状态
static bool led_state = false;

void led_init(void) {
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << LED_GPIO_PIN),
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    gpio_config(&io_conf);
    
    // 初始化LED为关闭状态
    gpio_set_level(LED_GPIO_PIN, 0);
    ESP_LOGI(LED_TAG, "LED initialized on GPIO%d", LED_GPIO_PIN);
}

void led_set_state(bool state) {
    led_state = state;
    gpio_set_level(LED_GPIO_PIN, state ? 1 : 0);
}

void led_toggle(void) {
    led_state = !led_state;
    gpio_set_level(LED_GPIO_PIN, led_state ? 1 : 0);
}

void led_blink_task(void *pvParameters) {
    uint32_t blink_period_ms = (uint32_t)pvParameters;
    
    if (blink_period_ms == 0) {
        blink_period_ms = 1000; // 默认1秒闪烁
    }
    
    ESP_LOGI(LED_TAG, "LED blink task started, period: %ld ms", blink_period_ms);
    
    while (1) {
        led_toggle();
        ESP_LOGD(LED_TAG, "LED state: %s", led_state ? "ON" : "OFF");
        vTaskDelay(pdMS_TO_TICKS(blink_period_ms));
    }
}

void led_blink_start(uint32_t period_ms) {
    // 创建LED闪烁任务
    xTaskCreate(led_blink_task, "led_blink", 1024, (void*)period_ms, 1, NULL);
}