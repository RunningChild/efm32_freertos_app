#include "uart_driver.h"

#define UART_MAX_DEVICE_NUM        (6)

static UARTDRV_HandleData_t     handleData[UART_MAX_DEVICE_NUM];
static uint8_t                  uart_device_count = 0;

/******************************************************************************
 * @brief uart_device_open
 *****************************************************************************/
int16_t uart_open_device(UARTDRV_Handle_t *handle)
{
    *handle = &(handleData[uart_device_count++]);
    APP_ERROR_CHECK_BOOL(uart_device_count <= UART_MAX_DEVICE_NUM);

    return 0;
}

