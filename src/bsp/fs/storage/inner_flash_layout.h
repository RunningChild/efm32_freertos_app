#ifndef INNER_FLASH_LAYOUT_H
#define INNER_FLASH_LAYOUT_H

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include "string.h"
#include "app_error.h"

#include "em_device.h"

#ifdef __cplusplus
extern "C" {
#endif


//页实际大小
#if defined(_EFM32_GIANT_FAMILY)
#define PAGE_SIZE                       ((FLASH_SIZE >= 0x80000) ? 0x1000 : 0x800)
#else
#pragma error "Unknown page size"
#endif

//计算页号对应的实际地址
#define ST_PAGE_ADDR(PAGE_NUM)          (PAGE_NUM*PAGE_SIZE)


//页号125~128
//用于密码等内部文件的存储空间，实现上:从ST_END_PAGE_NUM(高地址)向ST_START_PAGE_NUM(低地址)分配
#define ST_FILE_REGION_SIZE             (4*4*1024)

//页号121~124
//用于历史记录的存储空间，实现上:从ST_RAW_REGION_START_ADDRESS(低地址)向ST_RAW_REGION_END_ADDRESS(高地址)分配
#define ST_RAW_REGION_SIZE              (4*4*1024)
#define ST_RAW_REGION_END_ADDRESS       (FLASH_SIZE - ST_FILE_REGION_SIZE)
#define ST_RAW_REGION_START_ADDRESS     (ST_RAW_REGION_END_ADDRESS - ST_RAW_REGION_SIZE)


#define DATA_ISOLATE_RESERVE_SIZE       (0*4*1024)
#define PROGRAM_REGION_END_ADDRESS      (ST_RAW_REGION_START_ADDRESS - DATA_ISOLATE_RESERVE_SIZE)

//用于ota的存储空间
#define WHOLE_BOOT_MAX_LIMIT_SIZE       (4*4*1024)
#define WHOLE_APP_MAX_LIMIT_SIZE        ((PROGRAM_REGION_END_ADDRESS - WHOLE_BOOT_MAX_LIMIT_SIZE)/2)

//swap:页号63~120
#define SWAP_REGION_END_ADDRESS         (PROGRAM_REGION_END_ADDRESS)
#define SWAP_REGION_START_ADDRESS       (SWAP_REGION_END_ADDRESS - WHOLE_APP_MAX_LIMIT_SIZE)

//app:页号5~62
#define APP_REGION_END_ADDRESS          (SWAP_REGION_START_ADDRESS)
#define APP_REGION_START_ADDRESS        (APP_REGION_END_ADDRESS - WHOLE_APP_MAX_LIMIT_SIZE)

//bootloader:页号0~4
#define BOOT_REGION_START_ADDRESS       (0)
#define BOOT_REGION_END_ADDRESS         (BOOT_REGION_START_ADDRESS + WHOLE_BOOT_MAX_LIMIT_SIZE)




//对外函数接口


#ifdef __cplusplus
}
#endif

#endif

