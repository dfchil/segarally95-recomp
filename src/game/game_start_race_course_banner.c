/* Course-event banners. record_index callx's the kind init, which
 * replaces node+4 with the per-frame draw.
 *
 * Kind 16 @ 0x451E0 / draw 0x44F00 — start gantry, mesh from 0x5e3df0/f8.
 * Kind 17 @ 0x45350 / draw 0x45240 — span-0 banner, mesh 0x5e4220[+0x5c].
 * Kind 18 @ 0x45550 / draw 0x45380 — span-0 banner, Y+1, frame 0x5e3b90.
 * Kind 20 @ 0x45AE0 / draw 0x45980 — checkpoint, scale from +0x60.
 *
 * source: disasm/maincpu/maincpu_044f00, 0451e0, 045240, 045350,
 *         045380, 045550, 045980, 045ae0
 */
// @rom 0x44f00 +0x2d0 game_start_race_course_obj_draw_16
// @rom 0x451e0 +0x3c  game_start_race_course_obj_init_16
// @rom 0x45240 +0x108 game_start_race_course_obj_draw_17
// @rom 0x45350 +0x20  game_start_race_course_obj_init_17
// @rom 0x45380 +0x1c4 game_start_race_course_obj_draw_18
// @rom 0x45550 +0x20  game_start_race_course_obj_init_18
// @rom 0x45980 +0x15c game_start_race_course_obj_draw_20
// @rom 0x45ae0 +0x20  game_start_race_course_obj_init_20

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "model2_memory.h"
#include "model2_rom.h"

u32 geo_rng_u8(u32 arg0, u32 arg1, u32 arg2);
void tile_texture_descriptor_apply(u32 slot_index, u32 fill_byte, u32 arg2);

static void fifo(u32 w)
{
    i960_mmio_write_u32(0x884000u, w);
}

static u32 nd(u32 node, u32 off)
{
    return i960_ld_u32(I960_ABS, node, off);
}

static void bank_poke(u32 bits)
{
    u32 bank = i960_ld_u32(I960_WORKRAM, 0x202278u, 0) & 3u;

    i960_st_u32(I960_ABS, 0x00800010u + (bank << 10), 0, bits);
}

static void bind_matrix(u32 emit)
{
    u32 wr_slot = i960_ld_u32(I960_WORKRAM, 0x20a290u, 0);
    u32 geo_wr = i960_ld_u32(I960_ABS, 0x00802008u, 0);

    i960_st_u32(I960_WORKRAM, wr_slot, 0, geo_wr);
    fifo(emit);
    i960_st_u32(I960_WORKRAM, 0x20a290u, 0, wr_slot + 4u);
    i960_st_u32(I960_ABS, 0x00801008u, 0, geo_wr + 0x34u);
}

static void submit_words(u32 base, int bump)
{
    u32 w0 = i960_ld_u32(I960_ABS, base, 0);
    u32 w1 = i960_ld_u32(I960_ABS, base, 4);
    u32 w2 = i960_ld_u32(I960_ABS, base, 8);
    u32 w3 = i960_ld_u32(I960_ABS, base, 12);

    if (bump) {
        u32 acc = i960_ld_u32(I960_WORKRAM, 0x20b940u, 0);

        i960_st_u32(I960_WORKRAM, 0x20b940u, 0, acc + w3);
    }
    i960_mmio_write_u32(GEO_PRG_FIFO, w0);
    i960_mmio_write_u32(GEO_PRG_FIFO, w1);
    i960_mmio_write_u32(GEO_PRG_FIFO, w2);
    i960_mmio_write_u32(GEO_PRG_FIFO, w3);
}

static void submit_catalog(u32 cat_ix)
{
    submit_words(CATALOG_VADDR + (cat_ix << 4), 1);
}

static void pose_yaw(u32 node)
{
    fifo(0x10002020u);
    fifo(0x13802727u);
    fifo(nd(node, 0x18u));
    fifo(nd(node, 0x1cu));
    fifo(nd(node, 0x20u));
    fifo(0x15002a2au);
    fifo(nd(node, 0x28u));
}

