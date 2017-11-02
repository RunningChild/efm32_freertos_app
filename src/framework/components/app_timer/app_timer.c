#include "app_timer.h"

#include "log.h"

#if(APP_TIMER_DRV_FREERTOS == APP_TIMER_DRV_SRC)
#include "app_timer_drv_freertos.h"
#define USE_FREERTOS_SOFT_TIMERS
#endif

static RTCDRV_TimerID_t init_empty_timer_id;

/*******************************************************************************
 * @brief 软件定时器初始化：app_timer_init
 * note : timer个数，修改 rtcdrv_config.h 宏定义#define EMDRV_RTCDRV_NUM_TIMERS
 ******************************************************************************/
void app_timer_init(void)
{
    uint32_t error_code = APP_SUCCESS;

    // Initialization of RTCDRV driver
#if (APP_TIMER_DRV_BURTC == APP_TIMER_DRV_SRC)
#warning "app_timer_drv use burtc"
    error_code = RTCDRV_Init(hal_burtc_init);
#elif (APP_TIMER_DRV_RTC == APP_TIMER_DRV_SRC)
#warning "app_timer_drv use rtc"
    error_code = RTCDRV_Init(hal_rtc_init);
#elif (APP_TIMER_DRV_LETIMER == APP_TIMER_DRV_SRC)
#warning "app_timer_drv use letimer"
    error_code = RTCDRV_Init(hal_letimer_init);
#elif defined(USE_FREERTOS_SOFT_TIMERS)
#warning "app_timer_drv use freertos soft_timers"
#endif
    APP_ERROR_CHECK_BOOL(error_code == APP_SUCCESS);

    //初始化建立一个空的timer，该句柄创建时，始终返回0
    //该空的timer只会调用app_timer_create一个函数接口
    app_timer_create(&init_empty_timer_id);
}

/*******************************************************************************
 * @brief app_timer_create
 ******************************************************************************/
bool app_timer_check_running(RTCDRV_TimerID_t timer_id)
{
#if !defined(USE_FREERTOS_SOFT_TIMERS)
    return RTCDRV_timer_check_running(timer_id);
#else
    return app_timer_drv_check_running(timer_id);
#endif
}

/*******************************************************************************
 * @brief app_timer_create
 ******************************************************************************/
void app_timer_create(RTCDRV_TimerID_t *id)
{
#if !defined(USE_FREERTOS_SOFT_TIMERS)
    uint32_t error_code;

    // Reserve a timer
    error_code = RTCDRV_AllocateTimer(id);
    APP_ERROR_CHECK_BOOL(error_code == APP_SUCCESS);
#else
    app_timer_drv_create(id);
#endif
}

/*******************************************************************************
 * @brief app_timer_stop
 ******************************************************************************/
void app_timer_stop(RTCDRV_TimerID_t id)
{
#if !defined(USE_FREERTOS_SOFT_TIMERS)
    uint32_t error_code;

    APP_ERROR_CHECK_BOOL(id != 0);

    error_code = RTCDRV_StopTimer(id);
    APP_ERROR_CHECK_BOOL(error_code == APP_SUCCESS);
#else
    app_timer_drv_stop(id);
#endif
}

/*******************************************************************************
 * @brief app_timer_restart
 ******************************************************************************/
void app_timer_restart(RTCDRV_TimerID_t id,
                        RTCDRV_TimerType_t type,
                        uint32_t timeout,
                        RTCDRV_Callback_t callback,
                        void *user)
{
#if !defined(USE_FREERTOS_SOFT_TIMERS)
    uint32_t error_code;

    APP_ERROR_CHECK_BOOL(id != 0);

    error_code = RTCDRV_StopTimer(id);
    APP_ERROR_CHECK_BOOL(error_code == APP_SUCCESS);

    error_code = RTCDRV_StartTimer(id, type, timeout, callback, user);
    APP_ERROR_CHECK_BOOL(error_code == APP_SUCCESS);
#else
    app_timer_drv_restart(id, type, timeout, callback, user);
#endif
}

/******************************************************************************
 * @brief app_timer_get_compare_second
 *****************************************************************************/
uint32_t app_timer_get_compare_second(const uint32_t s_1, const uint32_t s_2)
{
    return (s_2 - s_1);
}

/******************************************************************************
 * @brief app_timer_get_compare_millisecond
 *****************************************************************************/
uint32_t app_timer_get_compare_millisecond(const uint32_t ms_1, const uint32_t ms_2)
{
    return (ms_2 - ms_1);
}

