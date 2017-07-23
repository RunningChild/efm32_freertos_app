#include <stdio.h>
#include <stdlib.h>

#include "em_int.h"

#include "platform.h"

#include "softwdt.h"
#include "app_schedule.h"
#include "app_error.h"

#include "thread_manager.h"

#include "log.h"

//APP任务事件调度 功能 实现 相关
typedef struct
{
    on_msg_func handler;                                    /**< Pointer to event handler to receive the event. */
    uint16_t event_data_size;                               /**< Size of event data. */
    tm_msg event_data;
} app_evt_schedule_t;

#define APP_EVENT_SCHED_SIZE                    (72)

//APP任务事件调度线程相关
#define APP_EVENT_SCHED_STACK_SIZE_FOR_TASK_HIGH   (configMINIMAL_STACK_SIZE + 1000) //注意设置的堆栈大小
#define APP_EVENT_SCHED_STACK_SIZE_FOR_TASK_MID    (configMINIMAL_STACK_SIZE + 800) //注意设置的堆栈大小
#define APP_EVENT_SCHED_STACK_SIZE_FOR_TASK_LOW    (configMINIMAL_STACK_SIZE + 1300) //注意设置的堆栈大小
#define APP_EVENT_SCHED_TASK_PRIORITY_HIGH         (tskIDLE_PRIORITY + configMAX_PRIORITIES - 3)
#define APP_EVENT_SCHED_TASK_PRIORITY_MID          (tskIDLE_PRIORITY + configMAX_PRIORITIES - 4)
#define APP_EVENT_SCHED_TASK_PRIORITY_LOW          (tskIDLE_PRIORITY + configMAX_PRIORITIES - 5)

#define APP_EVENT_SCHED_SOFT_WDT_MONITOR_TIME      (30)

//优先级任务队列
static xQueueHandle app_evt_sched_Queue_high;
static xQueueHandle app_evt_sched_Queue_mid;
static xQueueHandle app_evt_sched_Queue_low;
static req_soft_wdt_t *app_evt_sched_soft_wdt_handle_high;
static req_soft_wdt_t *app_evt_sched_soft_wdt_handle_mid;
static req_soft_wdt_t *app_evt_sched_soft_wdt_handle_low;

//外部接口实现
#define APP_TASK_LIST_MAX_LEN                   (20)
#define APP_TASK_BASE_ID                        (1)

static uint8_t task_count = APP_TASK_BASE_ID;

/******************************************************************************
 * @brief  print_evt_sched_info
 *****************************************************************************/
static void print_evt_sched_info(xQueueHandle * handle)
{
    if(&app_evt_sched_Queue_high == handle)
    {
        LOG(LEVEL_DEBUG, "[app_schedule] high=0x%x", xTaskGetCurrentTaskHandle());
    }
    else if(&app_evt_sched_Queue_mid == handle)
    {
        LOG(LEVEL_DEBUG, "[app_schedule] mid");
    }
    else if(&app_evt_sched_Queue_low == handle)
    {
        LOG(LEVEL_DEBUG, "[app_schedule] low");
    }
    else
    {
        LOG(LEVEL_DEBUG, "[app_schedule] error");
    }
}

/******************************************************************************
 * @brief  app_evt_sched_softwdt_feed
 *****************************************************************************/
static void app_evt_sched_softwdt_feed(xQueueHandle * handle)
{
    //软件看门狗喂狗
    if(&app_evt_sched_Queue_high == handle)
    {
        softwdt_feed(app_evt_sched_soft_wdt_handle_high);
    }
    else if(&app_evt_sched_Queue_mid == handle)
    {
        softwdt_feed(app_evt_sched_soft_wdt_handle_mid);
    }
    else if(&app_evt_sched_Queue_low == handle)
    {
        softwdt_feed(app_evt_sched_soft_wdt_handle_low);
    }
    else
    {
        LOG(LEVEL_DEBUG, "[app_schedule] error");
    }
}

/******************************************************************************
 * @brief  app_evt_sched_thread
 *****************************************************************************/
