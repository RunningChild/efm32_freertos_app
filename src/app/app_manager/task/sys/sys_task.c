#include "sys_task.h"

#include "log.h"

/******************************************************************************
 * @brief sys_shutdown_action
 *****************************************************************************/
void sys_shutdown_action(void)
{
    //Enable pin retention
    GPIO_EM4SetPinRetention(true);

    //关闭系统输入

}

tm_task sys_task = APP_TASK_INIT(sys);

/******************************************************************************
 * @brief sys_task_init
 *****************************************************************************/
void sys_task_init(void)
{
    app_task_open(&sys_task, APP_TASK_LOW_PRIORITY, sys_task_on_msg);
}

void sys_task_on_msg(tm_msg *msg, uint16_t msg_size)
{
    switch (msg->type)
    {
        case MSG_POWER_ON_RESET_LOGIC:
        {
            break;
        }

        default:
            break;
    }

    if (msg->data != NULL)
    {
        free_overwrite(msg->data);
        msg->data = NULL;
    }
}


