/* Desert course flyers.
 *
 * Kind 5 init @ 0x44540 installs draw @ 0x442D0. One desert record,
 * span 0x10e. Body quad @ 0x028652A0, then keyframe table 0x5e2960,
 * then the 0x42/0x48/0x44/0x43 step that writes node+0x18/1c/20.
 *
 * Kind 3 init @ 0x44240 installs draw @ 0x43EA0. Phase in +0x60
 * selects keyframe tables 0x5e2830 / 0x5e2860 / 0x5e2840. 0x43C20
 * sets phase 2 when a car is within 42 of the flyer.
 *
 * Keyframe player @ 0x280D0. g0=table, g1=index, g2=step, g3=wrap.
 *
 * source: disasm/maincpu/maincpu_0280d0, 0442d0, 044540, 043ea0,
 *         044240, 043c20
 */
// @rom 0x280d0 +0x1f0 game_course_keyframe_draw
// @rom 0x442d0 +0x26c game_start_race_course_obj_draw_5
// @rom 0x44540 +0x4c  game_start_race_course_obj_init_5
// @rom 0x43ea0 +0x390 game_start_race_course_obj_draw_3
// @rom 0x44240 +0x90  game_start_race_course_obj_init_3
// @rom 0x43c20 +0x274 game_start_race_course_heli_near

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "model2_memory.h"
#include "model2_rom.h"

u32 geo_rng_u8(u32 arg0, u32 arg1, u32 arg2);

static void fifo(u32 w)
{
    i960_mmio_write_u32(0x884000u, w);
}

static u32 nd(u32 node, u32 off)
{
    return i960_ld_u32(I960_ABS, node, off);
}

static void ns(u32 node, u32 off, u32 v)
{
    i960_st_u32(I960_ABS, node, off, v);
}

static u32 mirror(u32 va)
{
    return model2_workram_mirror_u32(va);
}

static i32 ldis_at(u32 va)
{
    u32 w = i960_ld_u32(I960_ABS, va & ~3u, 0);
    u32 shift = (va & 2u) ? 16u : 0u;

    return (i32)(int16_t)((w >> shift) & 0xffffu);
}

static void bank_poke(u32 bits)
{
    u32 bank = i960_ld_u32(I960_WORKRAM, 0x202278u, 0) & 3u;

    i960_st_u32(I960_ABS, 0x00800010u + (bank << 10), 0, bits);
}

static void bind_matrix(void)
{
    u32 wr = i960_ld_u32(I960_WORKRAM, 0x20a290u, 0);
    u32 geo = i960_ld_u32(I960_ABS, 0x00802008u, 0);

    i960_st_u32(I960_WORKRAM, wr, 0, geo);
    fifo(0x02800505u);
    i960_st_u32(I960_WORKRAM, 0x20a290u, 0, wr + 4u);
    i960_st_u32(I960_ABS, 0x00801008u, 0, geo + 0x34u);
}

static void submit_quad(u32 base)
{
    u32 w0 = i960_ld_u32(I960_ABS, base, 0);
    u32 w1 = i960_ld_u32(I960_ABS, base, 4);
    u32 w2 = i960_ld_u32(I960_ABS, base, 8);
    u32 w3 = i960_ld_u32(I960_ABS, base, 12);
    u32 acc = i960_ld_u32(I960_WORKRAM, 0x20b940u, 0);

    i960_st_u32(I960_WORKRAM, 0x20b940u, 0, acc + w3);
    i960_mmio_write_u32(GEO_PRG_FIFO, w0);
    i960_mmio_write_u32(GEO_PRG_FIFO, w1);
    i960_mmio_write_u32(GEO_PRG_FIFO, w2);
    i960_mmio_write_u32(GEO_PRG_FIFO, w3);
}

static u32 fifo_ld(void)
{
    return i960_mmio_read_u32(0x884000u);
}

/* @0x44428 / @0x4411C. 0x25, yaw, 0x42/0x48/0x44/0x43, read xyz back, pop. */
static void fly_step(u32 node)
{
    u32 link = (u32)g14;

    fifo(0x12802525u);
    fifo(0x15002a2au);
    fifo(nd(node, 0x28u));
    fifo(0x21004242u);
    fifo(link);
    fifo(nd(node, 0x30u));
    fifo(nd(node, 0x34u));
    fifo(nd(node, 0x38u));
    fifo(0x24004848u);
    fifo(link);
    fifo(0x22004444u);
    fifo(link);
    fifo(nd(node, 0x18u));
    fifo(nd(node, 0x1cu));
    fifo(nd(node, 0x20u));
    fifo(0x21804343u);
    fifo(link);
    ns(node, 0x18u, fifo_ld());
    ns(node, 0x1cu, fifo_ld());
    ns(node, 0x20u, fifo_ld());
    fifo(0x10802121u);
}

/*
 * @0x280D0. Each piece is 0x14 bytes: xyz, then three signed halfwords
 * fed to 0x34 / 0x33 / 0x32. Catalog index is the next word at table+12.
 * Return is the stepped index, wrapped by g3.
 */
