/* Far object-pen emit @ 0x24560 — pen_desc mode ≠ 1.
 *
 * prg_budget promotes depth-list modes to 2 when view-depth > 0x213970
 * (~25). Unlifted call_rom no-ops, so world-yaw cars never drew and only
 * desert object_pen(follow) remained (rear-to-cam cancel).
 *
 * Disasm: push → 0x27 raw XYZ → yaw/pitch/roll → wr_slot → half-extent from
 * obj+0x30..0x3c → catalog pen+0xc with Y+=half → four catalogs from pen+8
 * → pop. pen+0 is a pointer to a 3-float local offset. Each pen+8 add
 * to 0x20b940 is that quad's word3.
 *
 * g0 = pen VA, g1 = pose guest VA, g2 = budget.
 *
 * source: disasm/maincpu/maincpu_024560_280.asm */
// @rom 0x24560 +0x268 game_start_race_object_pen_far

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "model2_memory.h"
#include "model2_rom.h"

#include "lift_syms.h"

#include <stdio.h>

/* call 0x24560 @ 0x2495C leaves g14 = 0x24960. Both bank pokes store it. */
static void geo_bank_poke_g14(void)
{
    u32 bank = i960_ld_u32(I960_WORKRAM, 0x202278, 0) & 3u;

    i960_mmio_write_u32(0x800010u + (bank << 10), (u32)g14);
}

static void wr_slot_bind(void)
{
    u32 wr_slot = i960_ld_u32(I960_WORKRAM, 0x20a290, 0);
    u32 wr_ptr = i960_ld_u32(I960_MMIO, 0x802008, 0);

    i960_st_u32(I960_WORKRAM, wr_slot, 0, wr_ptr);
    i960_mmio_write_u32(0x884000, 0x02800505u);
    i960_st_u32(I960_WORKRAM, 0x20a290, 0, wr_slot + 4u);
    i960_mmio_write_u32(0x801008, wr_ptr + 0x34u);
}

static void prg_stq_words(u32 w0, u32 w1, u32 w2, u32 w3)
{
    geo_bank_poke_g14();
    i960_mmio_write_u32(GEO_PRG_FIFO, w0);
    i960_mmio_write_u32(GEO_PRG_FIFO, w1);
    i960_mmio_write_u32(GEO_PRG_FIFO, w2);
    i960_mmio_write_u32(GEO_PRG_FIFO, w3);
}

static u32 catalog_word(u32 ix, u32 off)
{
    return i960_ld_u32(I960_ROM, CATALOG_VADDR + (ix << 4), off);
}

