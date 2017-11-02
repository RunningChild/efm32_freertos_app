#include "app_timer_hal_burtc.h"
#include "app_timer_hal_burtc_config.h"

#include "em_cmu.h"
#include "em_common.h"
#include "em_int.h"
#include "em_rmu.h"
#include "em_emu.h"
#include "em_burtc.h"

#include "datetime.h"
#include "calendar.h"
#include "app_timer_wall_clock.h"

#include "bsp_config.h"
#include "module_config.h"
#include "interrupt_config.h"

#include "log.h"

static bool hal_burtc_initialized = false;

/******************************************************************************
 * @brief Configure backup RTC
 *****************************************************************************/
static void burtcSetup(void)
{
    //配置burtc
    BURTC_Init_TypeDef burtcInit = BURTC_INIT_DEFAULT;

    burtcInit.mode        = burtcModeEM4;      /* BURTC is enabled to EM4 */
#if (HAL_USE_LFXO == 0)
    burtcInit.clkSel      = burtcClkSelLFRCO;  /* Select LFRCO as clock source */
#else
    burtcInit.clkSel      = burtcClkSelLFXO;   /* Select LFXO as clock source */
#endif
    burtcInit.clkDiv      = RTC_DIVIDER;       /* Choose 32768HZ/RTC_DIVIDER clock frequency */
    burtcInit.compare0Top = false;             /* Wrap on max. */

    burtcInit.enable        = false;
    burtcInit.debugRun      = false;
    burtcInit.timeStamp     = true;
    burtcInit.lowPowerMode  = burtcLPDisable;

    /* Initialization of BURTC */
    BURTC_Init(&burtcInit);
}

#if defined(EMDRV_RTCDRV_WALLCLOCK_CONFIG)
/************************************************************
                    wall clock实现相关
************************************************************/
#warning "wallclock use burtc"

#define BURTC_PRESCALING                        (RTC_DIVIDER)
#define COUNTS_PER_SEC                          (RTC_CLOCK/BURTC_PRESCALING)

static uint32_t overflow_interval;
static uint32_t overflow_interval_r;

/***************************************************************************//**
 * @brief Set up backup domain.
 ******************************************************************************/
static void budSetup(void)
{
    /* Assign default TypeDefs */
    EMU_EM4Init_TypeDef em4Init = EMU_EM4INIT_DEFAULT;
    EMU_BUPDInit_TypeDef bupdInit = EMU_BUPDINIT_DEFAULT;

    /*Setup EM4 configuration structure */
    em4Init.lockConfig = true;
#if (HAL_USE_LFXO == 0)
    em4Init.osc = emuEM4Osc_LFRCO;
#else
    em4Init.osc = emuEM4Osc_LFXO;
#endif
    em4Init.buRtcWakeup = false;
    em4Init.vreg = true;

    /* Setup Backup Power Domain configuration structure */
    bupdInit.probe = emuProbe_Disable;
    bupdInit.bodCal = false;
    bupdInit.statusPinEnable = false;
    bupdInit.resistor = emuRes_Res0;
    bupdInit.voutStrong = false;
    bupdInit.voutMed = false;
    bupdInit.voutWeak = false;
    bupdInit.inactivePower = emuPower_None;
    bupdInit.activePower = emuPower_None;
    bupdInit.enable = true;

    /* Unlock configuration */
    EMU_EM4Lock( false );

    /* Initialize EM4 and Backup Power Domain with init structs */
    EMU_BUPDInit( &bupdInit );
    EMU_EM4Init( &em4Init );

    /* Release reset for backup domain */
    RMU_ResetControl( rmuResetBU, rmuResetModeClear );

    /* Lock configuration */
    EMU_EM4Lock( true );

    LOG(LEVEL_DEBUG, "EMU->STATUS=0x%x", EMU->STATUS);
}

/***************************************************************************//**
 * @brief  Backup CALENDAR to retention registers
 *   RET[0].REG : number of BURTC overflows
 *   RET[1].REG : epoch offset
 ******************************************************************************/
static void clock_backup(void)
{
    uint32_t burtcOverflowCounter = wallclock_get_overflowcnt();
    uint32_t burtcStartTime = wallclock_get_timebase();

    /* Write overflow counter to retention memory */
    BURTC_RetRegSet(0, burtcOverflowCounter);

    /* Write local epoch offset to retention memory */
    BURTC_RetRegSet(1, burtcStartTime);
}

/***************************************************************************//**
 * @brief  Restore CALENDAR from retention registers
 ******************************************************************************/
