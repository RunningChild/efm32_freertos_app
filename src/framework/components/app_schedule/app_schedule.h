#ifndef APP_SCHEDULER_H__
#define APP_SCHEDULER_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include "string.h"
#include "stdbool.h"

    typedef void (*on_task_func) (uint8_t * data, uint16_t data_length);
    typedef struct
    {
        uint8_t type;
        on_task_func on_task_implement;
        uint8_t *data;
        uint16_t data_length;
    } tm_msg;

    typedef void (*on_msg_func) (tm_msg * msg, uint16_t msg_size);

//app evt task priority
    typedef enum
    {
        APP_TASK_HIGH_PRIORITY = 1, //内部使用
        APP_TASK_MID_PRIORITY = 2,
        APP_TASK_LOW_PRIORITY = 3,
    } app_task_priority_t;

    typedef struct
    {
        uint16_t task_id;
        bool is_used;
        app_task_priority_t priority;
        on_msg_func on_msg_implement;
        char *task_name;
    } tm_task;

//定义：函数指针
    typedef void (*task_logic_init) (void);

#define APP_TASK_INIT(name)                                 \
{                                                           \
    .task_id            = 0,                                \
    .is_used            = false,                            \
    .priority           = APP_TASK_LOW_PRIORITY,            \
    .on_msg_implement   = NULL,                             \
    .task_name          = #name                             \
}

//对外函数接口
    void send_task_cb(tm_task * target_task, on_task_func on_task_implement, uint8_t * data, uint16_t data_length);
    void send_task_type(tm_task * target_task, uint8_t type, uint8_t * data, uint16_t data_length);
    void send_task_type_rt_safe(tm_task * target_task, uint8_t type, uint8_t * data, uint16_t data_length);
    int16_t app_task_sched_init(void);
    void app_task_open(tm_task * target_task, app_task_priority_t priority, on_msg_func on_msg_implement);

#ifdef __cplusplus
}
#endif

#endif
