#ifndef LED_BLINK_H
#define LED_BLINK_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 初始化LED GPIO
 */
void led_init(void);

/**
 * @brief 设置LED状态
 * @param state true为亮，false为灭
 */
void led_set_state(bool state);

/**
 * @brief 切换LED状态
 */
void led_toggle(void);

/**
 * @brief LED闪烁任务
 * @param pvParameters 闪烁周期(ms)
 */
void led_blink_task(void *pvParameters);

/**
 * @brief 启动LED闪烁
 * @param period_ms 闪烁周期，单位毫秒
 */
void led_blink_start(uint32_t period_ms);

#endif // LED_BLINK_H