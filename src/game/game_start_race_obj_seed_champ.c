/* Championship AI car seed @ 0x219F0 — long path when 0x202230 != 1.
 *
 * Two stream walks from 0x5dd490 / 0x5dd494[214354*8], spacing from
 * 0x5c09a0[214354] (×3.4 when 0x20201d==3), car-type cursor on 0x5c0960.
 * Each hit: alloc @ 0x2AA90, pose link, attach @ 0x2C770.
 *
 * source: disasm/maincpu/maincpu_0219f0_2e0.asm */
// @rom 0x219f0 +0x2e0 game_start_race_obj_seed_champ

#include "i960_lift.h"
#include "i960_fp.h"
#include "lift_log.h"
#include "i960_mem.h"
#include "model2_rom.h"

#include "lift_syms.h"

#include <stdio.h>
#include <string.h>

void game_start_race_obj_car_attach_champ(u32 arg0, u32 arg1, u32 arg2);

static u32 f32_add(u32 a, u32 b)
{
    return (u32)i960_f64_to_u32(i960_u32_to_f64(a) + i960_u32_to_f64(b));
}

static u32 f32_sub(u32 a, u32 b)
{
    return (u32)i960_f64_to_u32(i960_u32_to_f64(a) - i960_u32_to_f64(b));
}

static i32 f32_to_i(u32 bits)
{
    return (i32)i960_u32_to_f64(bits);
}

static void champ_spawn(u32 *count_io, u32 w0, u32 w1, u32 w2, u32 w3,
                        u32 road)
{
    u32 node;
    u32 p;
    u8 b;
    u32 slot;
    u32 pose;
    u8 fl;

    g0 = 0;
    cgm_scratch_pool_alloc_link(0, 0, 0);
    node = (u32)g0;
    if (node == 0u)
        return;

    p = node + 0x40u;
    i960_st_u32(I960_ABS, p, 0, w0);
    i960_st_u32(I960_ABS, p, 4, w1);
    i960_st_u32(I960_ABS, p, 8, w2);
    i960_st_u32(I960_ABS, p, 12, w3);

    b = i960_ld_u8(I960_ABS, node, 0);
    b = (u8)((b & 0xc3u) | 4u);
    i960_st_u8(I960_ABS, node, 0, b);

    slot = *count_io;
    pose = 0x00213b40u + slot * 0x58u;
    i960_st_u32(I960_ABS, node, 0x8c, pose);
    *count_io = slot + 1u;
    i960_st_u32(I960_WORKRAM, 0x2139c8, 0, *count_io);
    i960_st_u32(I960_WORKRAM, 0x2140d0u + (slot << 2), 0, node);

    /* @0x21B20–0x21B50: and flags, (3<<6)=0xc0; or (count&15)<<2. */
    fl = i960_ld_u8(I960_ABS, pose + 0x51u, 0);
    fl = (u8)((fl & 0xc0u) | (u8)(((slot & 15u) << 2)));
    i960_st_u8(I960_ABS, pose + 0x51u, 0, fl);

    game_start_race_obj_car_attach_champ(node, road, p);
}