static void app_evt_sched_thread(void *pvParameters)
{
    portBASE_TYPE xStatus;
    const portTickType xTicksToWait = 1000 / portTICK_RATE_MS;
    xQueueHandle *sched_queue_handle = (xQueueHandle *) pvParameters;

    app_evt_schedule_t send_evt_sched_unit;
    tm_msg *msg = NULL;
    uint16_t msg_size = 0;

    for(;;)
    {
        app_evt_sched_softwdt_feed(sched_queue_handle);

        xStatus = xQueueReceive(*sched_queue_handle, &send_evt_sched_unit, xTicksToWait);

        if(xStatus == pdPASS)
        {
//            print_evt_sched_info(sched_queue_handle);//打印事件调度信息

            //当前有事件任务，且管理线程处于睡眠态，则唤醒管理线程
            if(false == manager_get_thread_active_flag())
            {
                manager_sleep_sem_send();
            }

            msg = &(send_evt_sched_unit.event_data);
            msg_size = send_evt_sched_unit.event_data_size;

            if((0 == msg->type) && (NULL != msg->on_task_implement))
            {
                //task的回调不为空
                msg->on_task_implement(msg->data, msg->data_length);

                if(NULL != msg->data)
                {
                    free_overwrite(msg->data);
                    msg->data = NULL;
                }
            }
            else
            {
                //task的type不为空
                if(NULL != send_evt_sched_unit.handler)
                {
                    send_evt_sched_unit.handler(msg, msg_size);
                }
            }

            msg->type = 0;
            msg->on_task_implement = NULL;
            msg->data_length = 0;
        }
    }
}

/******************************************************************************
 * @brief  tm_send_message
 *****************************************************************************/
static int16_t tm_send_message(tm_task * target_task, tm_msg msg)
{
    if((true == target_task->is_used) && (APP_TASK_BASE_ID <= target_task->task_id))
    {
        //target_task使能，且task_id合法
        xQueueHandle sched_queue_handle;
        app_evt_schedule_t send_evt_sched_unit;

        if(APP_TASK_HIGH_PRIORITY == target_task->priority)
        {
            sched_queue_handle = app_evt_sched_Queue_high;
        }
        else if(APP_TASK_MID_PRIORITY == target_task->priority)
        {
            sched_queue_handle = app_evt_sched_Queue_mid;
        }
        else
        {
            sched_queue_handle = app_evt_sched_Queue_low;
        }

        send_evt_sched_unit.handler = target_task->on_msg_implement;
        send_evt_sched_unit.event_data_size = sizeof(tm_msg);
        send_evt_sched_unit.event_data = msg;

        //发送数据到调度队列
        portBASE_TYPE xStatus = xQueueSendToBack_OverWrite(sched_queue_handle,
                                                           &send_evt_sched_unit,
                                                           (200 / portTICK_RATE_MS));
        APP_ERROR_CHECK_BOOL(xStatus == pdPASS);
    }
    else
    {
        LOG(LEVEL_DEBUG, "\033[1;34;40m[app] %s not init\033[0m", target_task->task_name);

        //target_task失能，或task_id不合法
        if(NULL != msg.data)
        {
            free_overwrite(msg.data);
            msg.data = NULL;
        }
    }

    return 0;
}

/******************************************************************************
 * @brief  send_task_type
 *****************************************************************************/
void send_task_type(tm_task * target_task, uint8_t type, uint8_t * data, uint16_t data_length)
{
    tm_msg nxt_msg;

    nxt_msg.type = type;
    nxt_msg.on_task_implement = NULL;
    nxt_msg.data_length = data_length;
    nxt_msg.data = data;

    tm_send_message(target_task, nxt_msg);
}

/******************************************************************************
 * @brief  send_task_cb
 *****************************************************************************/
void send_task_cb(tm_task * target_task, on_task_func on_task_implement, uint8_t * data, uint16_t data_length)
{
    tm_msg nxt_msg;

    nxt_msg.type = 0;
    nxt_msg.on_task_implement = on_task_implement;
    nxt_msg.data_length = data_length;
    nxt_msg.data = data;

    tm_send_message(target_task, nxt_msg);
}

/******************************************************************************
 * @brief  send_task_type_rt_safe
 *****************************************************************************/
