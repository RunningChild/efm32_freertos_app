#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include "string.h"
#include "app_error.h"

#include "uartdrv.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UART_TRANSFER_OK            (ECODE_EMDRV_UARTDRV_OK)
#define UART_TRANSFER_ABORTED       (ECODE_EMDRV_UARTDRV_ABORTED)


//定义：函数指针
typedef int16_t (*uart_tx)(uint8_t *data, uint32_t count);
typedef void (*uart_rx)(uint32_t transferStatus, uint8_t *data, uint32_t count);
typedef int16_t (*uart_shutdown)(void);
typedef int16_t (*uart_init)(uart_rx rx_cb);

//对外函数接口
int16_t uart_open_device(UARTDRV_Handle_t *handle);

#ifdef __cplusplus
}
#endif

#endif
