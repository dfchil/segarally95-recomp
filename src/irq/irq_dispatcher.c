/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/out/lift/slices/maincpu_000018d8_d8.asm */
// @rom 0x18d8 +0xd8 irq_dispatcher

#include "i960_lift.h"
#include "i960_mem.h"

/* convention: kind=leaf_ret  args g0,g1,g2  link g14 ret */
/* abi: u32 arg0=g0, u32 arg1=g1, u32 arg2=g2 → void */

/* pointers: fp=u32 *, g4=u32 */

extern void irq_mask_helper(u32 arg0, u32 arg1, u32 arg2);
extern void irq_timer_arm(u32 arg0, u32 arg1, u32 arg2);
extern void irq_dispatcher(u32 arg0, u32 arg1, u32 arg2);

void irq_dispatcher(u32 arg0, u32 arg1, u32 arg2)
{
    uintptr_t saved_g[16];
    uintptr_t saved_r[16];
    uintptr_t fp_save;
    uintptr_t sp_save;
    u8 frame[0x50];
    u32 pending;

    /* Host has no interrupt frame. Keep the caller's g/r/fp across the ISR. */
    saved_g[0] = g0; saved_g[1] = g1; saved_g[2] = g2; saved_g[3] = g3;
    saved_g[4] = g4; saved_g[5] = g5; saved_g[6] = g6; saved_g[7] = g7;
    saved_g[8] = g8; saved_g[9] = g9; saved_g[10] = g10; saved_g[11] = g11;
    saved_g[12] = g12; saved_g[13] = g13; saved_g[14] = g14; saved_g[15] = g15;
    saved_r[0] = r0; saved_r[1] = r1; saved_r[2] = r2; saved_r[3] = r3;
    saved_r[4] = r4; saved_r[5] = r5; saved_r[6] = r6; saved_r[7] = r7;
    saved_r[8] = r8; saved_r[9] = r9; saved_r[10] = r10; saved_r[11] = r11;
    saved_r[12] = r12; saved_r[13] = r13; saved_r[14] = r14; saved_r[15] = r15;
    fp_save = fp;
    sp_save = sp;
    fp = (uintptr_t)frame;
    sp = sp + 16u;
    g14 = 0;

    g0 = (uintptr_t)arg0;
    g1 = (uintptr_t)arg1;
    g2 = (uintptr_t)arg2;

    L_000018d8:
        g4 = i960_ld_u32(I960_MMIO, 0xe80000, 0); /* irq_request */;
        *(u32 *)(fp + 0x40) = (u32)g4;
        /* @0x18D4 / @0x18E8: and 0x3fc; cmpibe 0 → ret @ 0x1990. */
        pending = (u32)g4 & 0x3fcu;
        if (pending == 0u)
            goto L_exit;
        g4 = *(u32 *)(fp + 0x40);
        if (!((g4 >> 2) & 1))
            goto L_00001904;
        g0 = 4;
        irq_timer_arm(g0, g1, g2);
    goto L_000018d8;

    L_00001904:
        g4 = *(u32 *)(fp + 0x40);
        if (!((g4 >> 3) & 1))
            goto L_00001918;
        g0 = 8;
        irq_timer_arm(g0, g1, g2);
    goto L_000018d8;

    L_00001918:
        g4 = *(u32 *)(fp + 0x40);
        if (!((g4 >> 4) & 1))
            goto L_0000192c;
        g0 = 16;
        irq_timer_arm(g0, g1, g2);
    goto L_000018d8;

    L_0000192c:
        g4 = *(u32 *)(fp + 0x40);
        if (!((g4 >> 5) & 1))
            goto L_00001940;
        g0 = 31 + 1;
        irq_timer_arm(g0, g1, g2);
    goto L_000018d8;

    L_00001940:
        g4 = *(u32 *)(fp + 0x40);
        if (!((g4 >> 6) & 1))
            goto L_00001954;
        /* lift: setbit 6, 0, g0 @ 0x1948 */
        irq_mask_helper(g0, g1, g2);
    goto L_000018d8;

    L_00001954:
        g4 = *(u32 *)(fp + 0x40);
        if (!((g4 >> 7) & 1))
            goto L_00001968;
        /* lift: setbit 7, 0, g0 @ 0x195c */
        irq_mask_helper(g0, g1, g2);
    goto L_000018d8;

    L_00001968:
        g4 = *(u32 *)(fp + 0x40);
        if (!((g4 >> 8) & 1))
            goto L_0000197c;
        /* lift: setbit 8, 0, g0 @ 0x1970 */
        irq_mask_helper(g0, g1, g2);
    goto L_000018d8;

    L_0000197c:
        g4 = *(u32 *)(fp + 0x40);
        if (!((g4 >> 9) & 1))
            goto L_000018d8;
        /* lift: setbit 9, 0, g0 @ 0x1984 */
        irq_mask_helper(g0, g1, g2);
    goto L_000018d8;

    L_exit:
        fp = fp_save;
        sp = sp_save;
        g0 = saved_g[0]; g1 = saved_g[1]; g2 = saved_g[2]; g3 = saved_g[3];
        g4 = saved_g[4]; g5 = saved_g[5]; g6 = saved_g[6]; g7 = saved_g[7];
        g8 = saved_g[8]; g9 = saved_g[9]; g10 = saved_g[10]; g11 = saved_g[11];
        g12 = saved_g[12]; g13 = saved_g[13]; g14 = saved_g[14]; g15 = saved_g[15];
        r0 = saved_r[0]; r1 = saved_r[1]; r2 = saved_r[2]; r3 = saved_r[3];
        r4 = saved_r[4]; r5 = saved_r[5]; r6 = saved_r[6]; r7 = saved_r[7];
        r8 = saved_r[8]; r9 = saved_r[9]; r10 = saved_r[10]; r11 = saved_r[11];
        r12 = saved_r[12]; r13 = saved_r[13]; r14 = saved_r[14]; r15 = saved_r[15];
        return;
}