static void clock_restore( void )
{
    uint32_t burtcCount;
    uint32_t burtcTimestamp;
    uint32_t burtcOverflowCounter = 0;

    /* Store current BURTC value for consistency in display output within this function */
    burtcCount = BURTC_CounterGet();

    /* Timestamp is BURTC value at time of main power loss */
    burtcTimestamp = BURTC_TimestampGet();

    /* Read overflow counter from retention memory */
    burtcOverflowCounter = BURTC_RetRegGet( 0 );

    /* Check for overflow while in backup mode
    Assume that overflow interval >> backup source capacity
    i.e. that overflow has only occured once during main power loss */
    if( (_BURTC_STATUS_BUMODETS_MASK == (BURTC_Status() & BURTC_STATUS_BUMODETS))
     && (_EMU_STATUS_BURDY_MASK == (EMU->STATUS & EMU_STATUS_BURDY))//backup functionality is also available
      )
    {
        LOG_RELEASE(LEVEL_DEBUG, "BUMODE");

        if(burtcCount < burtcTimestamp)
        {
            LOG_RELEASE(LEVEL_DEBUG, "Overflow");
            burtcOverflowCounter++;
        }
    }

    /* Clear BURTC timestamp */
    BURTC_StatusClear();

    /* Restore epoch offset from retention memory */
    wallclock_set_timebase(BURTC_RetRegGet(1));
    LOG(LEVEL_DEBUG, "restore TimeBase=%d", wallclock_get_timebase());

    /* Restore clock overflow counter */
    wallclock_set_overflowcnt(burtcOverflowCounter);

    //考虑到OverflowCounter可能会变化，强制存储
    /* retention registers */
    clock_backup();
}

/***************************************************************************//**
 * @brief clock_Init
 ******************************************************************************/
static bool clock_Init(void)
{
    /* Set overflow interval based on counter width and frequency */
    overflow_interval  =  ((uint64_t)UINT32_MAX+1) / COUNTS_PER_SEC; /* in seconds */
    overflow_interval_r = ((uint64_t)UINT32_MAX+1) % COUNTS_PER_SEC; /* division remainder */

    //检查是否从备份域恢复
    bool is_backup_domain_flag = false;
    unsigned long resetcause = RMU_ResetCauseGet();
    LOG(LEVEL_DEBUG, "resetcause=0x%x\r\n", resetcause);

#if 0
    if( (resetcause & RMU_RSTCAUSE_BUMODERST)
    && !(resetcause & RMU_RSTCAUSE_BUBODREG)
    && !(resetcause & RMU_RSTCAUSE_BUBODUNREG)
    && !(resetcause & RMU_RSTCAUSE_BUBODBUVIN)
    &&  (resetcause & RMU_RSTCAUSE_BUBODVDDDREG)
    && !(resetcause & RMU_RSTCAUSE_EXTRST)
    && !(resetcause & RMU_RSTCAUSE_PORST) )
#else
    if( (0xA5A5 == BURTC_RetRegGet(2))
     && (0x0 == (BURTC_Status() & _BURTC_STATUS_RAMWERR_MASK))
     && (true == BURTC_GetEnable()) )
#endif
    {
        is_backup_domain_flag = true;

        /* If waking from backup mode*/
        /* Restore time from backup RTC + retention memory*/
        clock_restore();
    }
    else
    {
        BURTC_RetRegSet(2, 0x0000);

        //配置burtc
        burtcSetup();

        //计数清零
        RTC_COUNTERRESET();

        wallclock_set_overflowcnt(0);

        //设置默认的系统时间2000-02-01 00:00:00
        wallclock_set_timebase(linux_mktime(BEGYEAR, 2, 1,
                                             0, 0, 0,
                                             0));
        LOG(LEVEL_DEBUG, "settime TimeBase=%d", wallclock_get_timebase());

        /* Start BURTC */
        BURTC_Enable( true );

        /* If normal startup*/
        /* Backup initial calendar (initialize retention registers) */
        clock_backup();

        BURTC_RetRegSet(2, 0xA5A5);
    }

    LOG(LEVEL_DEBUG, "%s domain recovery.", (is_backup_domain_flag)? "backup" : "normal");

    return is_backup_domain_flag;
}

/******************************************************************************
 * @brief Returns the current system time
 *
 * @param timer
 *   If not a null pointer, time is copied to this
 *
 * @return
 *   Current system time. Should, but does not, return -1 if system time is not available
 *
 *****************************************************************************/
