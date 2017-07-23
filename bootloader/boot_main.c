#include "em_chip.h"
#include "em_rmu.h"
#include "em_cmu.h"
#include "em_emu.h"

#include "wdt.h"
#include "log.h"
#include "boot_hal_manager.h"

#include "boot_bootloader.h"
#include "boot_global.h"

/******************************************************************************
 * @brief  cmu_setup
 *****************************************************************************/
static void cmu_setup(void)
{
#if (HAL_USE_HFXO == 1)
    /* Enable clock for HF peripherals */
    CMU_ClockEnable(cmuClock_HFPER, true);
    /* Setup crystal frequency */
    SystemHFXOClockSet(configCPU_CLOCK_HZ_DEFINE);
    /* Use HFXO as core clock frequency */
    CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);
    /* Disable HFRCO */
    CMU_OscillatorEnable(cmuOsc_HFRCO, false, false);

    //CMU_ClockDivSet(cmuClock_HF, 2);
    //CMU->CTRL &= ~_CMU_CTRL_HFLE_MASK;
    //CMU->HFCORECLKDIV = _CMU_HFCORECLKDIV_RESETVALUE;
#else
    //上电默认是内部RC振荡器14MHZ，可以调用以下接口修改HFRCO频率
    //Set the clock frequency to 11MHz so the ADC can run on the undivided HFCLK
    CMU_HFRCOBandSet(cmuHFRCOBand_28MHz);
#endif

    /* Starting LFRCO and waiting until it is stable */
    CMU_OscillatorEnable(cmuOsc_LFRCO, true, true);

    /* Enabling clock to the interface of the low energy modules */
    CMU_ClockEnable(cmuClock_CORELE, true);

#if (HAL_USE_LFXO == 1)
    /* Starting LFXO and waiting in EM2 until it is stable */
    CMU_OscillatorEnable(cmuOsc_LFXO, true, false);
    while (!(CMU->STATUS & CMU_STATUS_LFXORDY))
    {
    }
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

    //时钟系统构建初始化
    cmu_setup();

    //看门狗初始化
//    wdt_init();
//    wdt_start();

    //1、日志模块初始化
    log_init(Blog_uart_init, Blog_uart_tx, Blog_uart_shutdown);
    LOG(LEVEL_DEBUG, "\r\n\r\n\r\nBOOT");//上电打印关键信息
    LOG(LEVEL_DEBUG, "APP = %d.%d.%d.%d",
                    (APP_VERSION >> 24) & 0x0FF, (APP_VERSION >> 16) & 0x0FF,
                    (APP_VERSION >> 8) & 0x0FF, (APP_VERSION) & 0x0FF );

    //2、硬件初始化
    hal_init();

    //3、bootloader实现
    bootloader_entry();

    return 0;
}

