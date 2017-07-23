#include "trace.h"

#include "platform.h"
#include "log.h"

static TaskStatus_t pxTaskStatusArray[MAX_THREAD_NUMS];

/******************************************************************************
 * @brief  get_thread_state_implement
 * 获取OS任务信息
 *****************************************************************************/
void get_thread_state_implement(uint8_t* data, uint16_t data_length)
{
    const char task_state[] = {'r', 'R', 'B', 'S', 'D'};
    volatile UBaseType_t uxArraySize = 0;
    uint32_t ulTotalRunTime = 0;
    uint32_t ulStatsAsPercentage = 0;

    //获取任务总数目
    uxArraySize = uxTaskGetNumberOfTasks();
    if(uxArraySize > MAX_THREAD_NUMS)
    {
        LOG(LEVEL_DEBUG, "thread nums=%d invalid", uxArraySize);
        return;
    }

    //获取每个任务的状态信息
    uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime);

#if ( configUSE_TRACE_FACILITY == 1 )

    LOG(LEVEL_SIMPLE, "==================================================\r\n");
//    LOG(LEVEL_DEBUG, "总运行时间:%d", ulTotalRunTime);
    LOG(LEVEL_SIMPLE, "任务名                状态  ID    优先级  堆栈\r\n");

    //将获得的每一个任务状态信息部分的转化为程序员容易识别的字符串格式
    for(uint32_t x = 0; x < uxArraySize; x++ )
    {
        char tmp[128] = {0, };

        //避免除零错误
        if(ulTotalRunTime > 0)
        {
            //计算任务运行时间与总运行时间的百分比
            ulStatsAsPercentage = (uint64_t)(pxTaskStatusArray[ x ].ulRunTimeCounter)*100 / ulTotalRunTime;
        }

        if( ulStatsAsPercentage > 0UL )
        {
            sprintf(tmp, "%-22s%-6c%-6d%-8d%-8d", pxTaskStatusArray[ x].pcTaskName,   task_state[pxTaskStatusArray[ x ].eCurrentState],
                                                  pxTaskStatusArray[ x ].xTaskNumber, pxTaskStatusArray[ x].uxCurrentPriority,
                                                  pxTaskStatusArray[ x ].usStackHighWaterMark);
        }
        else
        {
            sprintf(tmp, "%-22s%-6c%-6d%-8d%-8d", pxTaskStatusArray[x ].pcTaskName,   task_state[pxTaskStatusArray[ x ].eCurrentState],
                                                  pxTaskStatusArray[ x ].xTaskNumber, pxTaskStatusArray[ x].uxCurrentPriority,
                                                  pxTaskStatusArray[ x ].usStackHighWaterMark);
        }

        LOG(LEVEL_SIMPLE, "%s\r\n", tmp);
    }

    LOG(LEVEL_SIMPLE, "任务状态:             r-运行  R-就绪  B-阻塞  S-挂起  D-删除\r\n");

#endif
}

