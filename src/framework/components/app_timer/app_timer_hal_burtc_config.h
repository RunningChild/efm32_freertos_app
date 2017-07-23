#ifndef __APP_TIMER_HAL_BURTC_CONFIG_H__
#define __APP_TIMER_HAL_BURTC_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif


#define RTC_CLOCK                     (32768U)
#define RTC_DIVIDER                   (cmuClkDiv_8)
#define TIMER_COUNT_DIRECTION         (TIMER_COUNT_UP)//向上计数

#define TIMEDIFF( a, b )              ((a) - (b))
#define RTC_COUNTER_PASS()            BURTC_CounterGet()
#define RTC_COUNTERGET()              BURTC_CounterGet()
#define RTC_COUNTER_BITS              (32)
#define RTC_ALL_INTS                  (_BURTC_IF_MASK)
#define RTC_OF_INT                    (BURTC_IF_OF)
#define RTC_COMP_INT                  (BURTC_IF_COMP0)
#define RTC_COUNTER_MASK              (_BURTC_CNT_MASK)
#define RTC_MAX_VALUE                 (_BURTC_CNT_MASK)
#define RTC_INTDISABLE( x )           BURTC_IntDisable(x)
#define RTC_INTENABLE( x )            BURTC_IntEnable(x)
#define RTC_INTCLEAR( x )             BURTC_IntClear(x )
#define RTC_INTGET()                  BURTC_IntGet()
#define RTC_COUNTERRESET()            BURTC_CounterReset()
#define RTC_COMPARESET(x)             BURTC_CompareSet(0, (x) & _BURTC_COMP0_MASK)
#define RTC_COMPAREGET()              BURTC_CompareGet(0)
#define NVIC_IRQ_NUM                  (BURTC_IRQn)
#define NVIC_CLEARPENDINGIRQ()        NVIC_ClearPendingIRQ(NVIC_IRQ_NUM)
#define NVIC_DISABLEIRQ()             NVIC_DisableIRQ(NVIC_IRQ_NUM)
#define NVIC_ENABLEIRQ()              NVIC_EnableIRQ(NVIC_IRQ_NUM)
#define RTC_ONESHOT_TICK_ADJUST       (0)


#ifdef __cplusplus
}
#endif

#endif
