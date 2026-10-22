/* Desert course-list objects (spectators / roadside / animals).
 *
 * Kind 4 init @ 0x2B580 installs per-frame 0x5CA420 (ROM 0x2B420).
 * Kind 21 init @ 0x2BB80 installs 0x5CAB50 (ROM 0x2BB50). Draw @ 0x2BB50
 * returns when 0x202230!=0 (practice), 0x217184 bit2, or attract.
 *
 * source: disasm/maincpu/maincpu_02b580_180.asm,
 *         maincpu_02b420_160.asm, maincpu_02bb50_40.asm */
// @rom 0x2b580 +0x20 game_start_race_course_obj_init_4
// @rom 0x2b420 +0x15c game_start_race_course_obj_draw_4
// @rom 0x2bb80 +0x20 game_start_race_course_obj_init_21
// @rom 0x2bb50 +0x2c game_start_race_course_obj_draw_21

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "model2_memory.h"
#include "model2_rom.h"

#include <stdio.h>

static void geo_bank_poke0(void)
{
    u32 bank = i960_ld_u32(I960_WORKRAM, 0x202278, 0) & 3u;

    i960_mmio_write_u32(0x800010u + (bank << 10), 0u);
}

static void wr_slot_bind_from(u32 geo_wr, u32 wr_slot)
{
    i960_st_u32(I960_WORKRAM, wr_slot, 0, geo_wr);
    i960_mmio_write_u32(0x801008, geo_wr + 0x34u);
    i960_st_u32(I960_WORKRAM, 0x20a290, 0, wr_slot + 4u);
}

void game_start_race_course_obj_draw_4(u32 arg0, u32 arg1, u32 arg2)
{
    u32 node = arg0 != 0u ? arg0 : (u32)g0;
    u32 side;
    u32 x, y, z;
    u32 eye_x, eye_z;
    u32 wr_slot;
    u32 geo_wr;
    u32 cat_ix;
    u32 w0, w1, w2, w3;
    u32 bump;
    static int logged;

    (void)arg1;
    (void)arg2;

    /* @0x2B420–0x2B448: side xor 0x2020a4 selects TGP payload order. */
    side = (u32)i960_ld_u8(I960_ABS, node, 0x60)
        ^ (u32)i960_ld_u8(I960_WORKRAM, 0x2020a4, 0);
    x = i960_ld_u32(I960_ABS, node, 0x18);
    y = i960_ld_u32(I960_ABS, node, 0x1c);
    z = i960_ld_u32(I960_ABS, node, 0x20);
    eye_x = i960_ld_u32(I960_WORKRAM, 0x20220c, 0);
    eye_z = i960_ld_u32(I960_WORKRAM, 0x202214, 0);

    if (!logged) {
        lift_log(
                "lift: course_obj_draw_4 node=%#x xyz=(%.3g,%.3g,%.3g) slot=%u\n",
                (unsigned)node,
                i960_u32_to_f64(x), i960_u32_to_f64(y), i960_u32_to_f64(z),
                (unsigned)i960_ld_u32(I960_ABS, node, 0x5c));
        fflush(stderr);
        logged = 1;
    }

    wr_slot = i960_ld_u32(I960_WORKRAM, 0x20a290, 0);
    geo_wr = i960_ld_u32(I960_ABS, 0x00802008u, 0);
    i960_st_u32(I960_WORKRAM, wr_slot, 0, geo_wr);

    i960_mmio_write_u32(0x884000, 0x2e805d5du);
    if ((side & 1u) != 0u) {
        /* @0x2B450: obj, eye.xz, obj again. */
        i960_mmio_write_u32(0x884000, x);
        i960_mmio_write_u32(0x884000, y);
        i960_mmio_write_u32(0x884000, z);
        i960_mmio_write_u32(0x884000, eye_x);
        i960_mmio_write_u32(0x884000, x);
        i960_mmio_write_u32(0x884000, eye_z);
        i960_mmio_write_u32(0x884000, z);
    } else {
        i960_mmio_write_u32(0x884000, x);
        i960_mmio_write_u32(0x884000, y);
        i960_mmio_write_u32(0x884000, z);
        i960_mmio_write_u32(0x884000, x);
        i960_mmio_write_u32(0x884000, eye_x);
        i960_mmio_write_u32(0x884000, z);
        i960_mmio_write_u32(0x884000, eye_z);
    }
    wr_slot_bind_from(geo_wr, wr_slot);

    cat_ix = model2_workram_mirror_u32(
        0x5ca210u + (i960_ld_u32(I960_ABS, node, 0x5c) << 2));
    geo_bank_poke0();
    w0 = i960_ld_u32(I960_ABS, CATALOG_VADDR + (cat_ix << 4), 0);
    w1 = i960_ld_u32(I960_ABS, CATALOG_VADDR + (cat_ix << 4), 4);
    w2 = i960_ld_u32(I960_ABS, CATALOG_VADDR + (cat_ix << 4), 8);
    w3 = i960_ld_u32(I960_ABS, CATALOG_VADDR + (cat_ix << 4), 12);
    bump = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
    i960_st_u32(I960_WORKRAM, 0x20b940, 0, bump + w3);
    i960_mmio_write_u32(GEO_PRG_FIFO, w0);
    i960_mmio_write_u32(GEO_PRG_FIFO, w1);
    i960_mmio_write_u32(GEO_PRG_FIFO, w2);
    i960_mmio_write_u32(GEO_PRG_FIFO, w3);
}