void send_task_type_rt_safe(tm_task * target_task, uint8_t type, uint8_t * data, uint16_t data_length)
{
    tm_task rt_task;

    memset(&rt_task, 0, sizeof(tm_task));
    memcpy(&rt_task, target_task, sizeof(tm_task));
    rt_task.priority = APP_TASK_HIGH_PRIORITY;

    send_task_type(&rt_task, type, data, data_length);
}

/******************************************************************************
 * @brief  app_task_sched_init
 *****************************************************************************/
int16_t app_task_sched_init(void)
{
    /* 创建APP任务事件优先级队列 */
    app_evt_sched_Queue_high = xQueueCreate(APP_EVENT_SCHED_SIZE * 1 / 12, sizeof(app_evt_schedule_t));
    APP_ERROR_CHECK_BOOL(app_evt_sched_Queue_high != NULL);

    app_evt_sched_Queue_mid = xQueueCreate(APP_EVENT_SCHED_SIZE * 6 / 12, sizeof(app_evt_schedule_t));
    APP_ERROR_CHECK_BOOL(app_evt_sched_Queue_mid != NULL);

    app_evt_sched_Queue_low = xQueueCreate(APP_EVENT_SCHED_SIZE * 5 / 12, sizeof(app_evt_schedule_t));
    APP_ERROR_CHECK_BOOL(app_evt_sched_Queue_low != NULL);

    /* 创建APP任务事件优先级调度线程 */
    portBASE_TYPE xStatus;
    xStatus =
        xTaskCreate(app_evt_sched_thread, "evt_h_1",
                    APP_EVENT_SCHED_STACK_SIZE_FOR_TASK_HIGH, &app_evt_sched_Queue_high, APP_EVENT_SCHED_TASK_PRIORITY_HIGH, NULL);
    APP_ERROR_CHECK_BOOL(xStatus == pdTRUE);

//    xStatus = xTaskCreate( app_evt_sched_thread, "evt_h_2", APP_EVENT_SCHED_STACK_SIZE_FOR_TASK_HIGH, &app_evt_sched_Queue_high, APP_EVENT_SCHED_TASK_PRIORITY_HIGH, NULL );
//    APP_ERROR_CHECK_BOOL(xStatus == pdTRUE);

    xStatus =
        xTaskCreate(app_evt_sched_thread, "evt_m_1",
                    APP_EVENT_SCHED_STACK_SIZE_FOR_TASK_MID, &app_evt_sched_Queue_mid, APP_EVENT_SCHED_TASK_PRIORITY_MID, NULL);
    APP_ERROR_CHECK_BOOL(xStatus == pdTRUE);

    xStatus =
        xTaskCreate(app_evt_sched_thread, "evt_l_1",
                    APP_EVENT_SCHED_STACK_SIZE_FOR_TASK_LOW, &app_evt_sched_Queue_low, APP_EVENT_SCHED_TASK_PRIORITY_LOW, NULL);
    APP_ERROR_CHECK_BOOL(xStatus == pdTRUE);

    // 分配软件看门狗
    softwdt_open(&app_evt_sched_soft_wdt_handle_high, APP_EVENT_SCHED_SOFT_WDT_MONITOR_TIME, "evt_h");
    softwdt_open(&app_evt_sched_soft_wdt_handle_mid, APP_EVENT_SCHED_SOFT_WDT_MONITOR_TIME, "evt_m");
    softwdt_open(&app_evt_sched_soft_wdt_handle_low, APP_EVENT_SCHED_SOFT_WDT_MONITOR_TIME, "evt_l");

    return 0;
}

/******************************************************************************
 * @brief app_task_open
 *****************************************************************************/
void app_task_open(tm_task * target_task, app_task_priority_t priority, on_msg_func on_msg_implement)
{
    if(true == target_task->is_used)
    {
        LOG(LEVEL_DEBUG, "[app] error, task_id=%d used", target_task->task_id);
        return;
    }

    memset(target_task, 0, sizeof(tm_task));

    target_task->task_id = task_count;
    target_task->priority = priority;
    target_task->on_msg_implement = on_msg_implement;
    target_task->is_used = true;
    LOG(LEVEL_DEBUG, "[app] task_id = %d", task_count);

    APP_ERROR_CHECK_BOOL(++task_count <= APP_TASK_LIST_MAX_LEN);
}
