#ifndef APP_TIMER_FREERTOS_H
#define APP_TIMER_FREERTOS_H

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include "string.h"
#include "app_error.h"

#include "app_timer_drv.h"

#ifdef __cplusplus
extern "C" {
#endif

//外部函数接口
bool app_timer_drv_check_running(RTCDRV_TimerID_t timer_id);
void app_timer_drv_create(RTCDRV_TimerID_t *id);
void app_timer_drv_stop(RTCDRV_TimerID_t id);
void app_timer_drv_restart(RTCDRV_TimerID_t id,
                        RTCDRV_TimerType_t type,
                        uint32_t timeout,
                        RTCDRV_Callback_t callback,
                        void *user);
#ifdef __cplusplus
}
#endif


#endif
