#ifndef __PLATFORM_H
#define __PLATFORM_H

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include "string.h"
#include "app_error.h"

//以下free rtos相关
#include <stdlib.h>
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "croutine.h"

#include "em_cmu.h"    //中断上下文检查

#ifdef __cplusplus
extern "C" {
#endif

//以下重写动态内存分配函数接口
/***********************************************************************************************
 * 重写xQueueSendToBack和xQueueSendToBackFromISR接口
 * xQueueSendToBack:
 * xQueueSendToBackFromISR: 第三个参数:阻塞超时时间 – 当队列满时，任务转入阻塞状态以等待队列空间有效
 * note：SCB->ICSR：                返回中断向量号
 **********************************************************************************************/
BaseType_t xQueueSendToBack_OverWrite(
                                       QueueHandle_t    xQueue,
                                       const void       *pvItemToQueue,
                                       TickType_t       xTicksToWait
                                     );

/***********************************************************************************************
 * mem_manager_print
 **********************************************************************************************/
void mem_manager_print(void);

/***********************************************************************************************
 * 重写pvPortMalloc和malloc接口
 * size_t bytes:            动态申请的内存字节数
 **********************************************************************************************/
void * malloc_overwrite(size_t bytes);

/***********************************************************************************************
 * 重写vPortFree和free接口
 * void *ptr:            动态申请的内存地址
 **********************************************************************************************/
void free_overwrite(void *ptr);

/******************************************************************************
 * @brief  sleep_overwrite
 *****************************************************************************/
void sleep_overwrite(uint32_t ms);

/*******************************************************************************
 * @brief get_uptime_second
 ******************************************************************************/
uint32_t get_uptime_second(void);


#ifdef __cplusplus
}
#endif

#endif

