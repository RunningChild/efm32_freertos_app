#include "thread_manager.h"

#include "softwdt.h"
#include "wdt.h"
#include "delay.h"

#include "sleep.h"
#include "log_cmd.h"

#include "global.h"
#include "log.h"

//管理线程相关
#define MANAGER_STACK_SIZE_FOR_TASK      (configMINIMAL_STACK_SIZE + 400)
#define MANAGER_TASK_PRIORITY            (tskIDLE_PRIORITY + configMAX_PRIORITIES - 5)
#define MANAGER_SOFT_WDT_MONITOR_TIME    (30)

static req_soft_wdt_t *manager_soft_wdt_handle;
static SemaphoreHandle_t manager_sleep_sem;
static bool manager_thread_is_active_flag = true;//活跃状态

static bool manager_sys_off_flag = false;//系统关机标志

/*******************************************************************************
 * @brief manager_power_on_logic
 * 接口不可以在中断中调用，且线程调用可能导致线程睡眠
 ******************************************************************************/
void manager_power_on_logic(void)
{
    if(true == manager_sys_off_flag)
    {
        return;
    }
}

/******************************************************************************
 * @brief  manager_is_all_peripherals_used
 *****************************************************************************/
static bool manager_is_all_peripherals_used(void)
{
    //预计EM2低功耗模式下:
    //将停止的外设有:msc、usart、uart、adc、aes、timer0/1/2/3
    //所以，需要定时 判断逻辑上是否正在使用以上外设
#if 0
    if((true == timer0_is_running())               /*timer0*/
    || (false == msc_is_idle())                 /*msc*/
    )
    {
        return true;
    }
#endif

    return false;
}

/******************************************************************************
 * @brief  manager_EFM32_sleep
 *****************************************************************************/
void manager_EFM32_sleep(void)
{
    //在这里再次检查是否使用高频外设，如使用则返回
    if(manager_is_all_peripherals_used())
    {
//        LOG(LEVEL_FORCE, "\r\n\033[1;36;40mEFM32_sleep\033[0m\r\n");
        delay_ms(2);
        return;
    }

    if(true == manager_sys_off_flag)
    {
        LOG_RELEASE(LEVEL_FORCE, "EM4");
        SLEEP_ForceSleepInEM4();
    }
    else
    {
        SLEEP_Sleep();
    }
}

/******************************************************************************
 * @brief  manager_EFM32_sys_off
 *****************************************************************************/
void manager_EFM32_sys_off(void)
{
    sys_shutdown_action();

    manager_sys_off_flag = true;
}

/*******************************************************************************
 * @brief manager_get_sys_off
 ******************************************************************************/
bool manager_get_sys_off(void)
{
    return manager_sys_off_flag;
}

/******************************************************************************
 * @brief  manager_sleep_sem_send
 *****************************************************************************/
