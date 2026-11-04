/* obj_lemmy_ap.c - "lemmy_ap", the balls the hotel 4 boss throws, from L4_s10_sub.o
 * (m_0f6ee .. m_0ff38). Up to four sit idle in state 0 until lemmy sends one (state 3,
 * step 0, speed in unk_036, dir and position set by obj_lemmy.c's throw_ball()).
 * A ball rolls straight on Mario's level at its speed, now and then (1 in 16) dipping
 * (step 3, speed +2) or rising (step 2, speed -2) for 7 ticks, or at speed >= 11 doing
 * a bounce (step 1: 8 ticks of 4-bit dx/dy pairs from x_min/x_max, then speed -2).
 * It leaves at the screen sides (x < 8 or > 0x164), the top (y < 2) or the bottom.
 * Stomping it hurts Mario (cmd 3 returns 1); a fireball or star breaks it for 10 points.
 * Frames: 1-8 / 9-16 rolling (frame 1 / 9 when straight), 17-20 rising/dipping.
 */
#include "obj_common.h"

static void ap_boxes(actor_t *a, gds *g) {
    a->box_x0 = a->x + 7; a->box_x1 = a->x + 0xd;
    a->box_y0 = a->y + 7; a->box_y1 = a->y + 0xd;
    a->cur_floor = g->mario->floor;
}

static void ap_out(actor_t *a) { a->state = 4; a->step = 3; a->unk_02a = 2; }

static int ap_rest(actor_t *a, gds *g) { (void)g; a->unk_02a = 0; return 0; }   /* m_0f6ee, m_0f83c */

static int ap_moving(actor_t *a, gds *g) {              /* m_0f864 */
    CALL(a->walk_steps[a->step], a, g);
    if (a->state == 3) draw_at_xy(a, g, a->frame);
    return 0;
}

static int ap_roll(actor_t *a, gds *g) {                /* m_0f952 */
    if (a->dir == 1) { a->x -= a->unk_036; a->frame = 9; }
    else             { a->x += a->unk_036; a->frame = 1; }
    ap_boxes(a, g);
    if (a->unk_036 < 10) {
        if ((RND(g) & 0xf) == 1) { a->step = 3; a->unk_036 += 2; a->unk_02a = 0; }
    } else if (a->unk_036 < 0xb) {
        if ((RND(g) & 0xf) == 1) { a->step = 2; a->unk_036 -= 2; a->unk_02a = 0; }
    } else if ((RND(g) & 0xf) == 1) {
        a->step = 1; a->unk_02a = 0;
        a->x_min = -0x159dd952; a->x_max = -0x51159dda;
    }
    if (a->x < 8 || a->x > 0x164) ap_out(a);
    return 0;
}

static int ap_bounce(actor_t *a, gds *g) {              /* m_0faf6 */
    int dx, dy;
    a->frame = (int2)(a->unk_02a + (a->dir == 1 ? 9 : 1));
    if (++a->unk_02a > 7) { a->unk_036 -= 2; a->unk_02a = 0; a->step = 0; }
    dx = (int)(a->x_min & 0xf) - 8;
    dy = (int)(a->x_max & 0xf) - 8;
    a->x_min >>= 4; a->x_max >>= 4;
    a->x = (int2)(a->x + (a->dir == 1 ? -dx * 2 : dx * 2));
    a->y = (int2)(a->y + dy * 2);
    ap_boxes(a, g);
    if (a->x < 8 || a->x > 0x164 || a->y < 2) ap_out(a);
    return 0;
}

static int ap_rise(actor_t *a, gds *g) {                /* m_0fc3a */
    if (a->dir == 1) { a->frame = 0x13; a->x -= a->unk_036; }
    else             { a->frame = 0x11; a->x += a->unk_036; }
    a->y -= 2;
    ap_boxes(a, g);
    if (++a->unk_02a > 6) a->step = 0;
    if (a->x < 8 || a->x > 0x164 || a->y < 2) ap_out(a);
    return 0;
}

static int ap_dip(actor_t *a, gds *g) {                 /* m_0fd20 */
    if (a->dir == 1) { a->frame = 0x14; a->x -= a->unk_036; }
    else             { a->frame = 0x12; a->x += a->unk_036; }
    a->y += 2;
    ap_boxes(a, g);
    if (++a->unk_02a > 6) a->step = 0;
    if (a->x < 8 || a->x > 0x164 || a->y > 0xee) ap_out(a);
    return 0;
}

static int ap_stomped(actor_t *a, gds *g) { (void)a; (void)g; return 1; }   /* m_0fe06: hurts */

static int ap_destroy(actor_t *a, gds *g) {             /* m_0fe2a */
    if (g->unk_496 == 0) {
        if (a->unk_04c == 0 && g->unk_53a == 0) SCORE(g, 10);
        else a->unk_04c = 0;
        ap_out(a);
    } else a->unk_04e = a->cmd;
    return 0;
}

static int ap_gone_step(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }   /* m_0febe */

static int ap_gone(actor_t *a, gds *g) {                /* m_0fef8: back to rest */
    (void)g;
    if (a->unk_02a-- < 2) {
        a->state = 0; a->unk_04e = 0;
        a->box_x0 = a->box_x1 = a->box_y0 = a->box_y1 = 0;
    }
    return 0;
}

int lemmy_ap(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, ap_rest);
        H(0, 2, ap_rest);
        H(3, 1, obj_erase);
        H(3, 2, ap_moving);
        H(3, 3, ap_stomped);
        H(3, 4, ap_destroy);
        H(4, 1, obj_erase);
        H(4, 2, ap_gone_step);
        W(0, ap_roll);
        W(1, ap_bounce);
        W(2, ap_rise);
        W(3, ap_dip);
        B(3, ap_gone);
    }
    return obj_dispatch(a, g);
}