/* @0x44F00. g5 of the ldl is node+0x60; r3 is its low nibble. */
void game_start_race_course_obj_draw_16(u32 arg0, u32 arg1, u32 arg2)
{
    u32 node = arg0 != 0u ? arg0 : (u32)g0;
    u32 link = (u32)g14;
    u32 w5c = nd(node, 0x5cu);
    u32 w60 = nd(node, 0x60u);
    u32 bit = w5c & 1u;
    u32 var = w60 & 15u;
    u32 quad = 0x005e3e00u + (var << 4);
    u32 q0, q1, q2;
    u32 one;
    u32 mode;
    i32 sel;
    u32 flags;
    u32 cat;

    (void)arg1;
    (void)arg2;
    if (i960_ld_u32(I960_WORKRAM, 0x202098u, 0) == 2u)
        return;

    q0 = model2_workram_mirror_u32(quad);
    q1 = model2_workram_mirror_u32(quad + 4u);
    q2 = model2_workram_mirror_u32(quad + 8u);
    one = (u32)i960_f64_to_u32(1.0);

    fifo(0x10002020u);
    fifo(0x13802727u);
    fifo(nd(node, 0x18u));
    fifo(nd(node, 0x1cu));
    fifo(nd(node, 0x20u));
    fifo(0x15002a2au);
    fifo(nd(node, 0x28u));
    fifo(0x13802727u);
    fifo(q1);
    fifo(q2);
    fifo(link);
    fifo(0x14002828u);
    fifo(q0);
    fifo(one);
    fifo(one);
    bind_matrix(0x02800505u);

    /* @0x45030: mode > 4 forces sel 1; else the signed half at +0x0c. */
    mode = i960_ld_u32(I960_WORKRAM, 0x202098u, 0);
    if (mode > 4u)
        sel = 1;
    else {
        u32 raw = model2_workram_mirror_u32(0x005e3e0cu + (var << 4));

        sel = (i32)(int16_t)(raw & 0xffffu);
    }

    if (sel == 1)
        goto draw_primary;
    if (sel > 1) {
        if (sel == 2)
            goto draw_alt;
        goto pop;
    }
    if (sel != 0)
        goto pop;

    flags = i960_ld_u8(I960_WORKRAM, 0x217184u, 0);
    {
        u32 c4 = i960_ld_u32(I960_WORKRAM, 0x2020c4u, 0);
        u32 c0 = i960_ld_u32(I960_WORKRAM, 0x2020c0u, 0);

        i960_st_u8(I960_WORKRAM, 0x217184u, 0, (u8)(flags | 1u));
        if (c0 == c4 - 1u)
            i960_st_u8(I960_WORKRAM, 0x217184u, 0, (u8)(flags | 3u));
    }

draw_primary:
    flags = i960_ld_u8(I960_WORKRAM, 0x217184u, 0);
    cat = model2_workram_mirror_u32(0x005e3df0u + (bit << 2));
    bank_poke(link);
    submit_catalog(cat);
    i960_st_u8(I960_WORKRAM, 0x217184u, 0, (u8)(flags | 4u));
    goto pop;

draw_alt:
    flags = i960_ld_u8(I960_WORKRAM, 0x217184u, 0);
    if ((flags & 2u) == 0u)
        goto draw_bit0;
    cat = model2_workram_mirror_u32(0x005e3df8u + (bit << 2));
    bank_poke(link);
    submit_catalog(cat);
    goto pop;

draw_bit0:
    if ((flags & 1u) == 0u)
        goto pop;
    cat = model2_workram_mirror_u32(0x005e3df0u + (bit << 2));
    bank_poke(link);
    submit_catalog(cat);

pop:
    fifo(0x10802121u);
}

void game_start_race_course_obj_init_16(u32 arg0, u32 arg1, u32 arg2)
{
    u32 node = arg0 != 0u ? arg0 : (u32)g0;
    u32 flags = i960_ld_u8(I960_WORKRAM, 0x217184u, 0);

    (void)arg1;
    (void)arg2;
    /* @0x451F8: and 0xfc clears bits 0 and 1. */
    i960_st_u8(I960_WORKRAM, 0x217184u, 0, (u8)(flags & 0xfcu));
    i960_st_u32(I960_ABS, node, 0x64u, 0u);
    i960_st_u32(I960_ABS, node, 4u, 0x005e3f00u);
}

void game_start_race_course_obj_draw_17(u32 arg0, u32 arg1, u32 arg2)
{
    u32 node = arg0 != 0u ? arg0 : (u32)g0;
    u32 slot;
    u32 cat;

    (void)arg1;
    (void)arg2;
    if (i960_ld_u32(I960_WORKRAM, 0x202098u, 0) == 2u)
        return;

    pose_yaw(node);
    bind_matrix(0x02800505u);
    slot = nd(node, 0x5cu);
    cat = model2_workram_mirror_u32(0x005e4220u + (slot << 2));
    bank_poke((u32)g14);
    submit_catalog(cat);
    fifo(0x10802121u);
}

void game_start_race_course_obj_init_17(u32 arg0, u32 arg1, u32 arg2)
{
    u32 node = arg0 != 0u ? arg0 : (u32)g0;

    (void)arg1;
    (void)arg2;
    i960_st_u32(I960_ABS, node, 4u, 0x005e4240u);
}

