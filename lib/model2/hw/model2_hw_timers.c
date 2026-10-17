/* Model 2 timers at 0xf00000, 0xf00004, 0xf00008, 0xf0000c.
 *
 * Countdown rate is 25 MHz: timer 0's reload 0xc350 is the ~2 ms period
 * cited by irq_timer0_latch. A write stores the word and starts the count.
 * The CPU-visible value is the low 20 bits. irq_timer_arm reloads these
 * registers with 0xfffff, and the race frame treats a read of exactly
 * that word as "not running". The boot store of all-ones is that sentinel
 * until the count moves; the next values are what list_flag divides into
 * the rival LOD row.
 */
#include "model2_hw.h"

#include <time.h>

#define TIMER_BASE 0x00f00000u
#define TIMER_HZ   25000000.0

static u32 g_origin[4];
static u64 g_start_ns[4];
static int g_running[4];

static u64 timer_now_ns(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (u64)ts.tv_sec * 1000000000ull + (u64)ts.tv_nsec;
}

int model2_hw_timer_ea(u32 offset)
{
    return offset >= TIMER_BASE && offset < TIMER_BASE + 16u;
}

void model2_hw_timers_reset(void)
{
    unsigned i;

    for (i = 0; i < 4u; i++) {
        g_origin[i] = 0;
        g_start_ns[i] = 0;
        g_running[i] = 0;
    }
}

void model2_hw_timer_write(u32 offset, u32 value)
{
    unsigned i = (offset - TIMER_BASE) >> 2;

    g_origin[i] = value;
    g_start_ns[i] = timer_now_ns();
    g_running[i] = 1;
}

u32 model2_hw_timer_read(u32 offset)
{
    unsigned i = (offset - TIMER_BASE) >> 2;
    u64 elapsed;
    u32 count;

    if (!g_running[i])
        return g_origin[i] & 0xfffffu;
    elapsed = timer_now_ns() - g_start_ns[i];
    elapsed = (u64)((double)elapsed * (TIMER_HZ / 1.0e9));
    count = (u32)((u64)g_origin[i] - elapsed);
    return count & 0xfffffu;
}
