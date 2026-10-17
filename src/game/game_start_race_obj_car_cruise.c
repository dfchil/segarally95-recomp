/* Rival e8==2 body @ 0x2C02C and cruise @ 0x2EEF0.
 *
 * car_frame @ 0x2C020 calls road_span then falls into this body and
 * calls 0x2EEF0. That call is the per-frame pitch/yaw/roll update
 * (road_angles into the caller frame, then the 0x20cac8 blend).
 *
 * call 0x311E0 leaves g14 = 0x2C02C. road_span does not write g14, so
 * every `st g14` in the body stores that return address.
 *
 * source: disasm/maincpu/maincpu_02bd00_800.asm @ 0x2C020,
 *         disasm/maincpu/maincpu_02eef0_400.asm
 */
// @rom 0x2eef0 +0x4c0 game_start_race_obj_car_cruise

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "model2_hw.h"
#include "model2_rom.h"

#include "lift_syms.h"

#include <string.h>

/* Return address of `call 0x311E0` at car_frame 0x2C028. */
#define E8_MODE2_LINK 0x2c02cu

void game_start_race_obj_car_index_refresh(u32 node, u32 pred[3]);
void game_start_race_obj_road_angles_host(void *samp, u32 nrm, void *ang);

static u32 fld(u32 off)
{
    u32 v;

    memcpy(&v, (u8 *)fp + off, 4);
    return v;
}

static void fst(u32 off, u32 v)
{
    memcpy((u8 *)fp + off, &v, 4);
}

static u32 nd(u32 node, u32 off)
{
    return i960_ld_u32(I960_ABS, node, off);
}

static void ns(u32 node, u32 off, u32 v)
{
    i960_st_u32(I960_ABS, node, off, v);
}

static u32 f_add(u32 a, u32 b)
{
    return (u32)i960_f64_to_u32(i960_u32_to_f64(a) + i960_u32_to_f64(b));
}

static u32 f_sub(u32 a, u32 b)
{
    return (u32)i960_f64_to_u32(i960_u32_to_f64(a) - i960_u32_to_f64(b));
}

static u32 f_mul(u32 a, u32 b)
{
    return (u32)i960_f64_to_u32(i960_u32_to_f64(a) * i960_u32_to_f64(b));
}

static void fifo(u32 w)
{
    i960_mmio_write_u32(0x884000u, w);
}

static u32 fifo_ld(void)
{
    return i960_mmio_read_u32(0x884000u);
}

static u32 half_f(u32 bits)
{
    double h = i960_u32_to_f64(bits) * i960_rifl_read(0u, 0x3fe00000u);

    return (u32)i960_f64_to_u32(h);
}

static u32 blend_ang(u32 sample, u32 current, u32 lo, u32 hi)
{
    double mixed;

    /* bal 0x27BD8: g0 = wrapped (current − sample). */
    tile_attract_angle_fixup(sample, current);
    mixed = i960_u32_to_f64(sample)
        + i960_u32_to_f64((u32)g0) * i960_rifl_read(lo, hi);
    return (u32)i960_f64_to_u32(mixed);
}

/* @0x2EF74–0x2F13A: surface response. Returns r14. */
static u32 cruise_surface(u32 node)
{
    u32 w7c = fld(0x7cu);
    u32 g6 = w7c & 0xffffu;
    u32 g7 = w7c & 0xffu;
    u32 scaled;
    u32 r14;

    /* shlo 12 + shlo 8 + *16 + g6 → stos node+0x12. */
    scaled = (g6 << 12) + (g6 << 8);
    scaled = scaled + (g6 << 4) + g6;
    i960_st_u16(I960_ABS, node, 0x12u, (u16)scaled);

    /* @0x2EF94 bbc 7,g7 — bit clear takes the short path. */
    if ((g7 & 0x80u) == 0u) {
        fst(0x44u, f_add(fld(0x78u), fld(0x44u)));
        return i960_ld_u32(I960_WORKRAM, 0x2141ecu, 0);
    }

    {
        u32 uz = fld(0x74u);
        u32 sc = fld(0x78u);
        u32 ux = fld(0x6cu);
        double half = i960_u32_to_f64(sc) * i960_rifl_read(0u, 0x3fe00000u);
        u32 hx = (u32)i960_f64_to_u32(
            i960_u32_to_f64(nd(node, 0x20u)) + half * i960_u32_to_f64(ux));
        u32 hz = (u32)i960_f64_to_u32(
            i960_u32_to_f64(nd(node, 0x28u)) + half * i960_u32_to_f64(uz));
        u32 scale_bits;
        double gap;
        u32 a, b, c;
        u32 read2;
        u32 abs_r;

        ns(node, 0x20u, hx);
        ns(node, 0x28u, hz);

        /* @0x2F018 mulr (fp+0x78), 0x2141f0; clamp (K − scale) at 0. */
        scale_bits = f_mul(sc, i960_ld_u32(I960_WORKRAM, 0x2141f0u, 0));
        gap = i960_rifl_read(0x8b439581u, 0x3fefe76cu)
            - i960_u32_to_f64(scale_bits);
        if (gap < 0.0)
            gap = 0.0;

        /* @0x2F07C TGP 0x5b, six words, three readbacks. */
        fifo(0x2d805b5bu);
        fifo(fld(0x70u));
        fifo(i960_ld_u32(I960_WORKRAM, 0x20cad8u, 0));
        fifo(fld(0x74u));
        fifo(i960_ld_u32(I960_WORKRAM, 0x20cad4u, 0));
        fifo(i960_ld_u32(I960_WORKRAM, 0x20cad0u, 0));
        fifo(fld(0x6cu));
        a = fifo_ld();
        b = fifo_ld();
        c = fifo_ld();
        fst(0x70u, b);
        fst(0x6cu, a);

        read2 = b;
        r14 = (u32)i960_f64_to_u32(
            gap * i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x2141ecu, 0)));
        /* @0x2F0F4 cmpr +0.0; bge keeps the sign, else notbit 31. */
        if (i960_u32_to_f64(read2) < 0.0)
            abs_r = read2 ^ 0x80000000u;
        else
            abs_r = read2;
        fst(0x70u, abs_r);
        fst(0x74u, c);
        fst(0x44u, nd(node, 0x94u));
    }
    return r14;
}

