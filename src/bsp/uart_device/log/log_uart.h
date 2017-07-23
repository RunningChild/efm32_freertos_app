#ifndef LOG_UART_H
#define LOG_UART_H

#include "uart_driver.h"
#include "bsp_config.h"

#include "log.h" // add quote for (RELEASE_VERSION_ENABLE == 0)

#ifdef __cplusplus
extern "C" {
#endif


#if (RELEASE_VERSION_ENABLE == 0)
#define LOG_USART_BAUDRATE           (115200)
#else
#define LOG_USART_BAUDRATE           (115200)//(9600)
#endif

#define  LOG_RX_BUF_MAX_SIZE         (10)

//串口DMA接收超时检测定时器相关
//注意:
//1、LOG_UART_DMA_TIMEOUT_MS = 3 * LOG_DEAL_RX_BUF_SIZE / (bps/8/1000) 该宏定义在log_uart.h中设置
#define LOG_UART_DMA_TIMEOUT_MS      (20)

//注意:
//1、LOG_DEAL_RX_BUF_SIZE < LOG_RX_BUF_MAX_SIZE
//2、LOG_DEAL_RX_BUF_SIZE <= log_uart.h中设置的宏定义LOG_UART_DMA_TIMEOUT_MS*bps/8/1000
#define  LOG_DEAL_RX_BUF_SIZE        (1)


//对外函数接口
void log_uart_dma_timer_restart(uint32_t ms);
int16_t log_uart_init(uart_rx uart_rx_cb);
int16_t log_uart_tx(uint8_t *data, uint32_t count);
void log_uart_rx_start(uart_rx uart_rx_cb);
int16_t log_uart_shutdown(void);


#ifdef __cplusplus
}
#endif

#endif