void game_start_race_obj_seed_champ(u32 arg0, u32 arg1, u32 arg2)
{
    uintptr_t fp_save = fp;
    uintptr_t sp_save = sp;
    u8 frame[0x80];
    u32 course;
    u32 spacing;
    u32 cursor;
    u32 type_ptr;
    u32 accum;
    i32 remain;
    u32 count;
    u32 w0, w1, w2, w3;
    u32 type_w;
    u32 road;
    static int logged;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;
    sp = sp + 0x10u;

    if (!logged) {
        lift_log("lift: race_obj_seed_champ (0x219f0)\n");
        fflush(stderr);
        logged = 1;
    }

    /* ROM @0x219FC / @0x21A38 / @0x21A48: ld 0x214354 only. */
    course = i960_ld_u32(I960_WORKRAM, 0x214354, 0);
    if (i960_ld_u8(I960_WORKRAM, 0x20201d, 0) == 3u) {
        double s = i960_u32_to_f64(
            model2_workram_mirror_u32(0x5c09a0u + (course << 2)));
        u32 even = 0x33333333u;
        u32 odd = 0x400b3333u;

        s *= i960_rifl_read(even, odd);
        spacing = (u32)i960_f64_to_u32(s);
    } else {
        spacing = model2_workram_mirror_u32(0x5c09a0u + (course << 2));
    }

    count = 1u;
    i960_st_u32(I960_WORKRAM, 0x2139c8, 0, count);

    /* ——— first stream @ 0x5dd490[course*8] ——— */
    cursor = model2_workram_mirror_u32(0x5dd490u + (course << 3));
    remain = (i32)i960_ld_u32(I960_WORKRAM, 0x2020c8, 0) - 1;
    accum = 0;
    type_ptr = 0x5c0960u + ((u32)remain << 2);

    if (i960_ld_u32(I960_ABS, cursor, 0) != (u32)-1) {
        while (remain >= 0) {
            if (i960_ld_u32(I960_ABS, cursor, 0) == (u32)-1)
                break;

            accum = f32_add(accum, spacing);
            road = (u32)(f32_to_i(accum)
                         + (i32)i960_ld_u32(I960_ABS, cursor + 8u, 0));

            w0 = i960_ld_u32(I960_ABS, cursor, 0);
            w1 = i960_ld_u32(I960_ABS, cursor, 4);
            w2 = i960_ld_u32(I960_ABS, cursor, 8);
            w3 = i960_ld_u32(I960_ABS, cursor, 12);
            /* @0x21AA4: ld (r7) — type from 0x5c0960[remain]. */
            type_w = i960_ld_u32(I960_ABS, type_ptr, 0);
            if (type_w == 0u) {
                /* @0x21ABC: cmpi 0x2139cc; alterbit 0,0,g0 → 1 iff zero. */
                u32 cc = i960_ld_u32(I960_WORKRAM, 0x2139cc, 0);

                w0 = (cc == 0u) ? 1u : 0u;
            } else {
                w0 = type_w;
            }

            champ_spawn(&count, w0, w1, w2, w3, road);

            cursor += 16u;
            type_ptr -= 4u;
            remain -= 1;
        }
    }

    i960_st_u32(I960_WORKRAM, 0x2021f0, 0, (u32)(remain + 1));

    /* ——— second stream @ 0x5dd494[course*8] ——— */
    cursor = model2_workram_mirror_u32(0x5dd494u + (course << 3));
    remain = (i32)i960_ld_u32(I960_WORKRAM, 0x2020c8, 0) + 1;
    accum = 0;
    type_ptr = 0x5c0960u + ((u32)remain << 2);

    /*
     * @0x21BC0: cmpibl 14,r5 → exit when 14 < r5. With 2020c8==14, r5 starts
     * at 15 so this loop is a no-op (matches ROM for default champ enter).
     */
    if (i960_ld_u32(I960_ABS, cursor, 0) != (u32)-1) {
        while (remain <= 14) {
            u32 c8;

            if (i960_ld_u32(I960_ABS, cursor, 0) == (u32)-1)
                break;

            accum = f32_sub(accum, spacing);
            road = (u32)(f32_to_i(accum)
                         + (i32)i960_ld_u32(I960_ABS, cursor + 8u, 0));

            w0 = i960_ld_u32(I960_ABS, cursor, 0);
            w1 = i960_ld_u32(I960_ABS, cursor, 4);
            w2 = i960_ld_u32(I960_ABS, cursor, 8);
            w3 = i960_ld_u32(I960_ABS, cursor, 12);
            type_w = i960_ld_u32(I960_ABS, type_ptr, 0);
            c8 = i960_ld_u32(I960_WORKRAM, 0x2020c8, 0);
            if (type_w != 0u && !(c8 == 0u && remain == 1)) {
                w0 = type_w;
            } else {
                /* @0x21BF4: same alterbit 0,0,g0 after cmpi 0x2139cc. */
                u32 cc = i960_ld_u32(I960_WORKRAM, 0x2139cc, 0);

                w0 = (cc == 0u) ? 1u : 0u;
            }

            champ_spawn(&count, w0, w1, w2, w3, road);

            cursor += 16u;
            type_ptr += 4u;
            remain += 1;
        }
    }

    i960_st_u32(I960_WORKRAM, 0x20caf0, 0, 0xffffffffu);
    i960_st_u32(I960_WORKRAM, 0x2140d0u + (count << 2), 0, 0);

    lift_log("lift: race_obj_seed_champ count=%u idx=%u\n",
             (unsigned)count, (unsigned)course);
    fflush(stderr);

    fp = fp_save;
    sp = sp_save;
}
