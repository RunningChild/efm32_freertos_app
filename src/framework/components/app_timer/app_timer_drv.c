#include "app_timer_drv.h"
#include "em_cmu.h"
#include "em_common.h"
#include "em_int.h"
#include "em_rmu.h"
#include "em_emu.h"

#include "log.h"

#if (APP_TIMER_DRV_FREERTOS != APP_TIMER_DRV_SRC)
/******************************************************************************
                        硬件定时器实现偏移秒数和毫秒数
 *****************************************************************************/
#define TIMER_COUNTS_PER_SEC            (RTC_CLOCK/RTC_DIVIDER)
#define TIMER_COUNTS_PER_MSEC           ((TIMER_COUNTS_PER_SEC)/1000U)

static volatile uint32_t time_overflow_counter = 0;

/*******************************************************************************
 * @brief app_timer_get_wall_clock
 ******************************************************************************/
static uint32_t app_timer_get_wall_clock(const uint32_t counts_per_sec_or_millisec)
{
    uint32_t t = 0;
    uint32_t overflow_interval  =  ((uint64_t)RTC_COUNTER_MASK+1) / counts_per_sec_or_millisec;
    uint32_t overflow_interval_r = ((uint64_t)RTC_COUNTER_MASK+1) % counts_per_sec_or_millisec;

    t += time_overflow_counter * overflow_interval;
    if(overflow_interval_r != 0)
    {
        t += time_overflow_counter * overflow_interval_r / counts_per_sec_or_millisec;
    }

    t += (RTC_COUNTER_PASS() / counts_per_sec_or_millisec);

    return t;
}

/*******************************************************************************
 * @brief app_timer_get_second
 ******************************************************************************/
uint32_t app_timer_get_second(void)
{
    return app_timer_get_wall_clock(TIMER_COUNTS_PER_SEC);
}

/*******************************************************************************
 * @brief app_timer_get_millisecond
 ******************************************************************************/
uint32_t app_timer_get_millisecond(void)
{
    return app_timer_get_wall_clock(TIMER_COUNTS_PER_MSEC);
}

/******************************************************************************
                        软件定时器实现
 *****************************************************************************/

#define TIMER_COUNT_UP                  (1)
#define TIMER_COUNT_DOWN                (2)


// Maximum number of ticks per overflow period (not the maximum tick value)
#if (TIMER_COUNT_UP == TIMER_COUNT_DIRECTION)
#define MAX_RTC_TICK_CNT                (RTC_MAX_VALUE+1UL)
#define RTC_CLOSE_TO_MAX_VALUE          (RTC_MAX_VALUE-100UL)
#elif (TIMER_COUNT_DOWN == TIMER_COUNT_DIRECTION)
#define MAX_RTC_TICK_CNT                (1UL)
#define RTC_CLOSE_TO_MAX_VALUE          (100UL)
#endif


#define MSEC_TO_TICKS_DIVIDER           ( 1000U * RTC_DIVIDER )
#define MSEC_TO_TICKS_ROUNDING_FACTOR   ( MSEC_TO_TICKS_DIVIDER / 2 )
#define MSEC_TO_TICKS( ms )             ( ( ( (uint64_t)(ms) * RTC_CLOCK )    \
                                            + MSEC_TO_TICKS_ROUNDING_FACTOR ) \
                                          / MSEC_TO_TICKS_DIVIDER )

#define TICKS_TO_MSEC_ROUNDING_FACTOR   ( RTC_CLOCK / 2 )
#define TICKS_TO_MSEC( ticks )          ( ( ( (uint64_t)(ticks)               \
                                              * RTC_DIVIDER * 1000U )         \
                                            + TICKS_TO_MSEC_ROUNDING_FACTOR ) \
                                          / RTC_CLOCK )

#define TICKS_TO_SEC_ROUNDING_FACTOR    ( RTC_CLOCK / 2 )
#define TICKS_TO_SEC( ticks )           ( ( ( (uint64_t)(ticks)               \
                                              * RTC_DIVIDER )                 \
                                            + TICKS_TO_SEC_ROUNDING_FACTOR )  \
                                          / RTC_CLOCK )
#define TICK_TIME_USEC                  ( 1000000 * RTC_DIVIDER / RTC_CLOCK )

