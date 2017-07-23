#include <stdio.h>
#include "nop_delay.h"

void nop_delay_ms(uint32_t volatile number_of_ms)
{
    while(number_of_ms != 0)
    {
        number_of_ms--;
        nop_delay_us(999);
    }
}