/* @0x2F13C–0x2F3B0: blend predicted XYZ, write angles, scale velocity. */
static void cruise_body(u32 node, u32 r14)
{
    double k03 = i960_rifl_read(0x33333334u, 0x3fd33333u);
    u32 w6c = fld(0x6cu);
    u32 w70 = fld(0x70u);
    u32 w74 = fld(0x74u);
    u32 px = fld(0x40u);
    u32 py = fld(0x44u);
    u32 pz = fld(0x48u);
    u32 x = (u32)i960_f64_to_u32(i960_u32_to_f64(px) + i960_u32_to_f64(w6c) * k03);
    u32 y = (u32)i960_f64_to_u32(i960_u32_to_f64(py) + i960_u32_to_f64(w70) * k03);
    u32 z = (u32)i960_f64_to_u32(i960_u32_to_f64(pz) + i960_u32_to_f64(w74) * k03);
    u32 snap;
    u32 pitch;
    u32 yaw;
    u32 roll;
    u32 s15;
    u32 scaled;
    u32 vx, vy, vz;
    u32 sum;
    u32 diff;
    static int logged;

    ns(node, 0x70u, 0x3dcccccdu);
    ns(node, 0x6cu, 0x3dcccccdu);
    ns(node, 0x68u, 0x3dcccccdu);
    ns(node, 0x64u, 0x3dcccccdu);
    ns(node, 0x14u, x);
    ns(node, 0x18u, y);
    ns(node, 0x1cu, z);

    /*
     * @0x2F1F4: g0 = fp+0x60 (index_b sample), g1 = node+0x2c,
     * g2 = fp+0x50 (angle out). Sample and angles are caller-frame.
     */
    g0 = (u32)(uintptr_t)((u8 *)fp + 0x60u);
    g1 = node + 0x2cu;
    g2 = (u32)(uintptr_t)((u8 *)fp + 0x50u);
    game_start_race_obj_road_angles_host((u8 *)fp + 0x60u, node + 0x2cu,
                                         (u8 *)fp + 0x50u);

    /* @0x2F20C ld 0x20cac8; cmpibe 0 → blend, else copy the sample. */
    snap = i960_ld_u32(I960_WORKRAM, 0x20cac8u, 0);
    pitch = fld(0x50u);
    yaw = fld(0x54u);
    roll = fld(0x58u);
    if (snap != 0u) {
        ns(node, 0x38u, pitch);
        ns(node, 0x3cu, yaw);
        ns(node, 0x40u, roll);
    } else {
        /* @0x2F228 pitch and roll × 0.95; yaw × 0.9. */
        ns(node, 0x38u,
           blend_ang(pitch, nd(node, 0x38u), 0x66666666u, 0x3fee6666u));
        ns(node, 0x40u,
           blend_ang(roll, nd(node, 0x40u), 0x66666666u, 0x3fee6666u));
        ns(node, 0x3cu,
           blend_ang(yaw, nd(node, 0x3cu), 0xcccccccdu, 0x3fecccccu));
    }

    if (!logged) {
        lift_log(
                "lift: car_cruise node=%#x snap=%u yaw=%.3g pitch=%.3g\n",
                node, snap,
                i960_u32_to_f64(nd(node, 0x3cu)),
                i960_u32_to_f64(nd(node, 0x38u)));
        fflush(stderr);
        logged = 1;
    }

    /* @0x2F308 TGP 0x15 of pitch; × 1.25; scale vel by r14. */
    fifo(0x0a801515u);
    fifo(nd(node, 0x38u));
    s15 = fifo_ld();
    scaled = (u32)i960_f64_to_u32(
        i960_u32_to_f64(s15) * i960_rifl_read(0u, 0x3ff40000u));
    vx = f_mul(r14, nd(node, 0x2cu));
    vz = f_mul(r14, nd(node, 0x34u));
    vy = f_mul(r14, nd(node, 0x30u));
    ns(node, 0x2cu, vx);
    ns(node, 0x30u, vy);
    ns(node, 0x34u, vz);
    sum = f_add(scaled, fld(0x44u));
    diff = f_sub(fld(0x44u), scaled);
    ns(node, 0xb8u, sum);
    ns(node, 0xb4u, sum);
    ns(node, 0xc0u, diff);
    ns(node, 0xbcu, diff);
}

