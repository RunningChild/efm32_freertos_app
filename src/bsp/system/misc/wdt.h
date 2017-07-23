#ifndef __WDT_H
#define __WDT_H

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include "string.h"

#include "app_error.h"

#ifdef __cplusplus
extern "C" {
#endif


//外部函数接口
void wdt_init(void);
void wdt_start(void);
void wdt_feed_safe(void);

#ifdef __cplusplus
}
#endif

#endif
