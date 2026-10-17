/* Race sub-slot 5 @ 0x1D8D0 — time-up / DNF fanfare (2020a8==10|11).
 *
 * Even first hit: clear 0x20aae8, 2020a8 += 1. Phase0: install 0x5bf790 PRG,
 * mute, palette 0xa3, force 214120=2 / 20a564=8, catalog_draw_setup, bump
 * phase. Phase1: peer wait or champ/practice geo_prg (5bfff0 / 5bfe70).
 * source: disasm/maincpu/maincpu_01d8d0_1b0.asm */
// @rom 0x1d8d0 +0x1b0 game_start_race_slot5

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_mem.h"

#include "lift_syms.h"

#include <stdio.h>

void game_start_race_slot5(u32 arg0, u32 arg1, u32 arg2)
{
    u32 phase;
    u32 timer;
    static int logged;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    if ((i960_ld_u8(I960_WORKRAM, 0x2020a8, 0) & 1u) == 0u) {
        g14 = 0;
        i960_st_u32(I960_WORKRAM, 0x20aae8, 0, 0u);
        i960_st_u32(I960_WORKRAM, 0x2020a8, 0,
                    i960_ld_u32(I960_WORKRAM, 0x2020a8, 0) + 1u);
    }

    phase = i960_ld_u32(I960_WORKRAM, 0x20aae8, 0);
    if (!logged) {
        lift_log( "lift: race_slot5 phase=%u\n", (unsigned)phase);
        fflush(stderr);
        logged = 1;
    }

    if (phase == 0u) {
        g0 = 0x005bf790u;
        game_start_race_geo_prg_slot((u32)g0, 0, 0);
        tile_map_banks_clear(0, 0, 0);
        game_start_race_sound_mute(0, 0, 0);
        g14 = 0;
        i960_st_u32(I960_WORKRAM, 0x2020a4, 0, 0u);
        comm_palette_index_call(0xa3u);

        timer = i960_ld_u32(I960_WORKRAM, 0x20a560, 0);
        i960_st_u32(I960_WORKRAM, 0x214120, 0, 2u);
        g14 = 0;
        i960_st_u32(I960_WORKRAM, 0x20a770, 0, 0u);
        i960_st_u32(I960_WORKRAM, 0x20a564, 0, 8u);

        /*
         * @0x1D958 cmpible 0,g4 → 0x1D974 when g4 >= 0.
         * g4 < 0 selects glyph 3. In-time split matches slot4.
         */
        if ((i32)timer < 0)
            g3 = 3;
        else if (i960_ld_u32(I960_WORKRAM, 0x20ac70, 0) != 0u)
            g3 = 0;
        else
            g3 = 1;
        g0 = 0;
        g1 = 19;
        g2 = 0x0288cf60u;
        g4 = 0;
        catalog_draw_setup(0, 19, 0x0288cf60u);

        i960_st_u32(I960_WORKRAM, 0x20aae8, 0, phase + 1u);
        return;
    }

    if (phase != 1u)
        return;

    /*
     * @0x1D9B8–0x1DA34: peer barrier. Solo practice leaves 20a581 ≤ 0 and
     * timer < 0 → fall through to fanfare at 0x1DA38.
     */
    {
        i32 peers = (i32)(int8_t)i960_ld_u8(I960_WORKRAM, 0x20a581, 0);
        u32 matched = 0u;

        if (peers > 0) {
            u32 i = 0u;
            u32 ea = 0u;
            u32 slot = 0x01a121dcu;
            u32 self_t = i960_ld_u32(I960_WORKRAM, 0x20a560, 0);
            u32 stride = 7u << 6;

            while ((i32)i < peers) {
                if (i960_ld_u32(I960_ABS, slot, 0) == self_t) {
                    u32 kind = i960_ld_u32(
                        I960_ABS,
                        i960_ld_u32(I960_ABS, 0x01a121d8u + ea, 0),
                        0);

                    if (kind != 8u && kind != 7u)
                        break;
                    matched += 1u;
                }
                i += 1u;
                ea += stride;
                slot += stride;
            }
        }

        if ((i32)matched != peers) {
            timer = i960_ld_u32(I960_WORKRAM, 0x20a560, 0);
            if ((i32)timer >= 0
                && i960_ld_u32(I960_WORKRAM, 0x20a758, 0) == 0u)
                return;
        }
    }

    g14 = 0;
    i960_st_u32(I960_WORKRAM, 0x20ab60, 0, 0u);
    if (i960_ld_u32(I960_WORKRAM, 0x202230, 0) == 0u)
        g0 = 0x005bfff0u;
    else
        g0 = 0x005bfe70u;
    game_start_race_geo_prg_slot((u32)g0, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20aae8, 0, phase + 1u);
}