typedef struct Timer
{
  uint64_t            remaining;
  uint64_t            ticks;
  int                 periodicCompensationUsec;
  unsigned int        periodicDriftUsec;
  RTCDRV_Callback_t   callback;
  bool                running;
  bool                doCallback;
  bool                allocated;
  RTCDRV_TimerType_t  timerType;
  void                *user;
} Timer_t;

static Timer_t            timer[ EMDRV_RTCDRV_NUM_TIMERS ];
static uint32_t           lastStart;
static volatile uint32_t  startTimerNestingLevel;
static bool               inTimerIRQ;
static bool               rtcRunning;
static bool               rtcdrvIsInitialized = false;

static hal_timer_over_flow_irq_process wall_clock_over_flow_irq_process = NULL;


static void checkAllTimers( uint32_t timeElapsed );
static void delayTicks( uint32_t ticks );
static void executeTimerCallbacks( void );
static void rescheduleRtc( uint32_t rtcCnt );

/// @endcond

/***************************************************************************//**
 * @brief
 *    Allocate timer.
 *
 * @details
 *    Reserve a timer instance.
 *
 * @param[out] id The id of the reserved timer.
 *
 * @return
 *    @ref ECODE_EMDRV_RTCDRV_OK on success.@n
 *    @ref ECODE_EMDRV_RTCDRV_ALL_TIMERS_USED when no timers are available.@n
 *    @ref ECODE_EMDRV_RTCDRV_PARAM_ERROR if an invalid id pointer was supplied.
 ******************************************************************************/
Ecode_t RTCDRV_AllocateTimer( RTCDRV_TimerID_t *id )
{
  int i      = 0;
  int retVal = 0;

  INT_Disable();
  // Iterate through the table of the timers until the first available.
  while ( ( i < EMDRV_RTCDRV_NUM_TIMERS ) && ( timer[ i ].allocated ) ) {
    i++;
  }

  // Check if we reached the end of the table.
  if ( i == EMDRV_RTCDRV_NUM_TIMERS ) {
    retVal = ECODE_EMDRV_RTCDRV_ALL_TIMERS_USED;
  } else {
    // Check if a NULL pointer was passed.
    if ( id != NULL ) {
      timer[ i ].allocated = true;
      *id = i;
      retVal = ECODE_EMDRV_RTCDRV_OK;
    } else {
      retVal = ECODE_EMDRV_RTCDRV_PARAM_ERROR;
    }
  }
  INT_Enable();

  return retVal;
}

/***************************************************************************//**
 * @brief
 *    Millisecond delay function.
 *
 * @param[in] ms Milliseconds to stay in the delay function.
 *
 * @return
 *    @ref ECODE_EMDRV_RTCDRV_OK.
 ******************************************************************************/
Ecode_t RTCDRV_Delay( uint32_t ms )
{
  uint64_t totalTicks;

  totalTicks = MSEC_TO_TICKS( ms );

  while ( totalTicks > RTC_CLOSE_TO_MAX_VALUE ) {
    delayTicks( RTC_CLOSE_TO_MAX_VALUE );
    totalTicks -= RTC_CLOSE_TO_MAX_VALUE;
  }
  delayTicks( totalTicks );

  return ECODE_EMDRV_RTCDRV_OK;
}

/***************************************************************************//**
 * @brief
 *    Free timer.
 *
 * @details
 *    Release a reserved timer.
 *
 * @param[out] id The id of the timer to release.
 *
 * @return
 *    @ref ECODE_EMDRV_RTCDRV_OK on success.@n
 *    @ref ECODE_EMDRV_RTCDRV_ILLEGAL_TIMER_ID if id has an illegal value.
 ******************************************************************************/
Ecode_t RTCDRV_FreeTimer( RTCDRV_TimerID_t id )
{
  // Check if valid timer ID.
  if ( id >= EMDRV_RTCDRV_NUM_TIMERS ) {
    return ECODE_EMDRV_RTCDRV_ILLEGAL_TIMER_ID;
  }

  INT_Disable();

  timer[ id ].running   = false;
  timer[ id ].allocated = false;

  INT_Enable();

  return ECODE_EMDRV_RTCDRV_OK;
}

/***************************************************************************//**
 * @brief
 *    Initialize RTCDRV driver.
 *
 * @details
 *    Will enable all necessary clocks. Initializes internal datastructures
 *    and configures the underlying hardware timer.
 *
 * @return
 *    @ref ECODE_EMDRV_RTCDRV_OK.
 ******************************************************************************/