void game_start_race_course_obj_draw_18(u32 arg0, u32 arg1, u32 arg2)
{
    u32 node = arg0 != 0u ? arg0 : (u32)g0;
    u32 phase = nd(node, 0x60u);
    u32 y;
    u32 frame;
    u32 cat;
    u32 table;

    (void)arg1;
    (void)arg2;

    /* @0x45384: +0x60 == 0 takes the mode gate; == 1 draws; else ret. */
    if (phase == 0u) {
        if (i960_ld_u32(I960_WORKRAM, 0x202230u, 0) != 0u)
            return;
        if ((i960_ld_u8(I960_WORKRAM, 0x217184u, 0) & 4u) != 0u)
            return;
        if (i960_ld_u32(I960_WORKRAM, 0x202098u, 0) == 2u)
            return;
    } else if (phase != 1u) {
        return;
    }

    y = (u32)i960_f64_to_u32(i960_u32_to_f64(nd(node, 0x1cu)) + 1.0);
    fifo(0x10002020u);
    fifo(0x13802727u);
    fifo(nd(node, 0x18u));
    fifo(y);
    fifo(nd(node, 0x20u));
    fifo(0x15002a2au);
    fifo(nd(node, 0x28u));
    bind_matrix(0x02800505u);

    frame = nd(node, 0x64u);
    table = nd(node, 0x5cu) == 0u ? 0x005e3b90u : 0x005e3cc0u;
    cat = model2_workram_mirror_u32(table + (frame << 2));
    bank_poke((u32)g14);
    submit_catalog(cat);
    fifo(0x10802121u);

    /* @0x45534: +0x64 += 1; above 0x4a wraps to 0. */
    frame += 1u;
    if (frame > 0x4au)
        frame = 0u;
    i960_st_u32(I960_ABS, node, 0x64u, frame);
}

void game_start_race_course_obj_init_18(u32 arg0, u32 arg1, u32 arg2)
{
    u32 node = arg0 != 0u ? arg0 : (u32)g0;
    u32 roll;

    (void)arg1;
    (void)arg2;
    roll = geo_rng_u8(node, 0, 0) & 0x3fu;
    i960_st_u32(I960_ABS, node, 4u, 0x005e4380u);
    i960_st_u32(I960_ABS, node, 0x64u, roll);
}

void game_start_race_course_obj_draw_20(u32 arg0, u32 arg1, u32 arg2)
{
    u32 node = arg0 != 0u ? arg0 : (u32)g0;
    double scaled;
    u32 bits;
    u32 slot;
    u32 cat;

    (void)arg1;
    (void)arg2;

    pose_yaw(node);
    /* cvtir +0x60, × 0x3fb99999:9999999a, + 1. Uniform 0x28 scale. */
    scaled = (double)(i32)nd(node, 0x60u);
    scaled = scaled * i960_rifl_read(0x9999999au, 0x3fb99999u) + 1.0;
    bits = (u32)i960_f64_to_u32(scaled);
    fifo(0x14002828u);
    fifo(bits);
    fifo(bits);
    fifo(bits);
    /*
     * @0x45A54 shro 3, r9, r8. r9 is still the 0x2a marker, so the
     * word stored to the copro is that marker shifted, not 0x05.
     */
    bind_matrix(0x15002a2au >> 3);
    slot = nd(node, 0x5cu) & 3u;
    cat = model2_workram_mirror_u32(0x005e4970u + (slot << 2));
    bank_poke((u32)g14);
    submit_catalog(cat);
    fifo(0x10802121u);
}

void game_start_race_course_obj_init_20(u32 arg0, u32 arg1, u32 arg2)
{
    u32 node = arg0 != 0u ? arg0 : (u32)g0;

    (void)arg1;
    (void)arg2;
    i960_st_u32(I960_ABS, node, 4u, 0x005e4980u);
}

static u32 abs_f32(u32 bits)
{
    return bits & 0x7fffffffu;
}

static u32 bias_abs(u32 src, u32 pos_hi, u32 neg_hi)
{
    u32 mag = abs_f32(src);
    double base = i960_u32_to_f64(mag);
    u32 hi = ((i32)src < 0) ? neg_hi : pos_hi;

    return (u32)i960_f64_to_u32(base + i960_rifl_read(0u, hi));
}