void game_start_race_course_obj_init_4(u32 arg0, u32 arg1, u32 arg2)
{
    u32 node = arg0 != 0u ? arg0 : (u32)g0;

    (void)arg1;
    (void)arg2;
    /* @0x2B598: per-frame 0x5CA420. */
    i960_st_u32(I960_ABS, node, 4, 0x005ca420u);
}

void game_start_race_course_obj_draw_21(u32 arg0, u32 arg1, u32 arg2)
{
    /* @0x2BB50: ret if practice (0x202230!=0), 0x217184 bit2, or attract. */
    if (i960_ld_u32(I960_WORKRAM, 0x202230, 0) != 0u)
        return;
    if ((i960_ld_u8(I960_WORKRAM, 0x217184, 0) & 4u) != 0u)
        return;
    if (i960_ld_u32(I960_WORKRAM, 0x202098, 0) == 2u)
        return;
    game_start_race_course_obj_draw_4(arg0, arg1, arg2);
}

void game_start_race_course_obj_init_21(u32 arg0, u32 arg1, u32 arg2)
{
    u32 node = arg0 != 0u ? arg0 : (u32)g0;

    (void)arg1;
    (void)arg2;
    i960_st_u32(I960_ABS, node, 4, 0x005cab50u);
}

/*
 * Animated trackside mesh. Kind 6 init @ 0x2B6D0 installs draw 0x5CA5B0
 * (ROM 0x2B5B0). Kind 8 init @ 0x2B710 installs 0x5CA700, which is
 * call 0x2B5B0. Draw pushes, issues TGP 0x5e (translate, leave the
 * matrix), then keyframe 0x2B290 (TGP 0x5f per piece), then pops.
 *
 * source: disasm 0x2B5B0, 0x2B6D0, 0x2B700, 0x2B710, 0x2B290
 */
// @rom 0x2b290 +0x18c game_course_anim_keyframe
// @rom 0x2b5b0 +0x11c game_start_race_course_obj_draw_6
// @rom 0x2b6d0 +0x30  game_start_race_course_obj_init_6
// @rom 0x2b700 +0x8   game_start_race_course_obj_draw_8
// @rom 0x2b710 +0x30  game_start_race_course_obj_init_8

static void anim_fifo(u32 w)
{
    i960_mmio_write_u32(0x884000u, w);
}

static i32 anim_ldis(u32 va)
{
    u32 w = i960_ld_u32(I960_ABS, va & ~3u, 0);
    u32 shift = (va & 2u) ? 16u : 0u;

    return (i32)(int16_t)((w >> shift) & 0xffffu);
}