void manager_sleep_sem_send(void)
{
    portBASE_TYPE xStatus;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if(false == (0 != (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk)))
    {
        xStatus = xSemaphoreGive(manager_sleep_sem);
    }
    else
    {
        xStatus = xSemaphoreGiveFromISR(manager_sleep_sem, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }

    if( xStatus != pdPASS )
    {
        LOG(LEVEL_MANAGER_SLEEP, "\033[1;36;40mmanager sleep sem send\033[0m");
    }
    else
    {
        LOG(LEVEL_MANAGER_SLEEP, "manager sleep sem send");
    }
}

/******************************************************************************
 * @brief  manager_get_thread_active_flag
 *****************************************************************************/
bool manager_get_thread_active_flag(void)
{
    return manager_thread_is_active_flag;
}

/******************************************************************************
 * @brief  manager_thread
 *****************************************************************************/
void manager_thread( void *pvParameters )
{
    //管理线程运行初始化
    manager_thread_run_init();

    portBASE_TYPE xStatus;
    const portTickType xTicksToWait = 1000 / portTICK_RATE_MS;

    for( ;; )
    {
        softwdt_feed(manager_soft_wdt_handle);//软件看门狗喂狗

        if(manager_is_all_peripherals_used())
        {
            delay_ms(50);
        }
        else
        {
            //管理线程处于睡眠状态
            manager_thread_is_active_flag = false;
            xStatus = xSemaphoreTake(manager_sleep_sem, xTicksToWait);
            if( xStatus == pdPASS )
            {
                LOG(LEVEL_MANAGER_SLEEP, "manager sleep sem take");
            }

            //管理线程处于活跃状态
            manager_thread_is_active_flag = true;
        }
    }
}

/******************************************************************************
 * @brief  manager_thread_run_init
 * 禁止调用导致线程睡眠的接口
 *****************************************************************************/
void manager_thread_run_init(void)
{
    //打印HFLK的频率
    LOG(LEVEL_DEBUG, "SystemHFClockGet() = %d", SystemHFClockGet());

    //打印各个中断优先级
//        LOG(LEVEL_DEBUG, "PriorityGroup %d", NVIC_GetPriorityGrouping());
    LOG(LEVEL_DEBUG, "BURTC_IRQn %d", NVIC_GetPriority(BURTC_IRQn));
    LOG(LEVEL_DEBUG, "RTC_IRQn %d", NVIC_GetPriority(RTC_IRQn));
    LOG_RELEASE(LEVEL_DEBUG, "before for!");

#if (1 == LOG_CMD_ENABLE)
    cmd_init();
#endif

    //开始执行上电业务逻辑---功耗管理，必须转由其他线程去执行
    send_task_type(&sys_task, MSG_POWER_ON_RESET_LOGIC, NULL, 0);
}

/******************************************************************************
 * @brief  manager_init
 *****************************************************************************/
void manager_init(void)
{
    //创建APP任务事件优先级调度线程
    portBASE_TYPE xStatus;
    xStatus = xTaskCreate( manager_thread, "manager", MANAGER_STACK_SIZE_FOR_TASK, NULL, MANAGER_TASK_PRIORITY, NULL );
    APP_ERROR_CHECK_BOOL(xStatus == pdTRUE);

    manager_sleep_sem = xSemaphoreCreateCounting(5, 0);

    //分配软件看门狗
    softwdt_open(&manager_soft_wdt_handle, MANAGER_SOFT_WDT_MONITOR_TIME, "manager");
}

//周期性线程相关
#define PERIOD_STACK_SIZE_FOR_TASK       (configMINIMAL_STACK_SIZE + 200)
#define PERIOD_TASK_PRIORITY             (tskIDLE_PRIORITY + configMAX_PRIORITIES - 4)
#define PERIOD_SOFT_WDT_MONITOR_TIME     (120)

static req_soft_wdt_t *period_soft_wdt_handle;

/******************************************************************************
 * @brief  period_thread
 *****************************************************************************/
static void period_thread( void *pvParameters )
{
    for(;;)
    {
        softwdt_feed(period_soft_wdt_handle);//软件看门狗喂狗
        sleep_overwrite(1*1000);

        mem_manager_print();
    }
}

/******************************************************************************
 * @brief  period_init
 *****************************************************************************/
void period_init(void)
{
    portBASE_TYPE xStatus;
    xStatus = xTaskCreate( period_thread, "period", PERIOD_STACK_SIZE_FOR_TASK, NULL, PERIOD_TASK_PRIORITY, NULL );
    APP_ERROR_CHECK_BOOL(xStatus == pdTRUE);

    // 分配软件看门狗
    softwdt_open(&period_soft_wdt_handle, PERIOD_SOFT_WDT_MONITOR_TIME, "period");
}


//监控线程初始化线程相关
#define MONITOR_STACK_SIZE_FOR_TASK      (configMINIMAL_STACK_SIZE + 50)
#define MONITOR_TASK_PRIORITY            (tskIDLE_PRIORITY + configMAX_PRIORITIES - 1)
#define FEED_WDOG_INTERVAL_MS            (4)

static bool is_monitor_thread_running_flag = false;

/******************************************************************************
 * @brief  check_monitor_thread_running_flag
 *****************************************************************************/
bool check_monitor_thread_running_flag(void)
{
    return is_monitor_thread_running_flag;
}

/******************************************************************************
 * @brief  monitor_thread
 *****************************************************************************/
static void monitor_thread( void *pvParameters )
{
    is_monitor_thread_running_flag = true;

    uint8_t i = FEED_WDOG_INTERVAL_MS;//该初值实现，一上电就喂狗一次

    for(;;)
    {
        //每隔FEED_WDOG_INTERVAL_MS秒，硬件看门狗喂狗
        if(i == FEED_WDOG_INTERVAL_MS)
        {
            i = 0;
            wdt_feed_safe();
//            LOG(LEVEL_DEBUG, "wdt feed");
        }

        i++;

        softwdt_monitor_thread_util(sleep_overwrite, NVIC_SystemReset);
    }
}

/******************************************************************************
 * @brief  monitor_init
 *****************************************************************************/
void monitor_init(void)
{
    /* 创建APP任务事件优先级调度线程*/
    portBASE_TYPE xStatus;
    xStatus = xTaskCreate( monitor_thread, "monitor", MONITOR_STACK_SIZE_FOR_TASK, NULL, MONITOR_TASK_PRIORITY, NULL );
    APP_ERROR_CHECK_BOOL(xStatus == pdTRUE);
}