/* @0x45580. Championship start banner. +0x5c picks the quad. */
void game_start_race_course_obj_draw_19(u32 arg0, u32 arg1, u32 arg2)
{
    u32 node = arg0 != 0u ? arg0 : (u32)g0;
    u32 one = (u32)i960_f64_to_u32(1.0);
    u32 player;
    u32 r5;
    u32 r4;
    u32 tick;
    u32 nudged;
    u32 other;

    (void)arg1;
    (void)arg2;
    if (i960_ld_u32(I960_WORKRAM, 0x202230u, 0) != 0u)
        return;
    if ((i960_ld_u8(I960_WORKRAM, 0x217184u, 0) & 4u) != 0u)
        return;
    if (i960_ld_u32(I960_WORKRAM, 0x202098u, 0) == 2u)
        return;

    pose_yaw(node);
    fifo(0x14002828u);
    fifo(0x3f666666u);
    fifo(0x3f666666u);
    fifo(one);
    bind_matrix(0x02800505u);

    if (nd(node, 0x5cu) == 0u)
        submit_words(0x02867e00u, 1);
    else
        submit_words(0x02867dc0u, 1);

    /* Quad assembled from 0x2172a4 / 0x217324 / 0x2173a4 / g14. No budget add. */
    bank_poke((u32)g14);
    i960_mmio_write_u32(GEO_PRG_FIFO, i960_ld_u32(I960_WORKRAM, 0x2172a4u, 0));
    i960_mmio_write_u32(GEO_PRG_FIFO, i960_ld_u32(I960_WORKRAM, 0x217324u, 0));
    i960_mmio_write_u32(GEO_PRG_FIFO, i960_ld_u32(I960_WORKRAM, 0x2173a4u, 0));
    i960_mmio_write_u32(GEO_PRG_FIFO, (u32)g14);
    fifo(0x10802121u);

    player = i960_ld_u32(I960_WORKRAM, 0x2140ccu, 0);
    r5 = (u32)i960_f64_to_u32(
        i960_u32_to_f64(i960_ld_u32(I960_ABS, player, 0x14u))
        + i960_rifl_read(0u, 0x40771000u));
    r4 = (u32)i960_f64_to_u32(
        i960_u32_to_f64(i960_ld_u32(I960_ABS, player, 0x1cu))
        + i960_rifl_read(0u, 0x40728000u));
    tick = i960_ld_u32(I960_WORKRAM, 0x217180u, 0) + 1u;
    i960_st_u32(I960_WORKRAM, 0x217180u, 0, tick);

    /* @0x45814 ble: |r5| <= 3.0 leaves. @0x45844 bge: |r4| >= 48.0 leaves. */
    if (i960_u32_to_f64(abs_f32(r5)) <= i960_rifl_read(0u, 0x40080000u))
        return;
    if (i960_u32_to_f64(abs_f32(r4)) >= i960_rifl_read(0u, 0x40404000u))
        return;

    nudged = bias_abs(r5, 0x40080000u, 0xc0080000u);
    other = (u32)i960_f64_to_u32(
        i960_rifl_read(0u, ((i32)r4 < 0) ? 0xc0408000u : 0x40408000u)
        - i960_u32_to_f64(r4));
    if (i960_u32_to_f64(abs_f32(nudged)) <= i960_u32_to_f64(abs_f32(other)))
        other = 0u;
    else
        nudged = 0u;

    /* @0x458E0: counter > 30 calls 0x261B0 (g0=0x48, g1=0x7f). */
    if (tick > 30u) {
        g0 = 0x48u;
        g1 = 0x7fu;
        tile_texture_descriptor_apply(0x48u, 0x7fu, 0u);
    }

    player = i960_ld_u32(I960_WORKRAM, 0x2140ccu, 0);
    i960_st_u32(I960_ABS, player, 0x20u,
                (u32)i960_f64_to_u32(i960_u32_to_f64(i960_ld_u32(I960_ABS, player, 0x20u))
                                     + i960_u32_to_f64(nudged)));
    i960_st_u32(I960_ABS, player, 0x28u,
                (u32)i960_f64_to_u32(i960_u32_to_f64(i960_ld_u32(I960_ABS, player, 0x28u))
                                     + i960_u32_to_f64(other)));
}

void game_start_race_course_obj_init_19(u32 arg0, u32 arg1, u32 arg2)
{
    u32 node = arg0 != 0u ? arg0 : (u32)g0;
    u32 flags = i960_ld_u8(I960_WORKRAM, 0x217184u, 0);

    (void)arg1;
    (void)arg2;
    i960_st_u32(I960_ABS, node, 0x64u, 0u);
    i960_st_u32(I960_ABS, node, 4u, 0x005e4580u);
    i960_st_u32(I960_WORKRAM, 0x217180u, 0, 0u);
    i960_st_u8(I960_WORKRAM, 0x217184u, 0, (u8)(flags & ~4u));
}
