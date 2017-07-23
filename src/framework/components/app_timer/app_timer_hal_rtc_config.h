#ifndef __APP_TIMER_HAL_RTC_CONFIG_H__
#define __APP_TIMER_HAL_RTC_CONFIG_H__


#ifdef __cplusplus
extern "C" {
#endif


#define RTC_CLOCK                     (32768U)
#define RTC_DIVIDER                   (cmuClkDiv_8)
#define TIMER_COUNT_DIRECTION         (TIMER_COUNT_UP)//向上计数

// To get the math correct we must have the MSB of the underlying 24bit
// counter in the MSB position of a uint32_t datatype.
#define TIMEDIFF( a, b )              ((( (a)<<8) - ((b)<<8) ) >> 8 )
#define RTC_COUNTER_PASS()            RTC_CounterGet()
#define RTC_COUNTERGET()              RTC_CounterGet()
#define RTC_COUNTER_BITS              (24)
#define RTC_ALL_INTS                  (_RTC_IF_MASK)
#define RTC_OF_INT                    (RTC_IF_OF)
#define RTC_COMP_INT                  (RTC_IF_COMP0)
#define RTC_COUNTER_MASK              (_RTC_CNT_MASK)
#define RTC_MAX_VALUE                 (_RTC_CNT_MASK)
#define RTC_INTDISABLE(x)             RTC_IntDisable(x)
#define RTC_INTENABLE(x)              RTC_IntEnable(x)
#define RTC_INTCLEAR(x)               RTC_IntClear(x)
#define RTC_INTGET()                  RTC_IntGet()
#define RTC_COUNTERRESET()            RTC_CounterReset()
#define RTC_COMPARESET(x)             RTC_CompareSet(0, (x) & _RTC_COMP0_MASK)
#define RTC_COMPAREGET()              RTC_CompareGet(0)
#define NVIC_IRQ_NUM                  (RTC_IRQn)
#define NVIC_CLEARPENDINGIRQ()        NVIC_ClearPendingIRQ(NVIC_IRQ_NUM)
#define NVIC_DISABLEIRQ()             NVIC_DisableIRQ(NVIC_IRQ_NUM)
#define NVIC_ENABLEIRQ()              NVIC_EnableIRQ(NVIC_IRQ_NUM)
#define RTC_ONESHOT_TICK_ADJUST       (1)


#ifdef __cplusplus
}
#endif

#endif