uint32_t clock_gettime(void)
{
    uint32_t burtcOverflowCounter = wallclock_get_overflowcnt();

    /* Add the time offset */
    uint32_t t = wallclock_get_timebase();

    /* Add time based on number of counter overflows*/
    t += burtcOverflowCounter * overflow_interval;

    /* Correct if overflow interval is not an integer*/
    if ( overflow_interval_r != 0 )
    {
        t += burtcOverflowCounter * overflow_interval_r / COUNTS_PER_SEC;
    }

    /* Add the number of seconds for BURTC */
    t += (BURTC_CounterGet() / COUNTS_PER_SEC);

    return t;
}

/***************************************************************************//**
 * @brief Set the epoch offset
 ******************************************************************************/
void clock_settime(const uint32_t set_timestamp)
{
    uint32_t burtcOverflowCounter = wallclock_get_overflowcnt();

    //这里，不能改变burtc的cnt值
    uint32_t burtcStartTime = set_timestamp;
    burtcStartTime -= (BURTC_CounterGet() / COUNTS_PER_SEC);
    burtcStartTime -= burtcOverflowCounter * overflow_interval;
    if ( overflow_interval_r != 0 )
    {
        burtcStartTime -= burtcOverflowCounter * overflow_interval_r / COUNTS_PER_SEC;
    }

    //更新开始时间戳，并保存到备份域
    wallclock_set_timebase(burtcStartTime);
    LOG(LEVEL_DEBUG, "settime TimeBase=%d", wallclock_get_timebase());
    clock_backup();
}

/***************************************************************************//**
 * @brief hal_burtc_over_flow_irq_process
 ******************************************************************************/
static void hal_burtc_over_flow_irq_process(void)
{
    uint32_t overflowcnt = wallclock_get_overflowcnt();

    //更新溢出值
    wallclock_set_overflowcnt(++overflowcnt);

    //保存到备份域
    clock_backup();
}
#endif

/***************************************************************************//**
 * @brief hal_burtc_init
 ******************************************************************************/
Ecode_t hal_burtc_init(void)
{
    if(true == hal_burtc_initialized)
    {
        return ECODE_OK;
    }
    hal_burtc_initialized = true;
    LOG(LEVEL_DEBUG, "hal burtc init");

    // Ensure LE modules are clocked.
    CMU_ClockEnable(cmuClock_CORELE, true);

    //Enable LFACLK in CMU (will also enable oscillator if not enabled).
#if (HAL_USE_LFXO == 0)
    CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFRCO);
#else
    CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFXO);
#endif

    //Release reset for backup domain
    RMU_ResetControl(rmuResetBU, rmuResetModeClear);

#if defined(EMDRV_RTCDRV_WALLCLOCK_CONFIG)
    //配置backup domain
    budSetup();

    //检查是否是从备份域恢复
    clock_Init();
#else
    //配置burtc
    burtcSetup();

    //Start BURTC
    BURTC_Enable(true);

    //计数清零
    BURTC_CounterReset();
#endif

    //Disable RTC/RTCC interrupt generation.
    BURTC_IntDisable(_BURTC_IF_MASK);
    BURTC_IntClear(_BURTC_IF_MASK);

    //_by_pbh add macro
    NVIC_SetPriority(BURTC_IRQn, BURTC_IRQ_PRIORITY);

    //Clear and then enable RTC interrupts in NVIC.
    NVIC_ClearPendingIRQ(BURTC_IRQn);
    NVIC_EnableIRQ(BURTC_IRQn);

    // Enable overflow interrupt for wallclock.
    BURTC_IntEnable(BURTC_IF_OF);

    return ECODE_OK;
}

/***************************************************************************//**
 * @brief hal_burtc_deinit
 ******************************************************************************/
Ecode_t hal_burtc_deinit(void)
{
    // Disable and clear all interrupt sources.
    NVIC_DisableIRQ(BURTC_IRQn);
    BURTC_IntDisable(_BURTC_IF_MASK);
    BURTC_IntClear(_BURTC_IF_MASK);
    NVIC_ClearPendingIRQ(BURTC_IRQn);

    // Disable BURTC module and its clock.
    BURTC_Enable(false);

    // Mark the driver as uninitialized.
    hal_burtc_initialized = false;

    return ECODE_OK;
}

#if defined(FM_INNER_BURTC)                   \
 && (APP_TIMER_DRV_BURTC != APP_TIMER_DRV_SRC)
/***************************************************************************//**
 * @brief BURTC_IRQHandler
 ******************************************************************************/
void BURTC_IRQHandler(void)
{
    uint32_t flags;

    INT_Disable();

    flags = BURTC_IntGet();

    if(flags & RTC_OF_INT)
    {
        BURTC_IntClear(RTC_OF_INT);

        //执行wall_clock溢出处理函数
        hal_burtc_over_flow_irq_process();
    }

    INT_Enable();
}
#endif

