#ifndef __SYS_TASK_H
#define __SYS_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include "string.h"

#include "app_schedule.h"

typedef enum {
    MSG_POWER_ON_RESET_LOGIC            = 1,
}sys_msg_type;

extern tm_task sys_task;

//对外函数接口
void sys_shutdown_action(void);
void sys_task_init(void);
void sys_task_on_msg(tm_msg *msg, uint16_t msg_size);

#ifdef __cplusplus
}
#endif

#endif