void game_start_race_object_pen_far(u32 arg0, u32 arg1, u32 arg2)
{
    u32 pen_va = arg0 != 0u ? arg0 : (u32)g0;
    /* ROM g1 is the pose guest VA; never a host pointer (u32 truncates). */
    u32 obj_va = arg1 != 0u ? arg1 : (u32)g1;
    u32 budget = arg2 != 0u ? arg2 : (u32)g2;
    u8 *obj;
    u32 *obj_w;
    u32 pen_hdr;
    u32 cat_ix;
    u32 remain;
    u32 take;
    u32 half;
    u32 i;
    u32 w0, w1, w2, w3;
    static int logged;

    obj = model2_ram_mut(obj_va);
    if (!obj || pen_va == 0u || obj_va == 0u)
        return;

    obj_w = (u32 *)(void *)obj;
    /* @0x2495C call 0x24560; st g14 @ 0x246A8 / 0x24770 is that link. */
    g14 = 0x24960u;
    pen_hdr = model2_workram_mirror_u32(pen_va);
    if (pen_hdr == 0u)
        return;

    if (!logged) {
        u32 follow = i960_ld_u32(I960_WORKRAM, 0x213980, 0);
        u32 fy = follow ? i960_ld_u32(I960_ABS, follow, 0x1c) : 0u;

        lift_log(
                "lift: object_pen_far pen=%#x obj=%#x yaw=%.3g "
                "follow_yaw=%.3g d8=%u\n",
                pen_va, obj_va, i960_u32_to_f64(obj_w[0x1c / 4]),
                i960_u32_to_f64(fy),
                (unsigned)i960_ld_u32(I960_WORKRAM, 0x2139d8, 0));
        fflush(stderr);
        logged = 1;
    }

    /* @0x24564–0x245F4: push + translate raw XYZ + yaw/pitch/roll. */
    i960_mmio_write_u32(0x884000, 0x10002020u);
    i960_mmio_write_u32(0x884000, 0x13802727u);
    i960_mmio_write_u32(0x884000, obj_w[0]);
    i960_mmio_write_u32(0x884000, obj_w[1]);
    i960_mmio_write_u32(0x884000, obj_w[2]);
    i960_mmio_write_u32(0x884000, 0x15002a2au);
    i960_mmio_write_u32(0x884000, obj_w[0x1c / 4]);
    i960_mmio_write_u32(0x884000, 0x14802929u);
    i960_mmio_write_u32(0x884000, obj_w[0x18 / 4]);
    i960_mmio_write_u32(0x884000, 0x15802b2bu);
    i960_mmio_write_u32(0x884000, obj_w[0x20 / 4]);
    wr_slot_bind();

    /* @0x24620–0x24680: mean(obj+0x30..0x3c)*0.25 → second 0x27 Y addend. */
    half = (u32)i960_f64_to_u32(
        (i960_u32_to_f64(obj_w[0x30 / 4]) + i960_u32_to_f64(obj_w[0x34 / 4])
         + i960_u32_to_f64(obj_w[0x38 / 4]) + i960_u32_to_f64(obj_w[0x3c / 4]))
        * i960_rifl_read(0, 0x3fd00000u));

    remain = budget - i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
    if ((i32)remain < 1)
        remain = 1u;

    /* @0x2468C–0x24724: catalog pen+0xc, then 0x27(pen_hdr.xyz) with Y+=half. */
    cat_ix = model2_workram_mirror_u32(pen_va + 0xcu);
    w0 = catalog_word(cat_ix, 0);
    w1 = catalog_word(cat_ix, 4);
    w2 = catalog_word(cat_ix, 8);
    w3 = catalog_word(cat_ix, 12);
    /*
     * @0x246B8 movt keeps the original ldq. cmpobge may replace g2
     * for the 0x20b940 add; stq @ 0x246DC submits the original word3.
     */
    take = w3;
    if (remain != 0u && w3 >= remain)
        take = remain;
    prg_stq_words(w0, w1, w2, w3);

    {
        u32 bank = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
        u32 px = model2_workram_mirror_u32(pen_hdr);
        u32 py = model2_workram_mirror_u32(pen_hdr + 4u);
        u32 pz = model2_workram_mirror_u32(pen_hdr + 8u);

        i960_mmio_write_u32(0x884000, 0x13802727u);
        i960_mmio_write_u32(0x884000, px);
        i960_mmio_write_u32(
            0x884000,
            (u32)i960_f64_to_u32(i960_u32_to_f64(py) + i960_u32_to_f64(half)));
        i960_mmio_write_u32(0x884000, pz);
        i960_st_u32(I960_WORKRAM, 0x20b940, 0, bank + take);
        wr_slot_bind();
    }

    /*
     * @0x2472C–0x247B4: g1 starts at 0, increments, cmpi 3 / ble.
     * Four catalogs from pen+8. ldq word3 (r7) is what addo stores
     * into 0x20b940 — not the earlier body take.
     */
    cat_ix = model2_workram_mirror_u32(pen_va + 8u);
    for (i = 0; i < 4u; i++) {
        u32 bank = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);

        w0 = catalog_word(cat_ix, 0);
        w1 = catalog_word(cat_ix, 4);
        w2 = catalog_word(cat_ix, 8);
        w3 = catalog_word(cat_ix, 12);
        prg_stq_words(w0, w1, w2, w3);
        i960_st_u32(I960_WORKRAM, 0x20b940, 0, bank + w3);
        wr_slot_bind();
    }

    i960_mmio_write_u32(0x884000, 0x10802121u);
}

/* Mid object-pen @ 0x240C0 — pen_desc when scale @ +0xc <= 100.
 *
 * call 0x240C0 from 0x2496C leaves g14 = 0x24970. The three bank pokes
 * store that g14. Pen words: +0 pointer to xyz triples, +4 / +8 / +0xc
 * catalog indices. Four triples; yaw obj+0x24 only while the trip index
 * is 0 or 1. object_extra when budget − 0x20b940 > 5.
 *
 * source: disasm/maincpu/maincpu_0240c0_4a0.asm */
// @rom 0x240c0 +0x494 game_start_race_object_pen_mid

static u32 f_add(u32 a, u32 b)
{
    return (u32)i960_f64_to_u32(i960_u32_to_f64(a) + i960_u32_to_f64(b));
}

static u32 f_sub(u32 a, u32 b)
{
    /* subr / subrl: dst = src2 − src1. */
    return (u32)i960_f64_to_u32(i960_u32_to_f64(a) - i960_u32_to_f64(b));
}

static void catalog_quad(u32 ix, u32 out[4])
{
    u32 base = CATALOG_VADDR + (ix << 4);
    u32 i;

    for (i = 0; i < 4u; i++)
        out[i] = i960_ld_u32(I960_ROM, base, i * 4u);
}

