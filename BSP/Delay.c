#include "Delay.h"

void delay_ms(uint32_t ms)
{
    uint32_t cycles = (CPUCLK_FREQ / 1000) * ms;
    delay_cycles(cycles);
}

/* v4.23：软件 I2C 用，5us 级别延时
 * CPUCLK_FREQ = 80MHz → 1us = 80 cycles
 * 函数调用本身约 8 cycles，所以 us=0 也至少 1 个 cycle 兜底 */
void delay_us(uint32_t us)
{
    uint32_t cycles = (CPUCLK_FREQ / 1000000U) * us;
    if (cycles > 8U) cycles -= 8U;
    else cycles = 1U;
    delay_cycles(cycles);
}
