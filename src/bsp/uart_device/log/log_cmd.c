#include "log_cmd.h"

#include "log_uart.h"

#include "sys_task.h"
#include "trace.h"

#include "thread_manager.h"

#include "log.h"

/******************************************************************************
 * @brief cmd_help
 *****************************************************************************/
static void cmd_help(void)
{
    LOG(LEVEL_SIMPLE, "\r\n");
    LOG(LEVEL_SIMPLE, "Please enter a command: \r\n"
                    " h - display this message\r\n"
                    " p - enter lower power:em4\r\n"
                    " t - print sys info, mem\r\n");
}

/******************************************************************************
 * @brief cmd_parse
 *****************************************************************************/
static int cmd_parse(uint8_t *data, uint32_t count)
{
    if(count < 1)
    {
        return -1;
    }

    uint8_t command = data[0];

    switch(command)
    {
        case 'h':
        {
            cmd_help();
            break;
        }
        case 'p':
        {
            LOG(LEVEL_DEBUG, "======================lower power em4\r\n");
            manager_EFM32_sys_off();
            break;
        }
        case 't':
        {
            send_task_cb(&sys_task, get_thread_state_implement, NULL, 0);
            mem_manager_print();
            break;
        }

        default:
            break;
    }

    return 0;
}

/******************************************************************************
 * @brief cmd_data_rx_cb
 *****************************************************************************/
static void cmd_data_rx_cb(uint32_t transferStatus, uint8_t *data, uint32_t count)
{
    bool is_recv_resolve_need_flag = false;

    if( (transferStatus == UART_TRANSFER_OK)
     || (transferStatus == UART_TRANSFER_ABORTED) )
    {
        is_recv_resolve_need_flag = true;
    }
    else
    {
//        LOG(LEVEL_FORCE, "recv fail!!!");
    }

    if( (true == is_recv_resolve_need_flag)
     && (count > 0) )
    {
        cmd_parse(data, count);
    }
}

/******************************************************************************
 * @brief cmd_init
 *****************************************************************************/
void cmd_init(void)
{
    log_uart_rx_start(cmd_data_rx_cb);
}

