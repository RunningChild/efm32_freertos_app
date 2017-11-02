#include "hal_manager.h"

#include "sleep.h"
#include "gpiointerrupt.h"
#include "em_gpio.h"
#include "inner_rtc_action.h"

#include "app_manager.h"
#include "thread_manager.h"
#include "app_timer.h"

#include "bsp_config.h"

#if defined(FM_INNER_BURTC)
#include "app_timer_hal_burtc.h"
#endif

#include "global.h"

static void gpio_interrupt_init_and_disable(void)
{
    NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
    NVIC_DisableIRQ(GPIO_ODD_IRQn);

    NVIC_ClearPendingIRQ(GPIO_EVEN_IRQn);
    NVIC_DisableIRQ(GPIO_EVEN_IRQn);
}

static void gpio_interrupt_enable(void)
{
    NVIC_EnableIRQ(GPIO_ODD_IRQn);
    NVIC_EnableIRQ(GPIO_EVEN_IRQn);
}

/*******************************************************************************
 * @brief hal_set_common_unuse_pin
 ******************************************************************************/
static void hal_set_common_unuse_pin(void)
{
    GPIO_PinModeSet(COMMON_UNUSE_PORT, COMMON_UNUSE_PIN, gpioModeDisabled, 0);
}

/*******************************************************************************
 * @brief hal_init
 ******************************************************************************/
void hal_init(void)
{
    //外部io中断实现初始化
    GPIOINT_Init();
    gpio_interrupt_init_and_disable();

#if defined(FM_INNER_BURTC)
    //BURTC初始化
    hal_burtc_init();

    //内部BURTC时间，同步到软件万年历
    calendar_sync_from_inner_rtc();
#endif

    //全局设置无用的引脚为失能状态
    hal_set_common_unuse_pin();

    //APP层的各task以及其他应用层相关的初始化
    app_task_init();

    //所有系统输入模块初始化放在最后，以防止上电意外输入，引起异常
    //打开IO中断
    gpio_interrupt_enable();

}

