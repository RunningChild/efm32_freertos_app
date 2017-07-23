#ifndef __GLOBAL_A_H
#define __GLOBAL_A_H

#ifdef __cplusplus
extern "C" {
#endif

//release版本信息相关
#define DEFAULT_HARD_VERSION            VERSION(1, 0, 0, 0)
#define DEFAULT_MEDIA_VERSION           VERSION(1, 3, 0, 0)
#define DEFAULT_ZB_VERSION              VERSION(0, 0, 1, 0)
#define APP_BASE_VERSION                VERSION(1, 0, 0, 0)

#if defined( OTA_VERSION )
#define APP_VERSION                     (APP_BASE_VERSION + 1)
#else
#define APP_VERSION                     APP_BASE_VERSION
#endif



#ifdef __cplusplus
}
#endif

#endif