u32 game_course_keyframe_draw(u32 table, u32 index, u32 step, u32 wrap)
{
    u32 limit = mirror(table);
    u32 count = mirror(table + 4u);
    u32 base = mirror(table + 8u);
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
            u32 cat = mirror(catp);

            fifo(0x10002020u);
            fifo(0x13802727u);
            fifo(i960_ld_u32(I960_ABS, rec, 0));
            fifo(i960_ld_u32(I960_ABS, rec, 4));
            fifo(i960_ld_u32(I960_ABS, rec, 8));
            fifo(0x1a003434u);
            fifo((u32)ldis_at(rec + 12u));
            fifo(0x19803333u);
            fifo((u32)ldis_at(rec + 14u));
            fifo(0x19003232u);
            fifo((u32)ldis_at(rec + 16u));
            bind_matrix();
            bank_poke(link);
            submit_quad(CATALOG_VADDR + (cat << 4));
            fifo(0x10802121u);
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

void game_start_race_course_obj_draw_5(u32 arg0, u32 arg1, u32 arg2)
{
    u32 node = arg0 != 0u ? arg0 : (u32)g0;
    u32 link = (u32)g14;
    u32 idx;
    u32 step;

    (void)arg1;
    (void)arg2;

    fifo(0x10002020u);
    fifo(0x13802727u);
    fifo(nd(node, 0x18u));
    fifo(nd(node, 0x1cu));
    fifo(nd(node, 0x20u));
    fifo(0x15002a2au);
    fifo(nd(node, 0x28u));
    fifo(0x10002020u);
    fifo(0x13802727u);
    fifo(link);
    fifo(0x3dcccccdu);
    fifo(link);
    bind_matrix();
    bank_poke(link);
    submit_quad(0x028652a0u);
    fifo(0x10802121u);

    idx = nd(node, 0x54u);
    step = nd(node, 0x58u);
    ns(node, 0x54u, game_course_keyframe_draw(0x005e2960u, idx, step, 0u));
    fly_step(node);
}

void game_start_race_course_obj_init_5(u32 arg0, u32 arg1, u32 arg2)
{
    u32 node = arg0 != 0u ? arg0 : (u32)g0;
    u32 yvel = 0u;
    u8 tag;

    (void)arg1;
    (void)arg2;
    ns(node, 0x54u, 0u);
    ns(node, 0x58u, 1u);
    ns(node, 0x34u, 0u);
    ns(node, 0x30u, 0u);
    ns(node, 4u, 0x005e32d0u);
    tag = i960_ld_u8(I960_ABS, node, 2u);
    if ((tag & 4u) == 0u)
        yvel = 0x3c343958u;
    ns(node, 0x38u, yvel);
}

/* @0x43C20. Distance is TGP 0x50 of (heli.xz, car.xz) with g14 as Y. */
void game_start_race_course_heli_near(u32 arg0, u32 arg1, u32 arg2)
{
    u32 node = arg0 != 0u ? arg0 : (u32)g0;
    u32 timer = i960_ld_u32(I960_WORKRAM, 0x20a560u, 0);
    u32 link = (u32)g14;
    double lim = i960_rifl_read(0u, 0x40450000u);
    u32 car_yaw = 0u;
    int hit = 0;

    (void)arg1;
    (void)arg2;

    if ((i32)timer > 0) {
        u32 count = i960_ld_u32(I960_WORKRAM, 0x2139c0u, 0);
        u32 i;

        if ((i32)count <= 0)
            return;
        for (i = 0u; i < count; i++) {
            u32 car = i960_ld_u32(I960_WORKRAM, 0x213b00u + i * 4u, 0);
            double dist;

            fifo(0x28005050u);
            fifo(nd(node, 0x18u));
            fifo(link);
            fifo(nd(node, 0x20u));
            fifo(i960_ld_u32(I960_ABS, car, 0));
            fifo(link);
            fifo(i960_ld_u32(I960_ABS, car, 8u));
            dist = i960_u32_to_f64(fifo_ld());
            if (dist < lim) {
                car_yaw = i960_ld_u32(I960_ABS, car, 0x1cu);
                hit = 1;
                break;
            }
        }
    } else {
        u32 pose = 0x00213b40u;
        u32 end = 0x00214068u;
        u32 gi = 0u;

        while ((i32)pose <= (i32)end) {
            double dist;

            fifo(0x28005050u);
            fifo(nd(node, 0x18u));
            fifo(link);
            fifo(nd(node, 0x20u));
            fifo(i960_ld_u32(I960_ABS, pose, 0));
            fifo(link);
            fifo(i960_ld_u32(I960_WORKRAM, 0x213b48u + gi, 0));
            dist = i960_u32_to_f64(fifo_ld());
            if (dist < lim) {
                car_yaw = i960_ld_u32(I960_ABS, pose, 0x1cu);
                hit = 1;
                break;
            }
            pose += 0x58u;
            gi += 0x58u;
        }
    }

    if (!hit)
        return;

    /* @0x43D9C: phase 2, yaw = car yaw + (rng&0x7fff)/32767·π − π/2. */
    ns(node, 0x60u, 2u);
    {
        double unit = (double)(geo_rng_u8(node, 0u, 0u) & 0x7fffu)
                      / i960_rifl_read(0u, 0x40dfffc0u);
        double yaw = i960_u32_to_f64(car_yaw)
                     + unit * i960_rifl_read(0x54442d18u, 0x3ff921fbu)
                     - i960_rifl_read(0x54442d18u, 0x3fe921fbu);

        ns(node, 0x28u, (u32)i960_f64_to_u32(yaw));
    }
}

static void heli_phase_bump(u32 node, i32 delta)
{
    u32 phase = nd(node, 0x60u);

    ns(node, 0x60u, (u32)((i32)phase + delta));
}

void game_start_race_course_obj_draw_3(u32 arg0, u32 arg1, u32 arg2)
{
    u32 node = arg0 != 0u ? arg0 : (u32)g0;
    u32 yaw;
    u32 phase;
    u32 idx;

    (void)arg1;
    (void)arg2;

    fifo(0x10002020u);
    fifo(0x13802727u);
    fifo(nd(node, 0x18u));
    fifo(nd(node, 0x1cu));
    fifo(nd(node, 0x20u));
    /* @0x43EEC: yaw + π before 0x2a. */
    yaw = (u32)i960_f64_to_u32(
        i960_u32_to_f64(nd(node, 0x28u))
        + i960_rifl_read(0x54442d18u, 0x400921fbu));
    fifo(0x15002a2au);
    fifo(yaw);

    phase = nd(node, 0x60u);
    if (phase == 1u)
        goto phase1;
    if ((i32)phase > 1) {
        if (phase == 2u)
            goto phase2;
        goto bad;
    }
    if (phase != 0u)
        goto bad;

    idx = game_course_keyframe_draw(0x005e2830u, nd(node, 0x54u),
                                    nd(node, 0x58u), 0u);
    ns(node, 0x54u, idx);
    ns(node, 0x38u, (u32)g14);
    if (idx != 0u)
        goto near_then_fly;
    ns(node, 0x38u, 0x3e6147aeu);
    if ((geo_rng_u8(node, 0u, 0u) & 3u) != 0u) {
        ns(node, 0x28u, (u32)i960_f64_to_u32(
               i960_u32_to_f64(nd(node, 0x28u))
               + i960_rifl_read(0x4ae74487u, 0x3fe65718u)));
        goto near_then_fly;
    }
    heli_phase_bump(node, 1);
    goto near_then_fly;

phase1:
    idx = game_course_keyframe_draw(0x005e2860u, nd(node, 0x54u),
                                    nd(node, 0x58u), 0u);
    ns(node, 0x54u, idx);
    ns(node, 0x38u, (u32)g14);
    if (idx != 20u)
        goto near_then_fly;
    if ((geo_rng_u8(node, 0u, 0u) & 3u) != 0u)
        goto near_then_fly;
    heli_phase_bump(node, -1);
    goto near_then_fly;

phase2:
    if (nd(node, 0x64u) > 0x12cu)
        goto fly;
    idx = game_course_keyframe_draw(0x005e2840u, nd(node, 0x54u),
                                    nd(node, 0x58u), 0u);
    ns(node, 0x54u, idx);
    {
        u32 v = nd(node, 0x34u);

        if (i960_u32_to_f64(v) < i960_rifl_read(0x11111111u, 0x3fc11111u))
            v = (u32)i960_f64_to_u32(
                i960_u32_to_f64(v)
                + i960_rifl_read(0x11111111u, 0x3f711111u));
        ns(node, 0x34u, v);
    }
    {
        u32 v = nd(node, 0x38u);

        if (i960_u32_to_f64(v) < i960_rifl_read(0x11111111u, 0x3fe11111u))
            v = (u32)i960_f64_to_u32(
                i960_u32_to_f64(v)
                + i960_rifl_read(0x7e4b17e4u, 0x3f97e4b1u));
        ns(node, 0x38u, v);
    }
    ns(node, 0x64u, nd(node, 0x64u) + 1u);
    goto fly;

bad:
    ns(node, 0x60u, (u32)g14);
    goto fly;

near_then_fly:
    game_start_race_course_heli_near(node, 0u, 0u);
fly:
    fly_step(node);
}

void game_start_race_course_obj_init_3(u32 arg0, u32 arg1, u32 arg2)
{
    u32 node = arg0 != 0u ? arg0 : (u32)g0;
    u32 link = (u32)g14;
    double yaw;

    (void)arg1;
    (void)arg2;
    ns(node, 0x60u, link);
    ns(node, 0x64u, link);
    ns(node, 0x30u, link);
    ns(node, 0x34u, link);
    ns(node, 0x38u, link);
    ns(node, 4u, 0x005e2ea0u);
    ns(node, 0x54u, link);
    ns(node, 0x58u, 1u);
    /* (rng & 0xff) / 255 · 2π → +0x28. */
    yaw = (double)(geo_rng_u8(node, 0u, 0u) & 0xffu)
          / i960_rifl_read(0u, 0x406fe000u);
    yaw *= i960_rifl_read(0x54442d18u, 0x401921fbu);
    ns(node, 0x28u, (u32)i960_f64_to_u32(yaw));
}