void game_start_race_obj_car_cruise(u32 node, u32 arg1, u32 arg2)
{
    u32 pred[3];
    i32 index;
    u32 r14;

    (void)arg1;
    (void)arg2;
    if (node == 0u)
        return;

    /* @0x2EF08: predicted = (x+vx, y, z+vz) at caller fp+0x40. */
    pred[0] = f_add(nd(node, 0x14u), nd(node, 0x2cu));
    pred[1] = nd(node, 0x18u);
    pred[2] = f_add(nd(node, 0x1cu), nd(node, 0x34u));
    fst(0x40u, pred[0]);
    fst(0x44u, pred[1]);
    fst(0x48u, pred[2]);

    /* @0x2EF48 call 0x30320. g0=node, g1=predicted, g2=&node+0x88. */
    g0 = node;
    g1 = (u32)(uintptr_t)((u8 *)fp + 0x40u);
    g2 = node + 0x88u;
    game_start_race_obj_car_index_refresh(node, pred);
    fst(0x40u, pred[0]);
    fst(0x44u, pred[1]);
    fst(0x48u, pred[2]);

    /* @0x2EF4C ldis +0x88; ld 0x214354; call table_index_a / _b. */
    index = (i32)(int16_t)i960_ld_u16(I960_ABS, node, 0x88u);
    g0 = i960_ld_u32(I960_WORKRAM, 0x214354u, 0);
    g1 = (u32)index;
    g2 = (u32)(uintptr_t)((u8 *)fp + 0x40u);
    g3 = 0;
    geo_view_table_index_a((void *)(uintptr_t)g0, (u32)g1,
                           (u8 *)fp + 0x40u);
    g0 = (u32)(uintptr_t)((u8 *)fp + 0x40u);
    g1 = (u32)(uintptr_t)((u8 *)fp + 0x60u);
    g2 = 0;
    geo_view_table_index_b((u8 *)fp + 0x40u, (u8 *)fp + 0x60u, 0);

    r14 = cruise_surface(node);
    cruise_body(node, r14);
}

