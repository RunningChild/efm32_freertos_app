#include "app_timer_hal_letimer.h"
#include "app_timer_hal_letimer_config.h"

#include "em_letimer.h"

#include "app_timer_wall_clock.h"

#include "bsp_config.h"
#include "interrupt_config.h"

#include "log.h"


static bool hal_letimer_initialized = false;

/***************************************************************************//**
 * @brief hal_letimer_init
 ******************************************************************************/
Ecode_t hal_letimer_init( void )
{
    if(true == hal_letimer_initialized)
    {
        return ECODE_OK;
    }
    hal_letimer_initialized = true;

    //Ensure LE modules are clocked.
    CMU_ClockEnable(cmuClock_CORELE, true);

    //Enable LFACLK in CMU (will also enable oscillator if not enabled).
#if (HAL_USE_LFXO == 0)
    CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFRCO);
#else
    CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFXO);
#endif

    CMU_ClockDivSet(cmuClock_LETIMER0, RTC_DIVIDER);

    CMU_ClockEnable(cmuClock_LETIMER0, true);

    LETIMER_Init_TypeDef letimerInit = LETIMER_INIT_DEFAULT;

    LETIMER_Init(LETIMER0, &letimerInit);

    // Disable RTC/RTCC interrupt generation.
    RTC_INTDISABLE(RTC_ALL_INTS);
    RTC_INTCLEAR(RTC_ALL_INTS);

    //_by_pbh add macro
    NVIC_SetPriority(NVIC_IRQ_NUM, LETIMER_IRQ_PRIORITY);

    // Clear and then enable RTC interrupts in NVIC.
    NVIC_CLEARPENDINGIRQ();
    NVIC_ENABLEIRQ();

    // Enable overflow interrupt for wallclock.
    RTC_INTENABLE(RTC_OF_INT);

    return ECODE_OK;
}

/***************************************************************************//**
 * @brief hal_letimer_deinit
 ******************************************************************************/
Ecode_t hal_letimer_deinit( void )
{
    //Disable and clear all interrupt sources.
    NVIC_DISABLEIRQ();
    RTC_INTDISABLE(RTC_ALL_INTS);
    RTC_INTCLEAR(RTC_ALL_INTS);
    NVIC_CLEARPENDINGIRQ();

    //Disable RTC module and its clock.
    LETIMER_Enable(LETIMER0, false);
    CMU_ClockEnable(cmuClock_LETIMER0, false);

    hal_letimer_initialized = false;

    return ECODE_OK;
}

