
#include "ymodem_transfer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char* TAG = "YMODEM";

// 计算CRC-16 (CCITT-FALSE算法)
static uint16_t ymodem_crc16(uint8_t* data, size_t len) {
    uint16_t crc = 0;
    size_t i, j;
    
    for (i = 0; i < len; i++) {
        crc ^= ((uint16_t)data[i] << 8);
        for (j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    
    return crc;
}

// 等待接收指定字符
static bool wait_for_char(ymodem_t* ym, char expected_char, uint32_t timeout_ms) {
    uint8_t ch;
    int len;
    uint32_t start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    while ((xTaskGetTickCount() * portTICK_PERIOD_MS - start_time) < timeout_ms) {
        len = uart_read_bytes(ym->uart_port, &ch, 1, pdMS_TO_TICKS(100));
        if (len > 0) {
            ESP_LOGD(TAG, "收到字符: 0x%02X", ch);
            if (ch == expected_char) {
                return true;
            } else if (ch == YMODEM_CAN) {
                ESP_LOGE(TAG, "接收到取消传输请求");
                return false;
            }
        }
    }
    
    ESP_LOGW(TAG, "等待字符 0x%02X 超时", expected_char);
    return false;
}

// 发送YMODEM数据包
static bool send_packet(ymodem_t* ym, uint8_t* data, size_t len, uint8_t packet_num) {
    uint8_t header[3];
    uint16_t crc;
    uint8_t crc_bytes[2];
    int retries = 3;  // 最多重试3次
    
    // 准备包头
    header[0] = (len == YMODEM_PACKET_SIZE_128) ? YMODEM_SOH : YMODEM_STX;
    header[1] = packet_num;
    header[2] = 255 - packet_num;  // 序号的补码
    
    while (retries--) {
        // 发送包头
        uart_write_bytes(ym->uart_port, (const char*)header, 3);
        
        // 发送数据
        uart_write_bytes(ym->uart_port, (const char*)data, len);
        
        // 计算并发送CRC
        crc = ymodem_crc16(data, len);
        crc_bytes[0] = (crc >> 8) & 0xFF;
        crc_bytes[1] = crc & 0xFF;
        uart_write_bytes(ym->uart_port, (const char*)crc_bytes, 2);
        
        ESP_LOGI(TAG, "发送数据包: 序号=%d, 长度=%d, CRC=0x%04X", 
                 packet_num, len, crc);
        
        // 等待ACK
        if (wait_for_char(ym, YMODEM_ACK, ym->timeout_ms)) {
            return true;
        }
        
        ESP_LOGW(TAG, "包 %d 发送失败，正在重试", packet_num);
    }
    
    ESP_LOGE(TAG, "包 %d 发送失败，已重试3次", packet_num);
    return false;
}

// 发送文件头包(包含文件名和大小)
static bool send_filename_packet(ymodem_t* ym) {
    uint8_t packet[YMODEM_PACKET_SIZE_128];
    char filesize_str[16];
    
    // 清空缓冲区
    memset(packet, 0, sizeof(packet));
    
    // 文件名
    strncpy((char*)packet, ym->filename, sizeof(packet) - 1);
    
    // 文件大小(ASCII)
    snprintf(filesize_str, sizeof(filesize_str), "%u", ym->total_size);
    strcpy((char*)(packet + strlen(ym->filename) + 1), filesize_str);
    
    ESP_LOGI(TAG, "等待接收方发送'C'字符...");
    
    // 等待接收方发送'C'
    for (int i = 0; i < 10; i++) {
        if (wait_for_char(ym, YMODEM_C, 1000)) {
            // 发送文件名包
            ESP_LOGI(TAG, "发送文件头包: 文件名=%s, 大小=%s", 
                     ym->filename, filesize_str);
            if (send_packet(ym, packet, YMODEM_PACKET_SIZE_128, 0)) {
                return true;
            }
        }
        ESP_LOGW(TAG, "等待'C'字符超时，尝试 %d/10", i+1);
    }
    
    ESP_LOGE(TAG, "发送文件头包失败");
    return false;
}

const char* ymodem_init(ymodem_t* ym, uart_port_t uart_port, uint32_t baudrate, 
                        uint32_t filesize, const char* filename) {
    if (!ym || !filename) {
        return "参数无效";
    }
    
    // 初始化YMODEM控制块
    memset(ym, 0, sizeof(ymodem_t));
    ym->uart_port = uart_port;
    ym->baudrate = baudrate;
    ym->total_size = filesize;
    ym->status = YMODEM_STATUS_INIT;
    ym->sequence = 1;  // 文件数据从序列号1开始
    ym->timeout_ms = 5000;  // 默认超时5秒
    strncpy(ym->filename, filename, sizeof(ym->filename) - 1);
    
    ESP_LOGI(TAG, "初始化YMODEM传输 (端口:%d, 波特率:%d, 文件大小:%u, 文件名:%s)",
             uart_port, baudrate, filesize, filename);
    
    // 配置UART
    uart_config_t uart_config = {
        .baud_rate = baudrate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_NONE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    
    // 尝试释放可能已存在的UART资源
    uart_driver_delete(uart_port);
    
    // 配置UART
    esp_err_t err = uart_param_config(uart_port, &uart_config);
    if (err != ESP_OK) {
        return "UART配置失败";
    }
    
    // 安装UART驱动
    err = uart_driver_install(uart_port, 2048, 2048, 0, NULL, 0);
    if (err != ESP_OK) {
        return "UART驱动安装失败";
    }
    
    // 设置UART引脚(使用默认引脚)
    uart_set_pin(uart_port, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    
    // 清空缓冲区
    uart_flush(uart_port);
    
    // 发送文件头包
    if (!send_filename_packet(ym)) {
        uart_driver_delete(uart_port);
        return "发送文件头包失败";
    }
    
    ym->status = YMODEM_STATUS_SENDING;
    return NULL;  // 成功
}

const char* ymodem_write(ymodem_t* ym, void* data, size_t len) {
    uint8_t* data_ptr = (uint8_t*)data;
    size_t remaining = len;
    size_t packet_size;
    size_t send_size;
    uint8_t packet[YMODEM_PACKET_SIZE_1024];
    
    if (!ym || !data) {
        return "参数无效";
    }
    
    if (ym->status != YMODEM_STATUS_SENDING) {
        return "YMODEM不在发送状态";
    }
    
    ESP_LOGI(TAG, "开始写入数据块: %d字节", len);
    
    while (remaining > 0) {
        // 选择包大小
        if (remaining >= YMODEM_PACKET_SIZE_1024) {
            packet_size = YMODEM_PACKET_SIZE_1024;
        } else if (remaining >= YMODEM_PACKET_SIZE_128) {
            packet_size = YMODEM_PACKET_SIZE_1024; // 仍使用1024字节包
        } else {
            packet_size = YMODEM_PACKET_SIZE_128;
        }
        
        // 准备数据包
        send_size = (remaining < packet_size) ? remaining : packet_size;
        memset(packet, 0, packet_size);  // 填充0
        memcpy(packet, data_ptr, send_size);
        
        ESP_LOGI(TAG, "准备发送数据包: 序号=%d, 大小=%d/%d", 
                 ym->sequence, send_size, packet_size);
        
        // 发送数据包
        if (!send_packet(ym, packet, packet_size, ym->sequence)) {
            ym->status = YMODEM_STATUS_ERROR;
            return "发送数据包失败";
        }
        
        // 更新状态
        data_ptr += send_size;
        remaining -= send_size;
        ym->sent_size += send_size;
        ym->sequence = (ym->sequence + 1) % 256;  // 序列号循环
    }
    
    ESP_LOGI(TAG, "数据块写入成功，已发送: %d/%d", ym->sent_size, ym->total_size);
    return NULL;  // 成功
}

const char* ymodem_end(ymodem_t* ym) {
    uint8_t eot = YMODEM_EOT;
    
    if (!ym) {
        return "参数无效";
    }
    
    if (ym->status == YMODEM_STATUS_ERROR) {
        uart_driver_delete(ym->uart_port);
        return "YMODEM处于错误状态";
    }
    
    ESP_LOGI(TAG, "正在结束YMODEM传输...");
    
    // 发送第一个EOT
    uart_write_bytes(ym->uart_port, (const char*)&eot, 1);
    
    // 等待NAK(接收方通常会用NAK响应第一个EOT)
    if (!wait_for_char(ym, YMODEM_NAK, ym->timeout_ms)) {
        ESP_LOGW(TAG, "未收到NAK响应第一个EOT，尝试继续...");
    }
    
    // 发送第二个EOT
    uart_write_bytes(ym->uart_port, (const char*)&eot, 1);
    
    // 等待ACK
    if (!wait_for_char(ym, YMODEM_ACK, ym->timeout_ms)) {
        uart_driver_delete(ym->uart_port);
        return "结束传输失败，未收到ACK";
    }
    
    ESP_LOGI(TAG, "接收到ACK，准备发送空包...");
    
    // 等待C
    if (!wait_for_char(ym, YMODEM_C, ym->timeout_ms)) {
        uart_driver_delete(ym->uart_port);
        return "结束传输失败，未收到C";
    }
    
    // 发送空包结束传输
    uint8_t empty_packet[YMODEM_PACKET_SIZE_128];
    memset(empty_packet, 0, sizeof(empty_packet));
    
    if (!send_packet(ym, empty_packet, YMODEM_PACKET_SIZE_128, 0)) {
        uart_driver_delete(ym->uart_port);
        return "发送空包失败";
    }
    
    // 释放UART资源
    uart_driver_delete(ym->uart_port);
    
    ym->status = YMODEM_STATUS_DONE;
    ESP_LOGI(TAG, "YMODEM传输完成，共发送 %u 字节", ym->sent_size);
    
    return NULL;  // 成功
}