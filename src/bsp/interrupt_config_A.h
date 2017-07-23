#ifndef __INTERRUPT_CONFIG_A_H
#define __INTERRUPT_CONFIG_A_H


#ifdef __cplusplus
extern "C" {
#endif

#if (1)
//定义os_tick实现定时器来源
#define OS_SYSTICK_RTC               (1)
#define OS_SYSTICK_BURTC             (2)
#define OS_SYSTICK_SOURCE            (OS_SYSTICK_RTC)
#endif

#if (1)
//定义app_timer实现定时器来源
#define APP_TIMER_DRV_RTC            (1)
#define APP_TIMER_DRV_BURTC          (2)
#define APP_TIMER_DRV_LETIMER        (3)
#define APP_TIMER_DRV_FREERTOS       (4)
#define APP_TIMER_DRV_SRC            (APP_TIMER_DRV_LETIMER)
#define APP_TIMER_DRV_IRQHandler     (LETIMER0_IRQHandler)
#endif


#if (1)
//以下定义各个中断硬件优先级
//操作系统可管理
#define LOWEST_IRQ_PRIORITY          (7)//不可改：提供free rtos系统节拍时钟

#define RTC_IRQ_PRIORITY             ((OS_SYSTICK_RTC == OS_SYSTICK_SOURCE)? LOWEST_IRQ_PRIORITY : 6)
#define BURTC_IRQ_PRIORITY           ((OS_SYSTICK_BURTC == OS_SYSTICK_SOURCE)? LOWEST_IRQ_PRIORITY : 6)
#define LETIMER_IRQ_PRIORITY         (6)
#define ADC_IRQ_PRIORITY             (7)
#define DMA_IRQ_PRIORITY             (4)
#define TIMER0_IRQ_PRIORITY          (3)//rgb pwd实现
#define TIMER1_IRQ_PRIORITY          (3)//buzz pwm实现
#define GPIOINT_IRQ_PRIORITY         (2)
#endif


#ifdef __cplusplus
}
#endif

#endif
