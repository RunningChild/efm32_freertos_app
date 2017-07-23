# FreeRTOS系统实现低功耗管理



实现OS的低功耗，一种是靠idle任务，一种是用户适当停止当前任务。

参考链接[http://www.tuicool.com/articles/EbIbYzn](http://www.tuicool.com/articles/EbIbYzn)



- **idle**

- **tickless**

- **设计思路**


-------------------


## idle

将FreeRTOSConfig.h宏定义`configUSE_IDLE_HOOK`配置为1。


## tickless

我们知道，系统是基于SysTick来运作的，任务调度器可以预期到下一个周期性任务的触发时间，但是突发任务却无法预测，要用户有自己的中断来触发唤醒。

通过动态调整SysTick中断触发时间，可以少进入几次中断，少唤醒几次，也就是Tick less。从而更长的时间停留在低功耗模式中，当然这时候SysTick的RELOAD是变化的。

此时，MCU可能被两种不同的情况唤醒，动态调整过的系统时钟中断或者突发性的外部事件，无论是哪一种情况，都可以通过运行在低功耗模式下的某种定时器来计算出 MCU 处于低功耗模式下的时间，这样在MCU唤醒后对系统时间进行软件补偿，不至于系统的一些时间周期错误，比如vTaskDelay可能因此误以为足够延迟等等。

将FreeRTOSConfig.h宏定义`configUSE_TICKLESS_IDLE`配置为1。


## 设计思路
需要考虑的情况有，一种是系统由运行态进入到tickless模式，一种是系统由tickless模式唤醒。

这里提供一种设计思路：

创建一个功耗管理线程manager_thread，建议该线程优先级priority比空闲任务线程idle thread优先级高1级。

需要实现以下功能：

1)．不能运行在低功耗模式下外设，需要等待到空闲状态，才能睡眠，最后让系统自然地由运行态进入到tickless模式；


2)．外部中断源唤醒tickless模式下的系统后，所有应用层线程在运行态时需要判断manager_thread是否被唤醒，如果睡眠需要通过特定信号量将其唤醒，并运行逻辑1)的判断。

线程主体示例代码：

        //检查所有不能运行在低功耗模式下的外设，是否处于空闲状态
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
详见：`thread_manager.c`里manager_thread的实现。