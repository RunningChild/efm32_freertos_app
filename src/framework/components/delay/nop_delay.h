#ifndef __NOP_DELAY_H
#define __NOP_DELAY_H

#include <stdint.h>
#include "em_device.h"

#include "bsp_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CORE_CLK_14MHZ_DEFINE   (14*1000*1000)
#define CORE_CLK_28MHZ_DEFINE   (28*1000*1000)
#define CORE_CLK_48MHZ_DEFINE   (48*1000*1000)


#if defined ( __CC_ARM   )
static __ASM void __INLINE nop_delay_us(uint32_t volatile number_of_us)
{
loop
        SUBS    R0, R0, #1

    #if (configCPU_CLOCK_HZ_DEFINE == CORE_CLK_48MHZ_DEFINE)
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
    #elif (configCPU_CLOCK_HZ_DEFINE == CORE_CLK_14MHZ_DEFINE)
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
    #elif (configCPU_CLOCK_HZ_DEFINE == CORE_CLK_28MHZ_DEFINE)
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
    #else
        #error "nop_delay parameter not defined!"
    #endif

        BNE    loop
        BX     LR
}
#elif defined ( __ICCARM__ )
static void __INLINE nop_delay_us(uint32_t volatile number_of_us)
{
__ASM (
"loop:\n\t"
       " SUBS R0, R0, #1\n\t"

    #if (configCPU_CLOCK_HZ_DEFINE == CORE_CLK_48MHZ_DEFINE)
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
    #elif (configCPU_CLOCK_HZ_DEFINE == CORE_CLK_14MHZ_DEFINE)
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
    #elif (configCPU_CLOCK_HZ_DEFINE == CORE_CLK_28MHZ_DEFINE)
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
       " NOP\n\t"
    #else
        #error "nop_delay parameter not defined!"
    #endif

       " BNE loop\n\t");
}
#elif defined   (  __GNUC__  )
static void __INLINE nop_delay_us(uint32_t volatile number_of_us)
{
    do
    {
    __ASM volatile (

    #if (configCPU_CLOCK_HZ_DEFINE == CORE_CLK_48MHZ_DEFINE)
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
    #elif (configCPU_CLOCK_HZ_DEFINE == CORE_CLK_14MHZ_DEFINE)
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
    #elif (configCPU_CLOCK_HZ_DEFINE == CORE_CLK_28MHZ_DEFINE)
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
    #else
        #error "nop_delay parameter not defined!"
    #endif

    );
    } while (--number_of_us);
}
#endif

void nop_delay_ms(uint32_t volatile number_of_ms);

#ifdef __cplusplus
}
#endif

#endif