Ecode_t RTCDRV_Init(const hal_timer_init timer_init, const hal_timer_over_flow_irq_process timer_over_flow_process)
{
    if ( rtcdrvIsInitialized == true ) {
        return ECODE_EMDRV_RTCDRV_OK;
    }
    rtcdrvIsInitialized = true;

    //运行硬件timer初始化加载
    if(NULL != timer_init)
    {
        timer_init();
    }

    //赋值溢出中断处理函数
    wall_clock_over_flow_irq_process = timer_over_flow_process;

    // Reset RTCDRV internal data structures/variables.
    memset( timer, 0, sizeof( timer ) );
    inTimerIRQ             = false;
    rtcRunning             = false;
    startTimerNestingLevel = 0;

    return ECODE_EMDRV_RTCDRV_OK;
}

/***************************************************************************//**
 * @brief
 *    Deinitialize RTCDRV driver.
 *
 * @details
 *    Will disable interrupts and turn off the clock to the underlying hardware
 *    timer.
 *    If integration with SLEEP module is enabled it will remove any
 *    restriction that are set on energy mode usage.
 *
 * @return
 *    @ref ECODE_EMDRV_RTCDRV_OK.
 ******************************************************************************/
Ecode_t RTCDRV_DeInit(const hal_timer_deinit timer_deinit)
{
    //运行硬件timer卸载
    if(NULL != timer_deinit)
    {
        timer_deinit();
    }

    // Mark the driver as uninitialized.
    rtcdrvIsInitialized = false;

    return ECODE_EMDRV_RTCDRV_OK;
}

/***************************************************************************//**
 * @brief
 *    Check if a given timer is running.
 *
 * @param[in] id The id of the timer to query.
 *
 * @param[out] isRunning True if timer is running. False if not running.
 *                       Only valid if return status is ECODE_EMDRV_RTCDRV_OK.
 *
 * @return
 *    @ref ECODE_EMDRV_RTCDRV_OK on success.@n
 *    @ref ECODE_EMDRV_RTCDRV_ILLEGAL_TIMER_ID if id has an illegal value. @n
 *    @ref ECODE_EMDRV_RTCDRV_TIMER_NOT_ALLOCATED if the timer is not reserved.@n
 *    @ref ECODE_EMDRV_RTCDRV_PARAM_ERROR if an invalid isRunning pointer was
 *         supplied.
 ******************************************************************************/
Ecode_t RTCDRV_IsRunning( RTCDRV_TimerID_t id, bool *isRunning )
{
  // Check if valid timer ID.
  if ( id >= EMDRV_RTCDRV_NUM_TIMERS ) {
    return ECODE_EMDRV_RTCDRV_ILLEGAL_TIMER_ID;
  }

  // Check pointer validity.
  if ( isRunning == NULL ) {
    return ECODE_EMDRV_RTCDRV_PARAM_ERROR;
  }

  INT_Disable();
  // Check if timer is reserved.
  if ( ! timer[ id ].allocated ) {
    INT_Enable();
    return ECODE_EMDRV_RTCDRV_TIMER_NOT_ALLOCATED;
  }
  *isRunning = timer[ id ].running;
  INT_Enable();

  return ECODE_EMDRV_RTCDRV_OK;
}

/***************************************************************************//**
 * @brief
 *    Start a timer.
 *
 * @note
 *    It is legal to start an already running timer.
 *
 * @param[in] id The id of the timer to start.
 * @param[in] type Timer type, oneshot or periodic. See @ref RTCDRV_TimerType_t.
 * @param[in] timeout Timeout expressed in milliseconds. If the timeout value
 *            is 0, the callback function will be called immediately and
 *            the timer will not be started.
 * @param[in] callback Function to call on timer expiry. See @ref
 *            RTCDRV_Callback_t. NULL is a legal value.
 * @param[in] user Extra callback function parameter for user application.
 *
 * @return
 *    @ref ECODE_EMDRV_RTCDRV_OK on success.@n
 *    @ref ECODE_EMDRV_RTCDRV_ILLEGAL_TIMER_ID if id has an illegal value.@n
 *    @ref ECODE_EMDRV_RTCDRV_TIMER_NOT_ALLOCATED if the timer is not reserved.
 ******************************************************************************/
