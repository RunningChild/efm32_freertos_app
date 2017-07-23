#ifndef __APP_TIMER_HAL_LETIMER_CONFIG_H__
#define __APP_TIMER_HAL_LETIMER_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif


#define RTC_CLOCK                     (32768U)
#define RTC_DIVIDER                   (cmuClkDiv_16)
#define TIMER_COUNT_DIRECTION         (TIMER_COUNT_DOWN)//向下计数

#define TIMEDIFF( a, b )              ((( (b)<<16) - ((a)<<16) ) >> 16 )
#define RTC_COUNTER_PASS()            (_LETIMER_CNT_MASK - LETIMER_CounterGet(LETIMER0))
#define RTC_COUNTERGET()              LETIMER_CounterGet(LETIMER0)
#define RTC_COUNTER_BITS              (16)
#define RTC_ALL_INTS                  (_LETIMER_IF_MASK)
#define RTC_OF_INT                    (LETIMER_IF_UF)
#define RTC_COMP_INT                  (LETIMER_IF_COMP0)
#define RTC_COUNTER_MASK              (_LETIMER_CNT_MASK)
#define RTC_MAX_VALUE                 (_LETIMER_CNT_MASK)
#define RTC_INTDISABLE( x )           LETIMER_IntDisable(LETIMER0, x)
#define RTC_INTENABLE( x )            LETIMER_IntEnable(LETIMER0, x)
#define RTC_INTCLEAR( x )             LETIMER_IntClear(LETIMER0, x)
#define RTC_INTGET()                  LETIMER_IntGet(LETIMER0)
#define RTC_COUNTERRESET()            LETIMER_CounterReset(LETIMER0)
#define RTC_COMPARESET( x )           LETIMER_CompareSet(LETIMER0, 0, (x) & _LETIMER_COMP0_MASK)
#define RTC_COMPAREGET()              LETIMER_CompareGet(LETIMER0, 0)
#define NVIC_IRQ_NUM                  (LETIMER0_IRQn)
#define NVIC_CLEARPENDINGIRQ()        NVIC_ClearPendingIRQ(NVIC_IRQ_NUM)
#define NVIC_DISABLEIRQ()             NVIC_DisableIRQ(NVIC_IRQ_NUM)
#define NVIC_ENABLEIRQ()              NVIC_EnableIRQ(NVIC_IRQ_NUM)
#define RTC_ONESHOT_TICK_ADJUST       (1)


#ifdef __cplusplus
}
#endif

#endif
