#include "calendar.h"

#include "log.h"

#define LOG_BUF_MAX_SIZE            (512)

static uart_tx              log_putstring;
static uart_shutdown        log_shutdown;

#if (LOG_THREAD_SAFE == 1)
#include "softwdt.h"
#include "thread_manager.h"

//日志线程相关
#define LOG_STACK_SIZE_FOR_TASK    (configMINIMAL_STACK_SIZE + 100)
#define LOG_TASK_PRIORITY          (tskIDLE_PRIORITY + configMAX_PRIORITIES - 2) //优先级  > 所有含有打印任务线程的优先级
#define LOG_SOFT_WDT_MONITOR_TIME  (15)

//日志任务队列
#define LOG_QUEUE_SIZE             (50)                 //在这里尽量设置较多buffer，防止队列溢出
typedef struct
{
    uint8_t size;
    char*   buffer;
}log_queue_buffer_t;

static xQueueHandle log_Queue;
static req_soft_wdt_t *log_soft_wdt_handle;

static void log_thread( void *pvParameters )
{
    portBASE_TYPE xStatus;
    const portTickType xTicksToWait = 1000 / portTICK_RATE_MS;

    log_queue_buffer_t logbuf;

    for( ;; )
    {
        xStatus = xQueueReceive( log_Queue, &logbuf, xTicksToWait );

        softwdt_feed(log_soft_wdt_handle);//软件看门狗喂狗

        if( xStatus == pdPASS )
        {
            if(logbuf.buffer != NULL)
            {
                uint32_t ret = log_putstring((uint8_t *)logbuf.buffer, logbuf.size);
                APP_ERROR_CHECK_BOOL(ret == APP_SUCCESS);

                free_overwrite(logbuf.buffer);//vPortFree//free
                logbuf.buffer = NULL;
            }
        }
    }
}

#endif

void log_init(uart_init log_uart_init, uart_tx log_uart_tx, uart_shutdown log_uart_shutdown)
{
    uint32_t ret = log_uart_init(NULL);
    APP_ERROR_CHECK_BOOL(ret == APP_SUCCESS);

    log_putstring = log_uart_tx;

    log_shutdown = log_uart_shutdown;

#if (LOG_THREAD_SAFE == 1)

    // 创建日志队列
    log_Queue = xQueueCreate( LOG_QUEUE_SIZE, sizeof(log_queue_buffer_t) );
    APP_ERROR_CHECK_BOOL(log_Queue != NULL);

    // 创建日志线程
    portBASE_TYPE xStatus;
    xStatus = xTaskCreate( log_thread, "log", LOG_STACK_SIZE_FOR_TASK, NULL, LOG_TASK_PRIORITY, NULL );
    APP_ERROR_CHECK_BOOL(xStatus == pdTRUE);

    // 分配软件看门狗
    softwdt_open(&log_soft_wdt_handle, LOG_SOFT_WDT_MONITOR_TIME, "log");
#endif
}

void __log(uint8_t level, /*const char * func, uint32_t line,*/ const char * restrict format, ...){
/*
    if( (LEVEL_SIMPLE_FORCE  <= level )
     && (level <= __LEVEL__) )
*/
    {
        char    str[LOG_BUF_MAX_SIZE];
        va_list ap;

        int cnt1 = 0;
        int cnt2 = 0;

        memset(str, 0, sizeof(str));

#if (LOG_TIME_PRINT == 1)
        if( (level != LEVEL_SIMPLE)
         && (level != LEVEL_RELEASE)
         && (level != LEVEL_SIMPLE_FORCE) )
        {
            UTCTimeStruct utc_time;
            get_wall_clock_time(&utc_time, false);
            cnt1 = snprintf(str, sizeof(str), "[%04d-%02d-%02d,%02d:%02d:%02d]:",
                                        utc_time.year, utc_time.month  , utc_time.day,
                                        utc_time.hour, utc_time.minute , utc_time.second
                                        );
        }
#endif

        va_start(ap, format);
        cnt2 = vsnprintf(&str[cnt1], sizeof(str), format, ap);
        va_end(ap);

        if(level != LEVEL_SIMPLE)
        {
            str[cnt1 + cnt2] = '\r';
            str[cnt1 + cnt2 + 1] = '\n';
        }

#if (LOG_THREAD_SAFE == 1)
        //监控线程还未运行，说明调度器还未运行，直接输出串口日志
        if(false == check_monitor_thread_running_flag())
        {
            level = LEVEL_FORCE;
        }

        if(level > LEVEL_FORCE)
        {
            log_queue_buffer_t logbuf;

            logbuf.size = cnt1 + cnt2 + 2;
            logbuf.buffer = (char *)malloc_overwrite(logbuf.size);//pvPortMalloc//malloc

            APP_ERROR_CHECK_BOOL(logbuf.buffer != NULL);
            memset(logbuf.buffer, 0, logbuf.size);
            memcpy(logbuf.buffer, str, logbuf.size);

            //发送数据到日志队列
            portBASE_TYPE xStatus = xQueueSendToBack_OverWrite(log_Queue, &logbuf, (200/portTICK_RATE_MS));
            APP_ERROR_CHECK_BOOL(xStatus == pdPASS);
        }
        else if((level == LEVEL_FORCE) || (level == LEVEL_SIMPLE_FORCE))
#endif
        {
            uint32_t ret = log_putstring((uint8_t *)str, cnt1 + cnt2 + 2);
            APP_ERROR_CHECK_BOOL(ret == APP_SUCCESS);
        }

    }
}