Ecode_t RTCDRV_StartTimer(  RTCDRV_TimerID_t id,
                            RTCDRV_TimerType_t type,
                            uint32_t timeout,
                            RTCDRV_Callback_t callback,
                            void *user )
{
  uint32_t timeElapsed, cnt, compVal, loopCnt = 0;
  uint32_t timeToNextTimerCompletion;

  // Check if valid timer ID.
  if ( id >= EMDRV_RTCDRV_NUM_TIMERS ) {
    return ECODE_EMDRV_RTCDRV_ILLEGAL_TIMER_ID;
  }

  INT_Disable();
  if ( ! timer[ id ].allocated ) {
    INT_Enable();
    return ECODE_EMDRV_RTCDRV_TIMER_NOT_ALLOCATED;
  }

  if ( timeout == 0 ) {
    if ( callback != NULL ) {
      callback( id, user );
    }
    INT_Enable();
    return ECODE_EMDRV_RTCDRV_OK;
  }

  cnt = RTC_COUNTERGET();

  timer[ id ].callback  = callback;
  timer[ id ].ticks     = MSEC_TO_TICKS( timeout );
  if (rtcdrvTimerTypePeriodic == type) {
    // Calculate compensation value for periodic timers.
    timer[ id ].periodicCompensationUsec = 1000 * timeout -
      (timer[ id ].ticks * TICK_TIME_USEC);
    timer[ id ].periodicDriftUsec = TICK_TIME_USEC/2;
  }
  else
  {
    // Compensate for the fact that CNT is normally COMP0+1 after a
    // compare match event on some devices.
    timer[ id ].ticks -= RTC_ONESHOT_TICK_ADJUST;
  }
  // Add one tick in order to compensate if RTC is close to an increment event.
  timer[ id ].remaining = timer[ id ].ticks + 1;
  timer[ id ].running   = true;
  timer[ id ].timerType = type;
  timer[ id ].user      = user;

  if ( inTimerIRQ == true ) {
    // Exit now, remaining processing will be done in IRQ handler.
    INT_Enable();
    return ECODE_EMDRV_RTCDRV_OK;
  }

  // StartTimer() may recurse, keep track of recursion level.
  if ( startTimerNestingLevel < UINT32_MAX ) {
    startTimerNestingLevel++;
  }

  if ( rtcRunning == false ) {

#if (32 != RTC_COUNTER_BITS)
    lastStart = ( cnt ) & RTC_COUNTER_MASK;
#else
    lastStart = cnt;
#endif

    RTC_INTCLEAR( RTC_COMP_INT );

#if (TIMER_COUNT_UP == TIMER_COUNT_DIRECTION)
    compVal = SL_MIN( timer[ id ].remaining, RTC_CLOSE_TO_MAX_VALUE );
#elif (TIMER_COUNT_DOWN == TIMER_COUNT_DIRECTION)
    compVal = SL_MAX( timer[ id ].remaining, RTC_CLOSE_TO_MAX_VALUE );
#endif

#if (TIMER_COUNT_UP == TIMER_COUNT_DIRECTION)
    RTC_COMPARESET( cnt + compVal );
#elif (TIMER_COUNT_DOWN == TIMER_COUNT_DIRECTION)
    RTC_COMPARESET( cnt - compVal );
#endif

    // Start the timer system by enabling the compare interrupt.
    RTC_INTENABLE( RTC_COMP_INT );

    rtcRunning = true;

  } else {

    // The timer system is running. We must stop, update timers with the time
    // elapsed so far, find the timer with the shortest timeout and then restart.
    // As StartTimer() may be called from the callbacks we only do this
    // processing at the first nesting level.
    if ( startTimerNestingLevel == 1  ) {

      timer[ id ].running = false;
      // This loop is repeated if CNT is incremented while processing.
      do {

        RTC_INTDISABLE( RTC_COMP_INT );

        timeElapsed = TIMEDIFF( cnt, lastStart );
#if (32 != RTC_COUNTER_BITS)
        // Compensate for the fact that CNT is normally COMP0+1 after a
        // compare match event.
        if ( timeElapsed == RTC_MAX_VALUE ) {
          timeElapsed = 0;
        }
#endif

        // Update all timers with elapsed time.
        checkAllTimers( timeElapsed );

        // Execute timer callbacks.
        executeTimerCallbacks();

        // Set timer to running only after checkAllTimers() is called once.
        if ( loopCnt == 0 ) {
          timer[ id ].running = true;
        }
        loopCnt++;

        // Restart RTC according to next timeout.
        rescheduleRtc( cnt );

        cnt = RTC_COUNTERGET();
        timeElapsed = TIMEDIFF( cnt, lastStart );
        timeToNextTimerCompletion = TIMEDIFF( RTC_COMPAREGET(), lastStart );

        /* If the counter has passed the COMP(ARE) register value since we
           checked the timers, then we should recheck the timers and reschedule
           again. */
      }
      while ( rtcRunning && (timeElapsed > timeToNextTimerCompletion));
    }
  }

  if ( startTimerNestingLevel > 0 ) {
    startTimerNestingLevel--;
  }

  INT_Enable();
  return ECODE_EMDRV_RTCDRV_OK;
}

