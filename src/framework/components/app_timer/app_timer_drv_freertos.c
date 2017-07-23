#include "app_timer_drv_freertos.h"

#include "log.h"
#include "timers.h"
#include "calendar.h"

#if (APP_TIMER_DRV_FREERTOS == APP_TIMER_DRV_SRC)
/******************************************************************************
                        操作系统tick定时器实现偏移秒数和毫秒数
 *****************************************************************************/

/*******************************************************************************
 * @brief app_timer_get_second
 ******************************************************************************/
uint32_t app_timer_get_second(void)
{
    return get_second_wallclock();
}

/*******************************************************************************
 * @brief app_timer_get_millisecond
 ******************************************************************************/
uint32_t app_timer_get_millisecond(void)
{
    //无法实现
}
#endif

/******************************************************************************
                        操作系统soft_timers实现软件定时器
 *****************************************************************************/
static volatile TimerHandle_t     timer[EMDRV_RTCDRV_NUM_TIMERS] = {NULL, };
static volatile RTCDRV_Callback_t timeout_callback[EMDRV_RTCDRV_NUM_TIMERS] = {NULL, };
static volatile bool              timer_is_running[EMDRV_RTCDRV_NUM_TIMERS] = {false, };
static uint16_t                   timer_count = 0;

/*******************************************************************************
 * @brief empty_timer_callback
 ******************************************************************************/
static void empty_timer_callback(TimerHandle_t xtimer)
{
    uint32_t id = (uint32_t)pvTimerGetTimerID(xtimer);

    if(NULL != timeout_callback[id])
    {
        timeout_callback[id](id, NULL);
    }

    //当前定时器超时运行结束
    timer_is_running[id] = false;
}

/*******************************************************************************
 * @brief app_timer_drv_check_running
 ******************************************************************************/
bool app_timer_drv_check_running(RTCDRV_TimerID_t timer_id)
{
    //warnning:xTimerIsTimerActive不能在中断里头调用

    return timer_is_running[timer_id];
}

/*******************************************************************************
 * @brief app_timer_drv_create
 * note : timer个数，
 * 修改 rtcdrv_config.h  宏定义#define EMDRV_RTCDRV_NUM_TIMERS
 * 修改 FreeRTOSConfig.h 宏定义#configTIMER_QUEUE_LENGTH
 ******************************************************************************/
void app_timer_drv_create(RTCDRV_TimerID_t *id)
{
    //warnning:xTimerCreate不能用于中断调用

    timer[timer_count] = xTimerCreate(NULL, pdMS_TO_TICKS(1000), pdFALSE, (void *)(timer_count), empty_timer_callback);

    //当前定时器初始化停止运行
    timer_is_running[timer_count] = false;

    *id = timer_count;

    timer_count++;
    APP_ERROR_CHECK_BOOL(EMDRV_RTCDRV_NUM_TIMERS >= timer_count);
    APP_ERROR_CHECK_BOOL(NULL != timer[timer_count - 1]);
}

/*******************************************************************************
 * @brief app_timer_drv_stop
 ******************************************************************************/
void app_timer_drv_stop(RTCDRV_TimerID_t id)
{
    APP_ERROR_CHECK_BOOL(id != 0);

    portBASE_TYPE xStatus;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    volatile uint32_t isr_vector_num = (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk);
    portTickType xTicksToWait = 500/portTICK_RATE_MS;


    if (0 == isr_vector_num)
    {
        xStatus = xTimerStop(timer[id], xTicksToWait);
    }
    else
    {
        xStatus = xTimerStopFromISR(timer[id], &xHigherPriorityTaskWoken);
    }
    APP_ERROR_CHECK_BOOL(xStatus == pdPASS);

    //当前定时器停止运行
    timer_is_running[id] = false;
}

/*******************************************************************************
 * @brief app_timer_drv_restart
 ******************************************************************************/
void app_timer_drv_restart(RTCDRV_TimerID_t id,
                        RTCDRV_TimerType_t type,
                        uint32_t timeout,
                        RTCDRV_Callback_t callback,
                        void *user)
{
    APP_ERROR_CHECK_BOOL(id != 0);

    //warnning:不使用参数type和user
    portBASE_TYPE xStatus;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    volatile uint32_t isr_vector_num = (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk);
    portTickType xTicksToWait = 500/portTICK_RATE_MS;


    //停止定时器
    app_timer_drv_stop(id);

    //重新设置超时函数
    timeout_callback[id] = callback;

    if(0 == isr_vector_num)
    {
        xStatus = xTimerChangePeriod(timer[id], pdMS_TO_TICKS(timeout), xTicksToWait);
    }
    else
    {
        xStatus = xTimerChangePeriodFromISR(timer[id], pdMS_TO_TICKS(timeout), &xHigherPriorityTaskWoken);
    }
    APP_ERROR_CHECK_BOOL(xStatus == pdPASS);

    //重启定时器
    if(0 == isr_vector_num)
    {
        xStatus = xTimerStart(timer[id], xTicksToWait);
    }
    else
    {
        xStatus = xTimerStartFromISR(timer[id], &xHigherPriorityTaskWoken);
    }
    APP_ERROR_CHECK_BOOL(xStatus == pdPASS);

    //当前定时器开始运行
    timer_is_running[id] = true;
}

