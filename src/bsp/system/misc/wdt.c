#include "wdt.h"

#include "em_wdog.h"

#include "platform.h"
#include "interrupt_config.h"
#ifndef USE_BOOTLOADER
#include "thread_manager.h"
#endif

#include "log.h"

/* Defining the watchdog initialization data */
static WDOG_Init_TypeDef init =
{
    .enable     = false,            /* Do not start watchdog when init done */
    .debugRun   = false,            /* WDOG not counting during debug halt */
    .em2Run     = true,             /* WDOG counting when in EM2 */
    .em3Run     = true,             /* WDOG counting when in EM3 */
    .em4Block   = false,            /* EM4 can be entered */
    .swoscBlock = true,             /* Block disabling LFRCO/LFXO in CMU */
    .lock       = false,            /* Do not lock WDOG configuration (if locked, reset needed to unlock) */
    .clkSel     = wdogClkSelULFRCO,   /* Select the 1kHZ ULFRCO oscillator *///_by_pbh use ULFRCO
    .perSel     = wdogPeriod_8k,   /* Set the watchdog period to 8193 clock periods (8193/1000 = 8 seconds)*/
};

/*******************************************************************************
 * @brief wdt_init
 ******************************************************************************/
void wdt_init(void)
{
    /* Initializing watchdog with choosen settings */
    WDOG_Init(&init);
}

/*******************************************************************************
 * @brief wdt_start
 ******************************************************************************/
void wdt_start(void)
{
    /* Enabling watchdog, since it was not enabled during initialization */
    WDOG_Enable(true);

    /* Locking watchdog register (reset needed to unlock) */
    WDOG_Lock();
}

/*******************************************************************************
 * @brief wdt_feed_safe
 ******************************************************************************/
void wdt_feed_safe(void)
{
#ifndef USE_BOOTLOADER
    //监控线程在运行，说明调度器已经运行
    if(true == check_monitor_thread_running_flag())
    {
        //WDOG_Feed()函数执行时间短，在这里直接用临界区实现互斥
        taskENTER_CRITICAL();
    }
#endif

    WDOG_Feed();

#ifndef USE_BOOTLOADER
    if(true == check_monitor_thread_running_flag())
    {
        taskEXIT_CRITICAL();
    }
#endif
}