/***************************************************************************//**
 * @brief
 *    Stop a given timer.
 *
 * @param[in] id The id of the timer to stop.
 *
 * @return
 *    @ref ECODE_EMDRV_RTCDRV_OK on success.@n
 *    @ref ECODE_EMDRV_RTCDRV_ILLEGAL_TIMER_ID if id has an illegal value. @n
 *    @ref ECODE_EMDRV_RTCDRV_TIMER_NOT_ALLOCATED if the timer is not reserved.
 ******************************************************************************/
Ecode_t RTCDRV_StopTimer( RTCDRV_TimerID_t id )
{
  // Check if valid timer ID.
  if ( id >= EMDRV_RTCDRV_NUM_TIMERS ) {
    return ECODE_EMDRV_RTCDRV_ILLEGAL_TIMER_ID;
  }

  INT_Disable();
  if ( ! timer[ id ].allocated ) {
    INT_Enable();
    return ECODE_EMDRV_RTCDRV_TIMER_NOT_ALLOCATED;
  }

  timer[ id ].running = false;
  INT_Enable();

  return ECODE_EMDRV_RTCDRV_OK;
}

/***************************************************************************//**
 * @brief
 *    Get time left before a given timer expires.
 *
 * @param[in] id The id of the timer to query.
 *
 * @param[out] timeRemaining Time left expressed in milliseconds.
 *                        Only valid if return status is ECODE_EMDRV_RTCDRV_OK.
 * @return
 *    @ref ECODE_EMDRV_RTCDRV_OK on success.@n
 *    @ref ECODE_EMDRV_RTCDRV_ILLEGAL_TIMER_ID if id has an illegal value. @n
 *    @ref ECODE_EMDRV_RTCDRV_TIMER_NOT_ALLOCATED if the timer is not reserved.@n
 *    @ref ECODE_EMDRV_RTCDRV_TIMER_NOT_RUNNING if the timer is not running.@n
 *    @ref ECODE_EMDRV_RTCDRV_PARAM_ERROR if an invalid timeRemaining pointer
 *         was supplied.
 ******************************************************************************/
Ecode_t RTCDRV_TimeRemaining( RTCDRV_TimerID_t id, uint32_t *timeRemaining )
{
  uint64_t ticksLeft;
  uint32_t currentCnt, lastRtcStart;

  // Check if valid timer ID.
  if ( id >= EMDRV_RTCDRV_NUM_TIMERS ) {
    return ECODE_EMDRV_RTCDRV_ILLEGAL_TIMER_ID;
  }

  // Check pointer validity.
  if ( timeRemaining == NULL ) {
    return ECODE_EMDRV_RTCDRV_PARAM_ERROR;
  }

  INT_Disable();
  // Check if timer is reserved.
  if ( ! timer[ id ].allocated ) {
    INT_Enable();
    return ECODE_EMDRV_RTCDRV_TIMER_NOT_ALLOCATED;
  }

  // Check if timer is running.
  if ( ! timer[ id ].running ) {
    INT_Enable();
    return ECODE_EMDRV_RTCDRV_TIMER_NOT_RUNNING;
  }

  ticksLeft    = timer[ id ].remaining;
  currentCnt   = RTC_COUNTERGET();
  lastRtcStart = lastStart;
  INT_Enable();

  // Get number of RTC clock ticks elapsed since last RTC reschedule.
  currentCnt = TIMEDIFF( currentCnt, lastRtcStart );

  if ( currentCnt > ticksLeft ) {
    ticksLeft = 0;
  } else {
    ticksLeft -= currentCnt;
  }

  *timeRemaining = TICKS_TO_MSEC( ticksLeft );

  return ECODE_EMDRV_RTCDRV_OK;
}