/* @0x2B290. g3=1 holds at limit-1; g3=0 restarts at 0. */
static u32 game_course_anim_keyframe(u32 table, u32 index, u32 step, u32 wrap)
{
    u32 limit = model2_workram_mirror_u32(table);
    u32 count = model2_workram_mirror_u32(table + 4u);
    u32 base = model2_workram_mirror_u32(table + 8u);
    i32 idx = (i32)index;
    u32 mag = idx < 0 ? (u32)(-idx) : (u32)index;
    u32 rec;
    u32 catp;
    u32 i;
    u32 link = (u32)g14;
    i32 next;
    u32 an;

    if ((i32)count > 0) {
        rec = base + count * mag * 20u;
        catp = table + 12u;
        for (i = 0u; i < count; i++) {
            u32 cat = model2_workram_mirror_u32(catp);
            u32 wr = i960_ld_u32(I960_WORKRAM, 0x20a290, 0);
            u32 geo = i960_ld_u32(I960_ABS, 0x00802008u, 0);
            u32 w0, w1, w2, w3;
            u32 bump;
            u32 bank;

            i960_st_u32(I960_WORKRAM, wr, 0, geo);
            anim_fifo(0x2f805f5fu);
            anim_fifo(i960_ld_u32(I960_ABS, rec, 0));
            anim_fifo(i960_ld_u32(I960_ABS, rec, 4));
            anim_fifo(i960_ld_u32(I960_ABS, rec, 8));
            anim_fifo((u32)anim_ldis(rec + 12u));
            anim_fifo((u32)anim_ldis(rec + 16u));
            i960_st_u32(I960_ABS, 0x00801008u, 0, geo + 0x34u);

            bank = i960_ld_u32(I960_WORKRAM, 0x202278, 0) & 3u;
            i960_mmio_write_u32(0x800010u + (bank << 10), link);

            w0 = i960_ld_u32(I960_ABS, CATALOG_VADDR + (cat << 4), 0);
            w1 = i960_ld_u32(I960_ABS, CATALOG_VADDR + (cat << 4), 4);
            w2 = i960_ld_u32(I960_ABS, CATALOG_VADDR + (cat << 4), 8);
            w3 = i960_ld_u32(I960_ABS, CATALOG_VADDR + (cat << 4), 12);
            bump = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
            i960_st_u32(I960_WORKRAM, 0x20b940, 0, bump + w3);
            i960_mmio_write_u32(GEO_PRG_FIFO, w0);
            i960_mmio_write_u32(GEO_PRG_FIFO, w1);
            i960_mmio_write_u32(GEO_PRG_FIFO, w2);
            i960_mmio_write_u32(GEO_PRG_FIFO, w3);
            i960_st_u32(I960_WORKRAM, 0x20a290, 0, wr + 4u);
            rec += 20u;
            catp += 4u;
        }
    }

    next = (i32)index + (i32)step;
    an = next < 0 ? (u32)(-next) : (u32)next;
    if ((i32)an < (i32)limit)
        g0 = (u32)next;
    else if (wrap == 0u)
        g0 = 0u;
    else
        g0 = limit - 1u;
    return (u32)g0;
}

static void anim_place(u32 node)
{
    u32 side = (u32)i960_ld_u8(I960_ABS, node, 0x60)
        ^ (u32)i960_ld_u8(I960_WORKRAM, 0x2020a4, 0);
    u32 x = i960_ld_u32(I960_ABS, node, 0x18);
    u32 y = i960_ld_u32(I960_ABS, node, 0x1c);
    u32 z = i960_ld_u32(I960_ABS, node, 0x20);
    u32 eye_x = i960_ld_u32(I960_WORKRAM, 0x20220c, 0);
    u32 eye_z = i960_ld_u32(I960_WORKRAM, 0x202214, 0);

    anim_fifo(0x2f005e5eu);
    anim_fifo(x);
    anim_fifo(y);
    anim_fifo(z);
    if ((side & 1u) != 0u) {
        anim_fifo(eye_x);
        anim_fifo(x);
        anim_fifo(eye_z);
        anim_fifo(z);
    } else {
        anim_fifo(x);
        anim_fifo(eye_x);
        anim_fifo(z);
        anim_fifo(eye_z);
    }
}

void game_start_race_course_obj_draw_6(u32 arg0, u32 arg1, u32 arg2)
{
    u32 node = arg0 != 0u ? arg0 : (u32)g0;
    u32 tick;
    u32 table;
    u32 idx;
    u32 step;

    (void)arg1;
    (void)arg2;

    anim_fifo(0x10002020u);
    anim_place(node);
    tick = i960_ld_u32(I960_ABS, 0x01d001b8u, 0) / 30u;
    idx = i960_ld_u32(I960_ABS, node, 0x54);
    step = i960_ld_u32(I960_ABS, node, 0x58);
    table = model2_workram_mirror_u32(0x005ca280u + ((tick & 3u) << 2));
    /* call 0x2B290 @ 0x2B6B0; g14 becomes 0x2B6B4. */
    g14 = 0x2b6b4u;
    i960_st_u32(I960_ABS, node, 0x54,
                game_course_anim_keyframe(table, idx, step, 1u));
    anim_fifo(0x10802121u);
}

void game_start_race_course_obj_init_6(u32 arg0, u32 arg1, u32 arg2)
{
    u32 node = arg0 != 0u ? arg0 : (u32)g0;

    (void)arg1;
    (void)arg2;
    i960_st_u32(I960_ABS, node, 0x58, 1u);
    i960_st_u32(I960_ABS, node, 0x54, 0u);
    i960_st_u32(I960_ABS, node, 4, 0x005ca5b0u);
}

void game_start_race_course_obj_draw_8(u32 arg0, u32 arg1, u32 arg2)
{
    game_start_race_course_obj_draw_6(arg0, arg1, arg2);
}

void game_start_race_course_obj_init_8(u32 arg0, u32 arg1, u32 arg2)
{
    u32 node = arg0 != 0u ? arg0 : (u32)g0;

    (void)arg1;
    (void)arg2;
    i960_st_u32(I960_ABS, node, 0x58, 1u);
    i960_st_u32(I960_ABS, node, 0x54, 0u);
    i960_st_u32(I960_ABS, node, 4, 0x005ca700u);
}
