#ifndef BOOT_STORAGE_H
#define BOOT_STORAGE_H

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include "string.h"
#include "app_error.h"

#ifdef __cplusplus
extern "C" {
#endif

//存储相关宏定义

//OTA FLASH存储的header信息
#define HEADER_NOT_OTA_UPDATE                   (0xffffffff)
#define HEADER_OTA_UPDATE                       (0x55555555)

typedef struct
{
    uint32_t ota_update_flag;                   //OTA更新标志
    uint32_t ota_update_crc;                    //OTA更新CRC值
    uint32_t ota_update_length;                 //OTA更新文件大小
    uint32_t ota_update_version;                //OTA更新文件版本号
}ota_flash_header_t;




//对外函数接口



#ifdef __cplusplus
}
#endif

#endif