void game_start_race_obj_car_e8_mode2(u32 node, u32 span_base)
{
    const u32 link = E8_MODE2_LINK;
    u32 yaw;
    u32 vx, vz;
    u32 r0, r1, r2;
    u32 hx, hz;
    u32 ac;
    u32 lim;
    u32 coeff;
    u32 flag;
    u32 len;
    u32 b0;
    u32 n9c, na4;
    u32 dot;
    double w;
    u32 bx, by, bz;
    u32 nx, nz;

    if (node == 0u)
        return;

    g0 = node;
    g1 = span_base;
    game_start_race_obj_road_span(node, span_base, 0);
    /* Callee does not write g14; CALL left the return address. */
    g14 = link;

    /* @0x2C02C push, 0x25, 0x2a(notbit 31 of node+0x3c). */
    fifo(0x10002020u);
    fifo(0x12802525u);
    yaw = nd(node, 0x3cu) ^ 0x80000000u;
    fifo(0x15002a2au);
    fifo(yaw);

    /* @0x2C06C 0x2c(vx, g14, vz); third readback → +0x80 and +0x84. */
    vx = nd(node, 0x2cu);
    vz = nd(node, 0x34u);
    fifo(0x16002c2cu);
    fifo(vx);
    fifo(link);
    fifo(vz);
    r0 = fifo_ld();
    r1 = fifo_ld();
    r2 = fifo_ld();
    fst(0x48u, r2);
    fst(0x40u, r0);
    fst(0x44u, r1);
    ns(node, 0x80u, r2);
    ns(node, 0x84u, f_add(nd(node, 0x84u), r2));

    fifo(0x10802121u);
    ns(node, 0xacu, nd(node, 0xd8u));

    /* @0x2C0F4 half +0x20 and +0x28, fold into xyz and vel, then st g14. */
    hx = half_f(nd(node, 0x20u));
    hz = half_f(nd(node, 0x28u));
    ns(node, 0x20u, hx);
    ns(node, 0x28u, hz);
    ns(node, 0x14u, f_add(nd(node, 0x14u), hx));
    ns(node, 0x1cu, f_add(nd(node, 0x1cu), hz));
    ns(node, 0x2cu, f_add(nd(node, 0x2cu), hx));
    ns(node, 0x34u, f_add(nd(node, 0x34u), hz));
    ns(node, 0x20u, link);
    ns(node, 0x24u, link);
    ns(node, 0x28u, link);

    /* @0x2C1A0 ldl vel → +0xc4/+0xc8, +0x34 → +0xcc. */
    ns(node, 0xc4u, nd(node, 0x2cu));
    ns(node, 0xc8u, nd(node, 0x30u));
    ns(node, 0xccu, nd(node, 0x34u));

    /* @0x2C1B0 delta vs span basis, TGP 0x2f → +0x9c/+0xa0/+0xa4. */
    vx = f_sub(nd(node, 0x90u), nd(node, 0x14u));
    vz = f_sub(nd(node, 0x98u), nd(node, 0x1cu));
    fst(0x40u, vx);
    fst(0x44u, link);
    fifo(0x17802f2fu);
    fifo(vx);
    fifo(link);
    fifo(vz);
    ns(node, 0x9cu, fifo_ld());
    ns(node, 0xa0u, fifo_ld());
    ns(node, 0xa4u, fifo_ld());

    /* @0x2C21C (node+0xac) * double const vs +0xb0 → bit 4 and coeff. */
    ac = nd(node, 0xacu);
    lim = (u32)i960_f64_to_u32(
        i960_u32_to_f64(ac) * i960_rifl_read(0xd714cf45u, 0x3f72f684u));
    b0 = nd(node, 0xb0u);
    flag = i960_ld_u8(I960_ABS, node, 0x10u);
    if (i960_u32_to_f64(lim) <= i960_u32_to_f64(b0)) {
        flag = (u8)(flag | 16u);
        coeff = i960_ld_u32(I960_WORKRAM, 0x20cae8u, 0);
    } else {
        flag = (u8)(flag & (u8)~16u);
        coeff = i960_ld_u32(I960_WORKRAM, 0x20cae4u, 0);
    }
    i960_st_u8(I960_ABS, node, 0x10u, (u8)flag);

    /* @0x2C2A0 TGP 0x58 of (g14, g14, g14, vx, link, vz) → new +0xb0. */
    fst(0x40u, nd(node, 0x2cu));
    fst(0x44u, link);
    fst(0x48u, nd(node, 0x34u));
    fifo(0x2c005858u);
    fifo(link);
    fifo(link);
    fifo(link);
    fifo(fld(0x40u));
    fifo(fld(0x44u));
    fifo(fld(0x48u));
    len = fifo_ld();
    b0 = f_sub(lim, f_mul(f_sub(lim, len), coeff));
    ns(node, 0xb0u, b0);

    /* @0x2C304 0x2f of that triple, then blend with +0x9c/+0xa4. */
    fifo(0x17802f2fu);
    fifo(fld(0x40u));
    fifo(fld(0x44u));
    fifo(fld(0x48u));
    r0 = fifo_ld();
    r1 = fifo_ld();
    r2 = fifo_ld();
    fst(0x40u, r0);
    fst(0x44u, r1);
    fst(0x48u, r2);
    n9c = nd(node, 0x9cu);
    na4 = nd(node, 0xa4u);
    dot = f_add(f_mul(n9c, r0), f_mul(na4, r2));
    w = 1.0 - i960_u32_to_f64(dot);
    if (w > 1.0)
        w = 1.0;
    {
        u32 wb = (u32)i960_f64_to_u32(w);

        bx = f_add(r0, f_mul(f_sub(n9c, r0), wb));
        bz = f_add(r2, f_mul(f_sub(na4, r2), wb));
    }
    by = r1;
    fst(0x40u, bx);
    fst(0x48u, bz);

    /* @0x2C3B8 second 0x2f; × +0xb0 into vel x/z. */
    fifo(0x17802f2fu);
    fifo(bx);
    fifo(by);
    fifo(bz);
    nx = fifo_ld();
    by = fifo_ld();
    nz = fifo_ld();
    b0 = nd(node, 0xb0u);
    ns(node, 0x2cu, f_mul(nx, b0));
    ns(node, 0x34u, f_mul(nz, b0));
    (void)by;

    g0 = node;
    g1 = span_base;
    game_start_race_obj_car_cruise(node, span_base, 0);
}
