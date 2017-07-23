#ifndef __GLOBAL_H
#define __GLOBAL_H

#include "common_string.h"

//所有task的头文件
#include "sys_task.h"

#include "app_schedule.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(BSP_A)
#include "global_A.h"
#endif


#ifdef __cplusplus
}
#endif

#endif