/***************************************************************************//**
 * @brief
 *    Convert from milliseconds to RTC/RTCC ticks.
 *
 * @param[in] ms Millisecond value to convert.
 *
 * @return
 *    Number of ticks.
 ******************************************************************************/
uint64_t  RTCDRV_MsecsToTicks( uint32_t ms )
{
  return MSEC_TO_TICKS( ms );
}

/***************************************************************************//**
 * @brief
 *    Convert from seconds to RTC/RTCC ticks.
 *
 * @param[in] secs Second value to convert.
 *
 * @return
 *    Number of ticks.
 ******************************************************************************/
uint64_t  RTCDRV_SecsToTicks( uint32_t secs )
{
  return MSEC_TO_TICKS( 1000 * secs );
}

/***************************************************************************//**
 * @brief
 *    Convert from RTC/RTCC ticks to milliseconds.
 *
 * @param[in] ticks Number of ticks to convert.
 *
 * @return
 *    Number of milliseconds.
 ******************************************************************************/
uint32_t  RTCDRV_TicksToMsec( uint64_t ticks )
{
  return TICKS_TO_MSEC( ticks );
}

/***************************************************************************//**
 * @brief
 *    Convert from RTC/RTCC ticks to seconds.
 *
 * @param[in] ticks Number of ticks to convert.
 *
 * @return
 *    Number of seconds.
 ******************************************************************************/
uint32_t  RTCDRV_TicksToSec( uint64_t ticks )
{
  return TICKS_TO_MSEC( ticks ) / 1000;
}

/***************************************************************************//**
 * @brief RTCDRV_GetCnt
 ******************************************************************************/
uint32_t  RTCDRV_GetCnt(void)
{
    return RTC_COUNTERGET();
}

/***************************************************************************//**
 * @brief RTCDRV_GetMaxCnt
 ******************************************************************************/
uint32_t  RTCDRV_GetMaxCnt(void)
{
    return RTC_MAX_VALUE;
}


/// @cond DO_NOT_INCLUDE_WITH_DOXYGEN
static void checkAllTimers( uint32_t timeElapsed )
{
  int i;

  // Iterate through the timer table.
  // Update time remaining, check for timeout and rescheduling of periodic
  // timers, check for callbacks.

  for ( i = 0; i < EMDRV_RTCDRV_NUM_TIMERS; i++ ) {
    timer[ i ].doCallback = false;
    if ( timer[ i ].running == true ) {
      if ( timer[ i ].remaining > timeElapsed ) {
        timer[ i ].remaining -= timeElapsed;
      } else {
        if ( timer[ i ].timerType == rtcdrvTimerTypeOneshot ) {
          timer[ i ].running = false;
        } else {
          // Compensate overdue periodic timers to avoid accumlating errors.
          timer[ i ].remaining = timer[ i ].ticks - /*timeElapsed --->*/(timeElapsed/* % timer[ i ].ticks*/)/*<-- change by th*/ +
                                 timer[ i ].remaining;
          if ( timer[ i ].periodicCompensationUsec > 0 ) {
            timer[ i ].periodicDriftUsec += timer[i].periodicCompensationUsec;
            if (timer[ i ].periodicDriftUsec >= TICK_TIME_USEC) {
              // Add a tick if the timer drift is longer than the time of
              // one tick.
              timer[ i ].remaining += 1;
              timer[ i ].periodicDriftUsec -= TICK_TIME_USEC;
            }
          }
          else {
            timer[ i ].periodicDriftUsec -= timer[i].periodicCompensationUsec;
            if (timer[ i ].periodicDriftUsec >= TICK_TIME_USEC) {
              // Subtract one tick if the timer drift is longer than the time
              // of one tick.
              timer[ i ].remaining -= 1;
              timer[ i ].periodicDriftUsec -= TICK_TIME_USEC;
            }
          }
        }
        if ( timer[ i ].callback != NULL ) {
          timer[ i ].doCallback = true;
        }
      }
    }
  }

}

static void delayTicks( uint32_t ticks )
{
  uint32_t startTime;
  volatile uint32_t now;

  if ( ticks ) {
    startTime = RTC_COUNTERGET();
    do {
      now = RTC_COUNTERGET();
    } while ( TIMEDIFF( now, startTime ) < ticks );
  }
}

