#include "app_timer_hal_rtc.h"
#include "app_timer_hal_rtc_config.h"

#include "em_rtc.h"

#include "app_timer_wall_clock.h"

#include "bsp_config.h"
#include "interrupt_config.h"

#include "log.h"


static bool hal_rtc_initialized = false;

#if defined(EMDRV_RTCDRV_WALLCLOCK_CONFIG)
/***************************************************************************//**
 * @brief hal_rtc_over_flow_irq_process
 ******************************************************************************/
void hal_rtc_over_flow_irq_process(void)
{
    uint32_t overflowcnt = wallclock_get_overflowcnt();

    //更新溢出值
    wallclock_set_overflowcnt(overflowcnt++);
}
#else
/***************************************************************************//**
 * @brief hal_rtc_over_flow_irq_process
 ******************************************************************************/
__attribute__(( weak )) void hal_rtc_over_flow_irq_process(void)
{

}
#endif

/***************************************************************************//**
 * @brief hal_rtc_init
 ******************************************************************************/
Ecode_t hal_rtc_init( void )
{
    if(true == hal_rtc_initialized)
    {
        return ECODE_OK;
    }
    hal_rtc_initialized = true;

    //Ensure LE modules are clocked.
    CMU_ClockEnable(cmuClock_CORELE, true);

    //Enable LFACLK in CMU (will also enable oscillator if not enabled).
#if (HAL_USE_LFXO == 0)
    CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFRCO);
#else
    CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFXO);
#endif

    //Set clock divider.
    CMU_ClockDivSet(cmuClock_RTC, RTC_DIVIDER);

    //Enable RTC module clock.
    CMU_ClockEnable(cmuClock_RTC, true);

    //Initialize RTC.
    const RTC_Init_TypeDef rtcInit =
    {
        true,  // Start counting when init completed.
        false, // Disable updating RTC during debug halt.
        false  // Count until max. to wrap around.
    };

    RTC_Init(&rtcInit);

    // Disable RTC/RTCC interrupt generation.
    RTC_INTDISABLE(RTC_ALL_INTS);
    RTC_INTCLEAR(RTC_ALL_INTS);

    //_by_pbh add macro
    NVIC_SetPriority(NVIC_IRQ_NUM, RTC_IRQ_PRIORITY);

    // Clear and then enable RTC interrupts in NVIC.
    NVIC_CLEARPENDINGIRQ();
    NVIC_ENABLEIRQ();

    // Enable overflow interrupt for wallclock.
    RTC_INTENABLE(RTC_OF_INT);

    return ECODE_OK;
}

/***************************************************************************//**
 * @brief hal_rtc_deinit
 ******************************************************************************/
Ecode_t hal_rtc_deinit( void )
{
    //Disable and clear all interrupt sources.
    NVIC_DISABLEIRQ();
    RTC_INTDISABLE(RTC_ALL_INTS);
    RTC_INTCLEAR(RTC_ALL_INTS);
    NVIC_CLEARPENDINGIRQ();

    //Disable RTC module and its clock.
    RTC_Enable(false);
    CMU_ClockEnable(cmuClock_RTC, false);

    hal_rtc_initialized = false;

    return ECODE_OK;
}

