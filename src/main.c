#include "em_chip.h"
#include "em_rmu.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "sleep.h"

#include "wdt.h"
#include "app_timer.h"
#include "delay.h"
#include "hal_manager.h"

#include "log.h"

#include "bsp_config.h"

//所有task的头文件
#include "global.h"
#include "thread_manager.h"

/******************************************************************************
 * @brief  cmu_setup
 *****************************************************************************/
static void cmu_setup(void)
{
#if (HAL_USE_HFXO == 1)
    #warning "use HFXO"
    /* Enable clock for HF peripherals */
    CMU_ClockEnable(cmuClock_HFPER, true);
    /* Setup crystal frequency */
    SystemHFXOClockSet(configCPU_CLOCK_HZ_DEFINE);
    /* Use HFXO as core clock frequency */
    CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);
    /* Disable HFRCO */
    CMU_OscillatorEnable(cmuOsc_HFRCO, false, false);

    /* enable HFXO */
    CMU_OscillatorEnable(cmuOsc_HFXO, true, true);
    while (!(CMU->STATUS & CMU_STATUS_HFXORDY))
    {
    }
#else
    #warning "not use HFXO"
    //上电默认是内部RC振荡器14MHZ，可以调用以下接口修改HFRCO频率
    //Set the clock frequency to 11MHz so the ADC can run on the undivided HFCLK
    CMU_HFRCOBandSet(cmuHFRCOBand_28MHz);
#endif

    /* Starting LFRCO and waiting until it is stable */
    CMU_OscillatorEnable(cmuOsc_LFRCO, true, true);
    while (!(CMU->STATUS & CMU_STATUS_LFRCORDY))
    {
    }

    /* Enabling clock to the interface of the low energy modules */
    CMU_ClockEnable(cmuClock_CORELE, true);

#if (HAL_USE_LFXO == 1)
    #warning "use LFXO"
    /* Starting LFXO and waiting in EM2 until it is stable */
    CMU_OscillatorEnable(cmuOsc_LFXO, true, false);
    while (!(CMU->STATUS & CMU_STATUS_LFXORDY))
    {
    }
#else
    #warning "not use LFXO"
#endif
}

/******************************************************************************
 * @brief  Main function
 *****************************************************************************/
int main(void)
{
    /* Chip errata */
    CHIP_Init();
    /* If first word of user data page is non-zero, enable eA Profiler trace */
    //BSP_TraceProfilerSetup();

    /* Initialize SLEEP driver, no calbacks are used */
    SLEEP_Init(NULL, NULL);

#if (configSLEEP_MODE < 3)
    /* do not let to sleep deeper than define */
    SLEEP_SleepBlockBegin((SLEEP_EnergyMode_t)(configSLEEP_MODE+1));
#endif

    //时钟系统构建初始化
    cmu_setup();

    //设置中断优先级组
    NVIC_SetPriorityGrouping(0);//[7:5] b100-b000 全部设置为抢占式优先级

    //看门狗初始化
    wdt_init();
    wdt_start();

    //日志模块初始化：日志线程
    log_init(log_uart_init, log_uart_tx, log_uart_shutdown);

    //软件定时器初始化
    app_timer_init();

    //定时器实现延时初始化
    delay_init();

    /******************************************************
    * 以下创建各个部分线程，并为各个线程分配软件看门狗。同时创建监控线程，负责监控各个线程的存活。
    ******************************************************/

    //1、系统事件调度初始化:事件调度线程
    app_task_sched_init();

    //硬件初始化：创建各个模块相应的线程
    hal_init();

    //管理线程
    manager_init();

    //监控线程
    monitor_init();

    //周期性线程
    period_init();

    //开始FreeRTOS调度器
    vTaskStartScheduler();

    //如果一切正常，main()函数不应该会执行到这里。但如果执行到这里，很可能是内存堆空间不足导致空闲任务无法创建。
    while(1)
    {
    }

    return 0;
}
