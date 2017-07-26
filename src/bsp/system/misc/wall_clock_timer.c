#include "wall_clock_timer.h"
#include "app_timer.h"

#include "calendar.h"
#include "datetime.h"

#include "log.h"

static uint32_t wall_clock_ms_interval_cache;

/*******************************************************************************
 ********************************万年历实现*************************************
 ******************************************************************************/

timer_hander_callback_t wall_clock_timeout_hander = NULL;

static RTCDRV_TimerID_t wall_clock_timer_id;

/*******************************************************************************
 * @brief 万年历定时器的timeout hander
 ******************************************************************************/
static void wall_clock_timer_hander(RTCDRV_TimerID_t id, void *user)
{
    if(wall_clock_timeout_hander != NULL)
    {
        wall_clock_timeout_hander();
    }

    UTCTime cur_second_count = get_second_counter();
    uint32_t cur_second_in_hour = get_second_in_hour();
    uint32_t cur_hour_count = get_hour_count();

    LOG(LEVEL_DEBUG, "soft second=%d, hal second=%d", cur_second_count, get_uptime_second());

    wall_clock_timer_restart(wall_clock_ms_interval_cache);
}

/*******************************************************************************
 * @brief 万年历定时器初始化
 ******************************************************************************/
void wall_clock_timer_init(void)
{
    app_timer_create(&wall_clock_timer_id);
}

/*******************************************************************************
 * @brief 定时器的重启
 ******************************************************************************/
void wall_clock_timer_restart(uint32_t ms_interval)
{
    app_timer_restart(wall_clock_timer_id, rtcdrvTimerTypeOneshot, ms_interval, wall_clock_timer_hander, NULL);
    wall_clock_ms_interval_cache = ms_interval;
}

