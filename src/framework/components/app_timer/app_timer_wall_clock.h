#ifndef __APP_TIMER_WALL_CLOCK_H__
#define __APP_TIMER_WALL_CLOCK_H__

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include "string.h"

#include "em_device.h"

#include "bsp_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Define to include wallclock functionality.
#if defined(FM_INNER_BURTC)
#define EMDRV_RTCDRV_WALLCLOCK_CONFIG
#endif


extern volatile uint32_t  wallClockTimeBase;
extern volatile uint32_t  wallClockOverflowCnt;


//对外函数接口
/***************************************************************************//**
 * @brief wallclock_set_timebase
 ******************************************************************************/
__STATIC_INLINE void wallclock_set_timebase(uint32_t timebase)
{
    wallClockTimeBase = timebase;
}

/***************************************************************************//**
 * @brief wallclock_get_timebase
 ******************************************************************************/
__STATIC_INLINE uint32_t wallclock_get_timebase(void)
{
    return wallClockTimeBase;
}

/***************************************************************************//**
 * @brief wallclock_set_overflowcnt
 ******************************************************************************/
__STATIC_INLINE void wallclock_set_overflowcnt(uint32_t overflowcnt)
{
    wallClockOverflowCnt = overflowcnt;
}

/***************************************************************************//**
 * @brief wallclock_get_overflowcnt
 ******************************************************************************/
__STATIC_INLINE uint32_t wallclock_get_overflowcnt(void)
{
    return wallClockOverflowCnt;
}


#ifdef __cplusplus
}
#endif

#endif
