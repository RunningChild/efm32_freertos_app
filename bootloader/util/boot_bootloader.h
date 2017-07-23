#ifndef __BOOT_BOOTLOADER_H
#define __BOOT_BOOTLOADER_H

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include "string.h"

#ifdef __cplusplus
extern "C" {
#endif

//对外函数接口
int bootloader_entry(void);

#ifdef __cplusplus
}
#endif

#endif

