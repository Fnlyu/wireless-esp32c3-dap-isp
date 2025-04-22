#ifndef YMODEM_TRANSFER_H
#define YMODEM_TRANSFER_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/uart.h"

// YMODEM 协议常量
#define YMODEM_SOH      0x01    // 128字节数据包起始
#define YMODEM_STX      0x02    // 1024字节数据包起始
#define YMODEM_EOT      0x04    // 传输结束
#define YMODEM_ACK      0x06    // 确认
#define YMODEM_NAK      0x15    // 否认，请求重发
#define YMODEM_CAN      0x18    // 取消传输
#define YMODEM_C        0x43    // ASCII 'C'，表示CRC模式

// YMODEM 数据包大小
#define YMODEM_PACKET_SIZE_128  128
#define YMODEM_PACKET_SIZE_1024 1024

// YMODEM 传输状态
typedef enum {
    YMODEM_STATUS_INIT = 0,
    YMODEM_STATUS_SENDING,
    YMODEM_STATUS_DONE,
    YMODEM_STATUS_ERROR
} ymodem_status_t;

// YMODEM 控制块
typedef struct {
    uart_port_t uart_port;      // UART端口号
    uint32_t baudrate;          // 波特率
    uint32_t total_size;        // 文件总大小
    uint32_t sent_size;         // 已发送大小
    uint8_t sequence;           // 当前包序号
    ymodem_status_t status;     // 当前状态
    char filename[64];          // 文件名
    uint32_t retries;           // 重试次数
    uint32_t timeout_ms;        // 超时时间(毫秒)
} ymodem_t;

/**
 * 初始化YMODEM传输
 * 
 * @param ym YMODEM控制块
 * @param uart_port UART端口号
 * @param baudrate 波特率
 * @param filesize 文件大小
 * @param filename 文件名
 * @return 成功返回NULL，失败返回错误消息
 */
const char* ymodem_init(ymodem_t* ym, uart_port_t uart_port, uint32_t baudrate, 
                        uint32_t filesize, const char* filename);

/**
 * 写入数据块
 * 
 * @param ym YMODEM控制块
 * @param data 数据指针
 * @param len 数据长度
 * @return 成功返回NULL，失败返回错误消息
 */
const char* ymodem_write(ymodem_t* ym, void* data, size_t len);

/**
 * 结束YMODEM传输
 * 
 * @param ym YMODEM控制块
 * @return 成功返回NULL，失败返回错误消息
 */
const char* ymodem_end(ymodem_t* ym);

#endif // YMODEM_TRANSFER_H