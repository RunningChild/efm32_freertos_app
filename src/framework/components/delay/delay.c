#include "delay.h"

#include "ustimer.h"
#include "nop_delay.h"

/*******************************************************************************
 * @brief delay_init
 ******************************************************************************/
void delay_init(void)
{
#if 0
    uint32_t error_code;

    // Initialization of USTIMER driver
    error_code = USTIMER_Init();

    APP_ERROR_CHECK_BOOL(error_code == APP_SUCCESS);
#endif
}

/*******************************************************************************
 * @brief delay_ms
 ******************************************************************************/
void delay_ms(uint32_t ms)
{
#if 0
    uint32_t error_code;

    // Wait for 250 microseconds
    error_code = USTIMER_Delay(ms*1000);

    APP_ERROR_CHECK_BOOL(error_code == APP_SUCCESS);
#endif

    nop_delay_ms(ms);
}

/*******************************************************************************
 * @brief delay_us
 ******************************************************************************/
void delay_us(uint32_t us)
{
    nop_delay_us(us);
}

