#ifndef __WALL_CLOCK_TIMER_H
#define __WALL_CLOCK_TIMER_H

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include "string.h"

#include "app_error.h"

#include "calendar.h"

#ifdef __cplusplus
extern "C" {
#endif

extern timer_hander_callback_t wall_clock_timeout_hander;


//外部函数接口
void wall_clock_timer_restart(uint32_t ms_interval);
void wall_clock_timer_init(void);

#ifdef __cplusplus
}
#endif

#endif
