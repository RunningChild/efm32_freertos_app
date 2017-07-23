#ifndef __CMD_H
#define __CMD_H

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include "string.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_CMD_ENABLE                      (0)


//对外函数接口
void cmd_init(void);

#ifdef __cplusplus
}
#endif

#endif

