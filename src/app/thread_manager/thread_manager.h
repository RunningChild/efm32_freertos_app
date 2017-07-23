#ifndef __THREAD_MANAGER_H
#define __THREAD_MANAGER_H

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "string.h"

#ifdef __cplusplus
extern "C" {
#endif


//外部函数接口
void manager_power_on_logic(void);
void manager_EFM32_sleep(void);
void manager_EFM32_sys_off(void);
bool manager_get_sys_off(void);
void manager_sleep_sem_send(void);
bool manager_get_thread_active_flag(void);

void manager_thread_run_init(void);
void manager_init(void);

void period_init(void);

bool check_monitor_thread_running_flag(void);
void monitor_init(void);


#ifdef __cplusplus
}
#endif

#endif
