#ifndef __APP_TIMER_DRV_H__
#define __APP_TIMER_DRV_H__

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "string.h"

#include "ecode.h"

#include "interrupt_config.h"

#if (APP_TIMER_DRV_BURTC == APP_TIMER_DRV_SRC)
#include "em_burtc.h"
#include "app_timer_hal_burtc.h"
#include "app_timer_hal_burtc_config.h"
#elif (APP_TIMER_DRV_RTC == APP_TIMER_DRV_SRC)
#include "em_rtc.h"
#include "app_timer_hal_rtc.h"
#include "app_timer_hal_rtc_config.h"
#elif (APP_TIMER_DRV_LETIMER == APP_TIMER_DRV_SRC)
#include "em_letimer.h"
#include "app_timer_hal_letimer.h"
#include "app_timer_hal_letimer_config.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Define the number of timers the application needs.
#define EMDRV_RTCDRV_NUM_TIMERS                 (48)


//函数指针
typedef Ecode_t (*hal_timer_init)(void);
typedef Ecode_t (*hal_timer_deinit)(void);
typedef void (*hal_timer_over_flow_irq_process)(void);


#define ECODE_EMDRV_RTCDRV_OK                   ( ECODE_OK )                             ///< Success return value.
#define ECODE_EMDRV_RTCDRV_ALL_TIMERS_USED      ( ECODE_EMDRV_RTCDRV_BASE | 0x00000001 ) ///< No timers available.
#define ECODE_EMDRV_RTCDRV_ILLEGAL_TIMER_ID     ( ECODE_EMDRV_RTCDRV_BASE | 0x00000002 ) ///< Illegal timer id.
#define ECODE_EMDRV_RTCDRV_TIMER_NOT_ALLOCATED  ( ECODE_EMDRV_RTCDRV_BASE | 0x00000003 ) ///< Timer is not allocated.
#define ECODE_EMDRV_RTCDRV_PARAM_ERROR          ( ECODE_EMDRV_RTCDRV_BASE | 0x00000004 ) ///< Illegal input parameter.
#define ECODE_EMDRV_RTCDRV_TIMER_NOT_RUNNING    ( ECODE_EMDRV_RTCDRV_BASE | 0x00000005 ) ///< Timer is not running.

/// @brief Timer ID.
typedef uint32_t RTCDRV_TimerID_t;

/***************************************************************************//**
 * @brief
 *  Typedef for the user supplied callback function which is called when
 *  a timer elapse.
 *
 * @note This callback is called from within an interrupt handler with
 *       interrupts disabled.
 *
 * @param[in] id
 *   The timer id.
 *
 * @param[in] user
 *   Extra parameter for user application.
 ******************************************************************************/
typedef void (*RTCDRV_Callback_t)( RTCDRV_TimerID_t id, void *user );

/// @brief Timer type enumerator.
typedef enum {
  rtcdrvTimerTypeOneshot=0,    ///< Oneshot timer.
  rtcdrvTimerTypePeriodic=1    ///< Periodic timer.
} RTCDRV_TimerType_t;


//对外函数接口
uint32_t app_timer_get_second(void);
uint32_t app_timer_get_millisecond(void);

Ecode_t   RTCDRV_AllocateTimer( RTCDRV_TimerID_t *id );
Ecode_t   RTCDRV_DeInit(const hal_timer_deinit timer_deinit);
Ecode_t   RTCDRV_Delay( uint32_t ms );
Ecode_t   RTCDRV_FreeTimer( RTCDRV_TimerID_t id );
Ecode_t   RTCDRV_Init(const hal_timer_init timer_init, const hal_timer_over_flow_irq_process timer_over_flow_process);
Ecode_t   RTCDRV_IsRunning( RTCDRV_TimerID_t id, bool *isRunning );
Ecode_t   RTCDRV_StartTimer( RTCDRV_TimerID_t id,
                             RTCDRV_TimerType_t type,
                             uint32_t timeout,
                             RTCDRV_Callback_t callback,
                             void *user );
Ecode_t   RTCDRV_StopTimer( RTCDRV_TimerID_t id );
Ecode_t   RTCDRV_TimeRemaining( RTCDRV_TimerID_t id, uint32_t *timeRemaining );
bool RTCDRV_timer_check_running(RTCDRV_TimerID_t timer_id);

uint64_t  RTCDRV_MsecsToTicks( uint32_t ms );
uint64_t  RTCDRV_SecsToTicks( uint32_t secs );
Ecode_t   RTCDRV_SetWallClock( uint32_t secs );
uint32_t  RTCDRV_TicksToMsec( uint64_t ticks );
uint32_t  RTCDRV_TicksToSec( uint64_t ticks );
uint32_t  RTCDRV_GetCnt(void);
uint32_t  RTCDRV_GetMaxCnt(void);

#ifdef __cplusplus
}
#endif

#endif
