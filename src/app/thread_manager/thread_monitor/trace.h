#ifndef __TRACE_H
#define __TRACE_H

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include "string.h"
#include "app_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_THREAD_NUMS                 (20)


//对外函数接口
void get_thread_state_implement(uint8_t* data, uint16_t data_length);

#ifdef __cplusplus
}
#endif

#endif

