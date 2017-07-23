#ifndef DELAY_H
#define DELAY_H

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include "string.h"
#include "app_error.h"

#ifdef __cplusplus
}
#endif


//外部函数接口
void delay_init(void);
void delay_ms(uint32_t ms);
void delay_us(uint32_t us);

#ifdef __cplusplus
}
#endif

#endif
