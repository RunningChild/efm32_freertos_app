#include "app_manager.h"
#include "event.h"
#include "app_timer.h"

#include "global.h"

#include "module_config.h"


static RTCDRV_TimerID_t event_period_timer_id;
static timer_hander_callback_t event_period_timeout_hander = NULL;

/*******************************************************************************
 * @brief event_period_timeout_cb
 ******************************************************************************/
static void event_period_timeout_cb(RTCDRV_TimerID_t id, void *user)
{
    if(NULL != event_period_timeout_hander)
    {
        event_period_timeout_hander();
    }
}

/*******************************************************************************
 * @brief event_period_timer_restart
 ******************************************************************************/
static void event_period_timer_restart(uint32_t ms_interval)
{
    app_timer_restart(event_period_timer_id, rtcdrvTimerTypeOneshot, ms_interval, event_period_timeout_cb, NULL);
}

/*******************************************************************************
 * @brief event_period_timer_init
 ******************************************************************************/
static void event_period_timer_init(void)
{
    app_timer_create(&event_period_timer_id);
}

/******************************************************************************
 * @brief  app_task_init
 *****************************************************************************/
void app_task_init(void)
{
    //应用任务公用组件初始化
    event_init(event_period_timer_init, event_period_timer_restart, &event_period_timeout_hander);

    //应用任务初始化
#if defined(AM_SYS_TASK)
    sys_task_init();
#endif
}


