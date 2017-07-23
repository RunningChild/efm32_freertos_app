/**************************************************************************//**
 * @file uart.c
 * @brief Booatloader UART communication
 * @author Silicon Labs
 * @version 1.04
 ******************************************************************************
 * @section License
 * <b>(C) Copyright 2014 Silicon Labs, http://www.silabs.com</b>
 *******************************************************************************
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Silicon Labs has no
 * obligation to support this Software. Silicon Labs is providing the
 * Software "AS IS", with no express or implied warranties of any kind,
 * including, but not limited to, any implied warranties of merchantability
 * or fitness for any particular purpose or warranties against infringement
 * of any proprietary rights of a third party.
 *
 * Silicon Labs will not be liable for any consequential, incidental, or
 * special damages, or any other relief, or for any claim by any third party,
 * arising from your use of this Software.
 *
 ******************************************************************************/


#include "em_device.h"
#include "em_usart.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "uart.h"
#include "boot_bootloader_config.h"



/******************************************************************************
 * Enables UART for the bootloader.
 *****************************************************************************/
void BLUART_init(void)
{
#if (false == USE_LEUART)
  /* Enable the required clocks */
  CMU_ClockEnable(UART_CLOCK, true);
  CMU_ClockEnable(cmuClock_GPIO, true);

  /* Configure GPIO */
  GPIO_PinModeSet(TXPORT, TXPIN, gpioModePushPull, 1);
  GPIO_PinModeSet(RXPORT, RXPIN, gpioModeInputPull, 1);//输入电平状态需要被读取，需要设置为输入上拉

  /* Configure USART peripheral. Default configuration
   * is fine. We only need to set the baud rate.  */
  USART_InitAsync_TypeDef uartInit = USART_INITASYNC_DEFAULT;
  uartInit.baudrate = BOOTLOADER_BAUDRATE;
  USART_InitAsync(UART, &uartInit);

  /* Enable RX and TX and set location */
  UART->ROUTE = USART_ROUTE_TXPEN | USART_ROUTE_RXPEN | UART_LOC;

#else

    LEUART_Init_TypeDef leuartInit = LEUART_INIT_DEFAULT;

    // Enable clocks
    CMU_ClockEnable(cmuClock_HFPER, true);
    CMU_ClockEnable(cmuClock_GPIO, true);
    CMU_ClockEnable(cmuClock_HFLE, true);

    leuartInit.baudrate = BOOTLOADER_BAUDRATE;
    if (leuartInit.baudrate <= 9600)
    {
        // Need to use LFCLK branch to get these low baudrates
        if (CMU->STATUS & CMU_STATUS_LFXOENS)
        {
            CMU_ClockSelectSet(cmuClock_LFB, cmuSelect_LFXO);
        }
        else if (CMU->STATUS & CMU_STATUS_LFRCOENS)//_by_pbh use CMU_STATUS_LFRCOENS
        {
            CMU_ClockSelectSet(cmuClock_LFB, cmuSelect_LFRCO);//_by_pbh use LFRCO
        }
        else
        {
            APP_ERROR_CHECK_BOOL(false);
            return /*ECODE_EMDRV_UARTDRV_CLOCK_ERROR*/;
        }
    }
    else
    {
        // Try to figure out the prescaler that will give us the best stability
        CMU_ClockSelectSet(cmuClock_LFB, cmuSelect_HFCLKLE);

        // Attainable baudrate lies between refclk and refclk/128. For maximum
        // accuracy, we want the reference clock to be as high as possible.
        uint32_t refclk = CMU_ClockFreqGet(cmuClock_LFB);
        uint8_t divisor = 0;

        while ((leuartInit.baudrate <= (refclk >> (divisor + 7)))
        && (divisor <= _CMU_LFBPRESC0_MASK))
        {
            divisor++;
        }

        // If we ran out of stretch on the clock division, error out.
        if (divisor > _CMU_LFBPRESC0_MASK)
        {
            APP_ERROR_CHECK_BOOL(false);
            return /*ECODE_EMDRV_UARTDRV_CLOCK_ERROR*/;
        }

        CMU_ClockDivSet(UART_CLOCK, (CMU_ClkDiv_TypeDef) (1 << divisor));
    }

    CMU_ClockEnable(UART_CLOCK, true);

    leuartInit.enable = leuartDisable;
    LEUART_Init(UART, &leuartInit);

    // Discard false frames and/or IRQs
    UART->CMD = LEUART_CMD_CLEARRX | LEUART_CMD_CLEARTX;

    // Clear any false IRQ/DMA request
    LEUART_IntClear(UART, ~0x0);


    // Configure GPIO
    GPIO_PinModeSet(TXPORT, TXPIN, gpioModePushPull, 1);
    GPIO_PinModeSet(RXPORT, RXPIN, gpioModeInputPull, 1);//输入电平状态需要被读取，需要设置为输入上拉

    // Enable TX && RX
    // wait，Enable RX and TX and set location
    LEUART_Enable(UART, leuartEnable);

    while (!(UART->STATUS & LEUART_STATUS_TXENS));
    while (!(UART->STATUS & LEUART_STATUS_RXENS));

    UART->ROUTE = LEUART_ROUTE_TXPEN
                  | LEUART_ROUTE_RXPEN
                  | (UART_LOC
                     << _LEUART_ROUTE_LOCATION_SHIFT);
#endif

}

/******************************************************************************
 * Receives one byte from UART. This function will stall until
 * a byte is available.
 *****************************************************************************/
RAMFUNC  uint8_t BLUART_receive(void)
{
#if (false == USE_LEUART)
  while ( !(UART->STATUS & USART_STATUS_RXDATAV) );
#else
  while ( !(UART->STATUS & LEUART_STATUS_RXDATAV) );
#endif
  return UART->RXDATA;
}

/******************************************************************************
 * Send one byte over UART.
 *****************************************************************************/
RAMFUNC  void BLUART_send(uint8_t data)
{
#if (false == USE_LEUART)
  while ( !(UART->STATUS & USART_STATUS_TXBL) );
  UART->TXDATA = data;
#else
  while ( !(UART->STATUS & LEUART_STATUS_TXBL) );
  UART->TXDATA = data;
#endif
}

/******************************************************************************
 * Send a string over UART.
 *****************************************************************************/
RAMFUNC void BLUART_sendString(char *str)
{
  while (1) {
    if ( *str == 0 )
      break;
    BLUART_send((uint8_t)*str++);
  }

#if (false == USE_LEUART)
  while ( !(UART->STATUS & USART_STATUS_TXC) );
#else
  while ( !(UART->STATUS & LEUART_STATUS_TXC) );
#endif
}

/******************************************************************************
 * @brief Blog_uart_init
 *****************************************************************************/
int16_t Blog_uart_init(uart_rx uart_rx_cb)
{
    static bool initialized = false;
    if (initialized)
    {
        return 0;
    }

    BLUART_init();

    initialized = true;

    return 0;
}

/******************************************************************************
 * @brief Blog_uart_tx
 *****************************************************************************/
int16_t Blog_uart_tx(uint8_t *data, uint32_t count)
{
    BLUART_sendString((char *)data);
    return 0;
}

/******************************************************************************
 * @brief Blog_uart_shutdown
 *****************************************************************************/
int16_t Blog_uart_shutdown(void)
{
    return 0;
}