static void executeTimerCallbacks( void )
{
  int i;

  for ( i = 0; i < EMDRV_RTCDRV_NUM_TIMERS; i++ ) {
    if ( timer[ i ].doCallback ) {

      //_by_pbh add debug
      volatile uint32_t isr_vector_num = (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk);

      if((16 + NVIC_IRQ_NUM) != isr_vector_num)
      {
        LOG(LEVEL_DEBUG,"[rtcdriver] timer_id=%d, irqn=%d", i, isr_vector_num);
      }

      timer[ i ].callback( i, timer[ i ].user );
    }
  }
}

static void rescheduleRtc( uint32_t rtcCnt )
{
  int i;
  uint64_t min = UINT64_MAX;

  // Find the timer with shortest timeout.
  for ( i = 0; i < EMDRV_RTCDRV_NUM_TIMERS; i++ ) {
    if (    ( timer[ i ].running   == true )
         && ( timer[ i ].remaining <  min  ) ) {
      min = timer[ i ].remaining;
    }
  }

  rtcRunning = false;
  if ( min != UINT64_MAX ) {

#if (TIMER_COUNT_UP == TIMER_COUNT_DIRECTION)
    min = SL_MIN( min, RTC_CLOSE_TO_MAX_VALUE );
#elif (TIMER_COUNT_DOWN == TIMER_COUNT_DIRECTION)
    min = SL_MAX( min, RTC_CLOSE_TO_MAX_VALUE );
#endif

#if (32 != RTC_COUNTER_BITS)
    if ( inTimerIRQ == false ) {
      lastStart = ( rtcCnt ) & RTC_COUNTER_MASK;
    } else
#endif
    {
      lastStart = rtcCnt;
    }
    RTC_INTCLEAR( RTC_COMP_INT );

#if (TIMER_COUNT_UP == TIMER_COUNT_DIRECTION)
    RTC_COMPARESET( rtcCnt + min );
#elif (TIMER_COUNT_DOWN == TIMER_COUNT_DIRECTION)
    RTC_COMPARESET( rtcCnt - min );
#endif

    rtcRunning = true;

    // Reenable compare IRQ.
    RTC_INTENABLE( RTC_COMP_INT );
  }
}
/// @endcond

//_by_pbh add
bool RTCDRV_timer_check_running(RTCDRV_TimerID_t timer_id)
{
  return timer[timer_id].running;
}

/*******************************************************************************
 * @brief 中断处理函数
 ******************************************************************************/
void APP_TIMER_DRV_IRQHandler(void)
{
    uint32_t flags, timeElapsed, cnt, timeToNextTimerCompletion;

    INT_Disable();

    // CNT will normally be COMP0+1 at this point,
    // unless IRQ latency exceeded one tick period.

    flags = RTC_INTGET();

    if ( flags & RTC_COMP_INT ) {

        // Stop timer system by disabling the compare IRQ.
        // Update all timers with the time elapsed, call callbacks if needed,
        // then find the timer with the shortest timeout (if any at all) and
        // reenable the compare IRQ if needed.
        RTC_INTCLEAR( RTC_COMP_INT );

        inTimerIRQ = true;

        cnt = RTC_COUNTERGET();

        // This loop is repeated if CNT is incremented while processing.
        do {

            RTC_INTDISABLE( RTC_COMP_INT );

            timeElapsed = TIMEDIFF( cnt, lastStart );

            // Update all timers with elapsed time.
            checkAllTimers( timeElapsed );

            // Execute timer callbacks.
            executeTimerCallbacks();

            // Restart RTC according to next timeout.
            rescheduleRtc( cnt );

            cnt = RTC_COUNTERGET();
            timeElapsed = TIMEDIFF( cnt, lastStart );
            timeToNextTimerCompletion = TIMEDIFF( RTC_COMPAREGET(), lastStart );
            /* If the counter has passed the COMP(ARE) register value since we
            checked the timers, then we should recheck the timers and reschedule
            again. */
        }
        while ( rtcRunning && (timeElapsed > timeToNextTimerCompletion));
        inTimerIRQ = false;
    }

    if ( flags & RTC_OF_INT )
    {
        RTC_INTCLEAR( RTC_OF_INT );

        time_overflow_counter++;

        //执行wall_clock溢出处理函数
        if(NULL != wall_clock_over_flow_irq_process)
        {
            wall_clock_over_flow_irq_process();
        }
    }

    INT_Enable();
}
#endif

