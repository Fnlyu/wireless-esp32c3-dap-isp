#include "led_blink.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *LED_TAG = "LED_BLINK";

#define LED_GPIO_PIN GPIO_NUM_12

static bool led_state = false;
static TaskHandle_t led_task_handle = NULL;

void led_init(void) {
    // 简化GPIO配置
    esp_err_t ret = gpio_reset_pin(LED_GPIO_PIN);
    if (ret != ESP_OK) {
        ESP_LOGE(LED_TAG, "GPIO reset failed");
        return;
    }
    
    ret = gpio_set_direction(LED_GPIO_PIN, GPIO_MODE_OUTPUT);
    if (ret != ESP_OK) {
        ESP_LOGE(LED_TAG, "GPIO set direction failed");
        return;
    }
    
    gpio_set_level(LED_GPIO_PIN, 0);
    ESP_LOGI(LED_TAG, "LED initialized on GPIO%d", LED_GPIO_PIN);
}

void led_blink_task(void *pvParameters) {
    uint32_t blink_period_ms = (uint32_t)(uintptr_t)pvParameters;
    
    if (blink_period_ms == 0) {
        blink_period_ms = 1000;
    }
    
    // 减少日志输出，避免控制台冲突
    ESP_LOGI(LED_TAG, "LED blink started, period: %ld ms", blink_period_ms);
    
    while (1) {
        led_state = !led_state;
        gpio_set_level(LED_GPIO_PIN, led_state ? 1 : 0);
        vTaskDelay(pdMS_TO_TICKS(blink_period_ms));
    }
}

void led_blink_start(uint32_t period_ms) {
    if (led_task_handle != NULL) {
        ESP_LOGW(LED_TAG, "LED task already running");
        return;
    }
    
    // 使用更大的堆栈和更低的优先级
    BaseType_t ret = xTaskCreate(
        led_blink_task, 
        "led_blink", 
        2048,  // 增加堆栈大小
        (void*)(uintptr_t)period_ms, 
        1,     // 最低优先级
        &led_task_handle
    );
    
    if (ret != pdPASS) {
        ESP_LOGE(LED_TAG, "Failed to create LED task");
        led_task_handle = NULL;
    }
}