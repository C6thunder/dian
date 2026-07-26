/* ===========================================================================
 * soft_timer.c
 * =========================================================================*/
#include "soft_timer.h"
#include <string.h>

volatile uint32_t g_tick_ms = 0;
Car_t car;  /* 整车状态，单例 */

typedef struct {
    volatile uint32_t period_ms;
    volatile uint32_t last_tick;
    volatile uint8_t  is_timeout;
    volatile uint8_t  is_running;
    volatile uint8_t  is_repeat;
} timer_node_t;

static timer_node_t g_timers[SOFT_TIMER_MAX];

void soft_timer_init(void)
{
    memset(g_timers, 0, sizeof(g_timers));
}

void soft_timer_single_init(soft_timer_type_t id, uint32_t period_ms)
{
    if (id >= SOFT_TIMER_MAX) return;
    g_timers[id].period_ms = period_ms;
    g_timers[id].is_repeat = 0;
}

void soft_timer_repeat_init(soft_timer_type_t id, uint32_t period_ms)
{
    if (id >= SOFT_TIMER_MAX) return;
    g_timers[id].period_ms = period_ms;
    g_timers[id].is_repeat = 1;
}

void soft_timer_start(soft_timer_type_t id)
{
    if (id >= SOFT_TIMER_MAX) return;
    g_timers[id].last_tick  = g_tick_ms;
    g_timers[id].is_timeout = 0;
    g_timers[id].is_running = 1;
}

void soft_timer_stop(soft_timer_type_t id)
{
    if (id >= SOFT_TIMER_MAX) return;
    g_timers[id].is_running = 0;
}

uint8_t soft_timer_is_timeout(soft_timer_type_t id)
{
    if (id >= SOFT_TIMER_MAX) return 0;
    return g_timers[id].is_timeout;
}

void soft_timer_reset(soft_timer_type_t id)
{
    if (id >= SOFT_TIMER_MAX) return;
    g_timers[id].last_tick  = g_tick_ms;
    g_timers[id].is_timeout = 0;
}

uint32_t soft_timer_elapsed(soft_timer_type_t id)
{
    if (id >= SOFT_TIMER_MAX) return 0;
    return (uint32_t)(g_tick_ms - g_timers[id].last_tick);
}

void soft_timer_tick(void)
{
    for (uint32_t i = 0; i < SOFT_TIMER_MAX; ++i) {
        if (!g_timers[i].is_running) continue;
        if (g_timers[i].is_timeout)  continue;       // 未消费前不重复置位
        if ((uint32_t)(g_tick_ms - g_timers[i].last_tick) >= g_timers[i].period_ms) {
            g_timers[i].is_timeout = 1;
            /* 单次定时器到点后自动停：单次仍然依赖调用方 soft_timer_reset()
             * 不会重启，等下次显式 soft_timer_start()。 */
            if (!g_timers[i].is_repeat) {
                g_timers[i].is_running = 0;
            }
        }
    }
}
