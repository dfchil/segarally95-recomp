/* Race sub-slot 4 @ 0x1D680 — course-complete fanfare (2020a8==8|9).
 *
 * Even first hit: clear 0x20aae4. Phase0: optional cam_bind, mute, force
 * 214120=2 / 20a564=7, catalog_draw_setup, install 0x5bf790, bump sub+phase.
 * Phase1: champ car-class / practice peer geo_prg fanfare leaves.
 * source: disasm/maincpu/maincpu_01d680_248.asm */
// @rom 0x1d680 +0x248 game_start_race_slot4

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_mem.h"

#include "lift_syms.h"

#include <stdio.h>

void game_start_race_slot4(u32 arg0, u32 arg1, u32 arg2)
{
    u32 phase;
    u32 timer;
    u32 car;
    static int logged;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    if ((i960_ld_u8(I960_WORKRAM, 0x2020a8, 0) & 1u) == 0u) {
        g14 = 0;
        i960_st_u32(I960_WORKRAM, 0x20aae4, 0, 0u);
    }

    phase = i960_ld_u32(I960_WORKRAM, 0x20aae4, 0);
    if (!logged) {
        lift_log( "lift: race_slot4 phase=%u\n", (unsigned)phase);
        fflush(stderr);
        logged = 1;
    }

    if (phase == 0u) {
        tile_map_banks_clear(0, 0, 0);
        timer = i960_ld_u32(I960_WORKRAM, 0x20a560, 0);
        if ((i32)timer > 0) {
            g0 = 0x005be1d0u;
            game_start_race_cam_bind((u32)g0, 0, 0);
        }
        g14 = 0;
        i960_st_u32(I960_WORKRAM, 0x2020a4, 0, 0u);
        i960_call_rom(0x1ebd0);
        game_start_race_sound_mute(0, 0, 0);

        timer = i960_ld_u32(I960_WORKRAM, 0x20a560, 0);
        i960_st_u32(I960_WORKRAM, 0x214120, 0, 2u);
        g14 = 0;
        i960_st_u32(I960_WORKRAM, 0x20ab60, 0, 0u);
        i960_st_u32(I960_WORKRAM, 0x20a564, 0, 7u);

        /*
         * @0x1D6FC cmpible 0,g4 → 0x1D718 when g4 >= 0.
         * g4 < 0 selects glyph 2 (SAS_FINISH). In-time split is
         * 0x20ac70: nonzero → glyph 0, zero → glyph 1.
         */
        if ((i32)timer < 0)
            g3 = 2;
        else if (i960_ld_u32(I960_WORKRAM, 0x20ac70, 0) != 0u)
            g3 = 0;
        else
            g3 = 1;
        g0 = 0;
        g1 = 19;
        g2 = 0x0288cf60u;
        g4 = 0;
        catalog_draw_setup(0, 19, 0x0288cf60u);

        g0 = 0x005bf790u;
        game_start_race_geo_prg_slot((u32)g0, 0, 0);

        phase = i960_ld_u32(I960_WORKRAM, 0x20aae4, 0) + 1u;
        i960_st_u32(I960_WORKRAM, 0x20aae4, 0, phase);
        i960_st_u32(I960_WORKRAM, 0x2020a8, 0,
                    i960_ld_u32(I960_WORKRAM, 0x2020a8, 0) + 1u);
        goto epilogue;
    }

    if (phase == 1u) {
        if (i960_ld_u32(I960_WORKRAM, 0x202230, 0) == 0u) {
            /* Championship: car-class fanfare. */
            car = i960_ld_u32(I960_WORKRAM, 0x214354, 0);
            if (car == 2u) {
                comm_palette_index_call(0x9eu);
                g0 = 0x005bfa50u;
                game_start_race_geo_prg_slot((u32)g0, 0, 0);
            } else if (car == 3u) {
                comm_palette_index_call(0xa3u);
                comm_palette_index_call(0x42u);
                g0 = 0x005bfbf0u;
                game_start_race_geo_prg_slot((u32)g0, 0, 0);
            } else {
                comm_palette_index_call(0x9eu);
                g0 = 0x005bf880u;
                game_start_race_geo_prg_slot((u32)g0, 0, 0);
            }
        } else {
            /* Practice: peer barrier then 0x9e/0x3f + 0x5bfcf0. */
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
                    goto epilogue;
            }

            comm_palette_index_call(0x9eu);
            comm_palette_index_call(0x3fu);
            g0 = 0x005bfcf0u;
            game_start_race_geo_prg_slot((u32)g0, 0, 0);
        }

        i960_st_u32(I960_WORKRAM, 0x20aae4, 0,
                    i960_ld_u32(I960_WORKRAM, 0x20aae4, 0) + 1u);
    }

epilogue:
    timer = i960_ld_u32(I960_WORKRAM, 0x20a560, 0);
    if ((i32)timer < 0)
        game_start_race_gate_walk(0, 0, 0);
}