static void fifo_stq(const u32 w[4])
{
    i960_mmio_write_u32(GEO_PRG_FIFO, w[0]);
    i960_mmio_write_u32(GEO_PRG_FIFO, w[1]);
    i960_mmio_write_u32(GEO_PRG_FIFO, w[2]);
    i960_mmio_write_u32(GEO_PRG_FIFO, w[3]);
}

static void bank_poke_g14(void)
{
    u32 bank = i960_ld_u32(I960_WORKRAM, 0x202278, 0) & 3u;

    i960_mmio_write_u32(0x800010u + (bank << 10), (u32)g14);
}

void game_start_race_object_pen_mid(u32 arg0, u32 arg1, u32 arg2)
{
    u32 pen = arg0 != 0u ? arg0 : (u32)g0;
    u32 obj = arg1 != 0u ? arg1 : (u32)g1;
    u32 budget = arg2 != 0u ? arg2 : (u32)g2;
    u32 pen0;
    u32 sum;
    u32 y;
    u32 diff;
    u32 pitch;
    u32 cat[4];
    u32 ix;
    u32 bank;
    u32 wr;
    u32 fifo_ptr;
    u32 remain;
    u32 cursor;
    u32 ysrc;
    u32 i;
    u8 *obj_host;

    if (pen == 0u || obj == 0u)
        return;

    /* @0x2496C call: g14 is 0x24970 for the bank pokes below. */
    g14 = 0x24970u;

    pen0 = i960_ld_u32(I960_ABS, pen, 0);

    /* @0x240D0: push, then Y = mean(obj+0x40..0x4c) − 0x3F947AE1:47AE147B. */
    i960_mmio_write_u32(0x884000, 0x10002020u);
    sum = i960_ld_u32(I960_ABS, obj, 0x40);
    sum = f_add(sum, i960_ld_u32(I960_ABS, obj, 0x44));
    sum = f_add(sum, i960_ld_u32(I960_ABS, obj, 0x48));
    sum = f_add(sum, i960_ld_u32(I960_ABS, obj, 0x4c));
    /* mulrl sum×0.25, then subrl constant,src → src − constant, movr. */
    y = (u32)i960_f64_to_u32(
        i960_u32_to_f64(sum) * i960_rifl_read(0u, 0x3fd00000u)
        - i960_rifl_read(0x47ae147bu, 0x3f947ae1u));
    i960_mmio_write_u32(0x884000, 0x13802727u);
    i960_mmio_write_u32(0x884000, i960_ld_u32(I960_ABS, obj, 0));
    i960_mmio_write_u32(0x884000, y);
    i960_mmio_write_u32(0x884000, i960_ld_u32(I960_ABS, obj, 8));

    /* @0x24184: yaw obj+0x1c. */
    i960_mmio_write_u32(0x884000, 0x15002a2au);
    i960_mmio_write_u32(0x884000, i960_ld_u32(I960_ABS, obj, 0x1c));

    /* @0x241A0: pitch = ((+0x40+0x44) − (+0x48+0x4c)) * 0x3FCE76C8:B4395810. */
    diff = f_add(i960_ld_u32(I960_ABS, obj, 0x40),
                 i960_ld_u32(I960_ABS, obj, 0x44));
    diff = f_sub(diff, f_add(i960_ld_u32(I960_ABS, obj, 0x48),
                             i960_ld_u32(I960_ABS, obj, 0x4c)));
    pitch = (u32)i960_f64_to_u32(
        i960_u32_to_f64(diff) * i960_rifl_read(0xb4395810u, 0x3fce76c8u));
    i960_mmio_write_u32(0x884000, 0x14802929u);
    i960_mmio_write_u32(0x884000, pitch);

    /* @0x241F8–0x2423C: roll obj+0x20, then wr slot bind. */
    i960_mmio_write_u32(0x884000, 0x15802b2bu);
    wr = i960_ld_u32(I960_WORKRAM, 0x20a290, 0);
    i960_mmio_write_u32(0x884000, i960_ld_u32(I960_ABS, obj, 0x20));
    fifo_ptr = i960_ld_u32(I960_MMIO, 0x802008, 0);
    i960_st_u32(I960_WORKRAM, wr, 0, fifo_ptr);
    i960_mmio_write_u32(0x884000, 0x02800505u);
    i960_mmio_write_u32(0x801008, fifo_ptr + 0x34u);

    /* @0x24244: catalog pen+4, 0x20b940 += word3, pop. */
    bank_poke_g14();
    ix = i960_ld_u32(I960_ABS, pen, 4);
    catalog_quad(ix, cat);
    bank = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
    i960_st_u32(I960_WORKRAM, 0x20a290, 0, wr + 4u);
    i960_st_u32(I960_WORKRAM, 0x20b940, 0, bank + cat[3]);
    fifo_stq(cat);
    i960_mmio_write_u32(0x884000, 0x10802121u);

    /* @0x242B0: push + raw XYZ + yaw/pitch/roll. */
    i960_mmio_write_u32(0x884000, 0x10002020u);
    i960_mmio_write_u32(0x884000, 0x13802727u);
    i960_mmio_write_u32(0x884000, i960_ld_u32(I960_ABS, obj, 0));
    i960_mmio_write_u32(0x884000, i960_ld_u32(I960_ABS, obj, 4));
    i960_mmio_write_u32(0x884000, i960_ld_u32(I960_ABS, obj, 8));
    i960_mmio_write_u32(0x884000, 0x15002a2au);
    i960_mmio_write_u32(0x884000, i960_ld_u32(I960_ABS, obj, 0x1c));
    i960_mmio_write_u32(0x884000, 0x14802929u);
    i960_mmio_write_u32(0x884000, i960_ld_u32(I960_ABS, obj, 0x18));
    i960_mmio_write_u32(0x884000, 0x15802b2bu);
    wr = i960_ld_u32(I960_WORKRAM, 0x20a290, 0);
    i960_mmio_write_u32(0x884000, i960_ld_u32(I960_ABS, obj, 0x20));
    fifo_ptr = i960_ld_u32(I960_MMIO, 0x802008, 0);
    i960_st_u32(I960_WORKRAM, wr, 0, fifo_ptr);
    i960_mmio_write_u32(0x884000, 0x02800505u);
    bank = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
    i960_st_u32(I960_WORKRAM, 0x20a290, 0, wr + 4u);
    i960_mmio_write_u32(0x801008, fifo_ptr + 0x34u);

    /* @0x2438C: remain = budget − 0x20b940, clamp up to 1. */
    remain = budget - bank;
    if ((i32)remain < 1)
        remain = 1u;

    /*
     * @0x24398: catalog pen+0xc. remain==0 keeps g5 (the index) as word3.
     * Else cmpo word3,remain / bge stores remain, else the catalog word3.
     */
    bank_poke_g14();
    ix = i960_ld_u32(I960_ABS, pen, 0xc);
    catalog_quad(ix, cat);
    if (remain == 0u)
        cat[3] = ix;
    else if (cat[3] >= remain)
        cat[3] = remain;
    bank = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
    i960_st_u32(I960_WORKRAM, 0x20b940, 0, bank + cat[3]);
    fifo_stq(cat);

    /* @0x2442C: four triples at pen[0], stride 12. Yaw obj+0x24 for the first two. */
    cursor = pen0;
    ysrc = obj + 0x30u;
    for (i = 0; i < 4u; i++) {
        u32 xw = i960_ld_u32(I960_ABS, cursor, 0);
        u32 yw = f_add(i960_ld_u32(I960_ABS, cursor, 4),
                       i960_ld_u32(I960_ABS, ysrc, 0));
        u32 zw = i960_ld_u32(I960_ABS, cursor, 8);

        i960_mmio_write_u32(0x884000, 0x10002020u);
        i960_mmio_write_u32(0x884000, 0x13802727u);
        i960_mmio_write_u32(0x884000, xw);
        i960_mmio_write_u32(0x884000, yw);
        i960_mmio_write_u32(0x884000, zw);
        if ((i32)i <= 1) {
            i960_mmio_write_u32(0x884000, 0x15002a2au);
            i960_mmio_write_u32(0x884000, i960_ld_u32(I960_ABS, obj, 0x24));
        }
        wr = i960_ld_u32(I960_WORKRAM, 0x20a290, 0);
        fifo_ptr = i960_ld_u32(I960_MMIO, 0x802008, 0);
        i960_st_u32(I960_WORKRAM, wr, 0, fifo_ptr);
        i960_mmio_write_u32(0x884000, 0x02800505u);
        i960_mmio_write_u32(0x801008, fifo_ptr + 0x34u);
        bank_poke_g14();
        ix = i960_ld_u32(I960_ABS, pen, 8);
        catalog_quad(ix, cat);
        cursor += 12u;
        ysrc += 4u;
        bank = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
        fifo_stq(cat);
        i960_mmio_write_u32(0x884000, 0x10802121u);
        i960_st_u32(I960_WORKRAM, 0x20a290, 0, wr + 4u);
        i960_st_u32(I960_WORKRAM, 0x20b940, 0, bank + cat[3]);
    }

    /* @0x24530: pop, then object_extra when remain > 5. */
    bank = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
    remain = budget - bank;
    i960_mmio_write_u32(0x884000, 0x10802121u);
    if ((i32)remain > 5) {
        obj_host = model2_ram_mut(obj);
        if (obj_host)
            geo_attract_object_extra(obj_host, 0, 0);
    }
}
