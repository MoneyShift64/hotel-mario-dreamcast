/* obj_banzai.c - "banzai" (the bullet that shoots diagonally across the screen,
 * L4_s03_sub.o, m_127a0 .. m_13536). It ignores floors: it picks a start point, slides
 * it back along a 45-degree line to the screen edge, and if that edge point is more than
 * 93 px from Mario (in x or y) it appears there and flies 14 px/tick in both axes until
 * it leaves the screen. cur_floor follows Mario's floor, so it can always hit him.
 *   start_mode 0: from its own door the first time, then from random doors; 6 shots.
 *   start_mode 1: aimed at Mario (ahead of him when he walks); 0x5b shots.
 *   start_mode 2: waits in state 0 until something wakes it (state 1; f_goomba's dying
 *                 does that for a type-0x28 object), then behaves as start_mode 1.
 * x_min: 1 = flying down, 0 = up; x_max: its flying frame (1 + 4 down + 2 left);
 * the frame is one higher when it is within 94 px of Mario.
 * Stomped or hit: 500 points, falls off. Frames 1-8 flying, 9-11 falling, 12-14 appearing.
 */
#include "obj_common.h"

static void draw_frame(actor_t *a, gds *g) { draw_at_xy(a, g, a->frame); }   /* m_13536 */

static int bz_init(actor_t *a, gds *g) {                /* m_127a0 */
    if (a->start_mode < 2) a->state = 1;
    a->appear_count = (a->start_mode == 1) ? 0x5b : 6;
    a->delay = (int2)(6 * (*(int32_t *)(g->stage_info + 8) - a->delay));
    a->unk_018 = 1;
    return 0;
}

static int bz_again(actor_t *a, gds *g) {               /* m_128ee */
    (void)g;
    if (a->appear_count > 0 && a->start_mode < 2) { a->state = 1; a->delay = 0x32; a->unk_04e = 0; }
    return 0;
}

static int bz_wait(actor_t *a, gds *g) {                /* m_12944 */
    mario_t *m = g->mario;
    uint16_t u, v;
    if (g->unk_53a != 0) return 0;
    if (!(a->timer++ > a->delay || a->start_mode > 1)) return 0;
    if (a->start_mode == 0) {
        if (a->appear_count == 6) {
            a->x = (int2)g->door_x[a->col];
            a->y = (int2)(g->floor_y[a->floor] - 0x1d);
        } else {
            a->x = (int2)g->door_x[smod((int)(RND(g) & 0xffff), 7)];
            a->y = (int2)(g->floor_y[smod((int)(RND(g) & 0xffff), 5)] - 0x1d);
        }
    } else {
        int lead = (m->state_group == 0 && m->state == 0) ? 1 : (m->state_group == 0 && m->state == 1) ? -1 : 0;
        a->y = (int2)(g->floor_y[m->floor] - 0x1d);
        u = (uint16_t)((a->y < 0x81) ? 0xd8 - a->y : a->y - 2);
        a->x = (int2)(m->x + (u >> 1) * lead);
        if (a->x >= 0x13f) a->x = 0x13e;
        else if (a->x < 0x20) a->x = 0x20;
    }
    if (a->x < 0xc0) {
        a->dir = 1;
        u = (uint16_t)(0x15e - a->x);
        if (a->y < 0x81) {
            a->x_min = 0;
            v = (uint16_t)(0xd8 - a->y);
            if (v < u) { a->x = (int2)((a->x + v) & 0xfffe); a->y = 0xd8; }
            else { a->y = (int2)(a->y + u); a->x = 0x15e; }
        } else {
            a->x_min = 1;
            v = (uint16_t)(a->y - 2);
            if (v < u) { a->x = (int2)((a->x + v) & 0xfffe); a->y = 2; }
            else { a->y = (int2)(a->y - u); a->x = 0x15e; }
        }
    } else {
        a->dir = 0;
        if (a->y < 0x81) {
            a->x_min = 0;
            u = (uint16_t)(0xd8 - a->y);
            if (u < (uint16_t)a->x) { a->x = (int2)((a->x - u) & 0xfffe); a->y = 0xd8; }
            else { a->y = (int2)(a->x + a->y); a->x = 0; }
        } else {
            a->x_min = 1;
            u = (uint16_t)(a->y - 2);
            if (u < (uint16_t)a->x) { a->x = (int2)((a->x - u) & 0xfffe); a->y = 2; }
            else { a->y = (int2)(a->y - a->x); a->x = 0; }
        }
    }
    if (a->start_mode == 2) a->start_mode = 1;
    a->drawn_x0 = a->drawn_x1 = a->x;
    a->drawn_y0 = a->drawn_y1 = a->y;
    a->unk_03c = 1;
    if (a->x_min == 1) {
        a->box_y0 = a->y + 8;    a->box_y1 = a->y + 0x11;
        a->hit_y0 = a->y + 0x12; a->hit_y1 = a->y + 0x1b;
        if (a->dir == 1) { a->box_x0 = a->x + 0xf; a->box_x1 = a->x + 0x19; a->hit_x0 = a->x + 6; a->hit_x1 = a->x + 0xe; }
        else { a->box_x0 = a->x + 7; a->box_x1 = a->x + 0x11; a->hit_x0 = a->x + 0x12; a->hit_x1 = a->x + 0x1a; }
    } else {
        a->box_y0 = a->y + 6;   a->box_y1 = a->y + 0xe;
        a->hit_y0 = a->y + 0xf; a->hit_y1 = a->y + 0x19;
        if (a->dir == 1) { a->box_x0 = a->x + 6; a->box_x1 = a->x + 0xf; a->hit_x0 = a->x + 0x10; a->hit_x1 = a->x + 0x19; }
        else { a->box_x0 = a->x + 0x11; a->box_x1 = a->x + 0x1a; a->hit_x0 = a->x + 7; a->hit_x1 = a->x + 0x10; }
    }
    if (abs(a->x - m->x) > 0x5d || abs(a->y - m->y) > 0x5d) {
        a->appear_count--;
        a->state = 2; a->unk_02a = 0;
    }
    return 0;
}

static int bz_erase(actor_t *a, gds *g) {               /* m_13414: the HUD band needs a redraw */
    int y = (g->draw_field == 1) ? a->drawn_y0 : a->drawn_y1;
    if (y < 0x24 && 0x13 < y + 0x1d) g->unk_532 = 1;
    return 0;
}

static int bz_appear(actor_t *a, gds *g) {              /* m_12eb4 */
    a->frame = (int2)(a->unk_02a + 0xc);
    if (++a->unk_02a < 3) {
        if (a->frame == 0xc) g->unk_054 = 0xc;
    } else {
        a->frame = 1;
        if (a->x_min == 1) a->frame += 4;
        if (a->dir == 1) a->frame += 2;
        a->x_max = a->frame;
        if (g->unk_53a == 0) { a->state = 3; a->step = 0; }
        else { a->state = 4; a->step = 4; a->unk_02a = 0; }
    }
    draw_frame(a, g);
    return 0;
}

static int bz_moving(actor_t *a, gds *g) {              /* m_12f96 */
    int r = CALL(a->walk_steps[a->step], a, g);
    draw_frame(a, g);
    return r;
}

static void off_screen(actor_t *a) { a->state = 4; a->step = 3; a->unk_02a = 2; }

static int bz_fly(actor_t *a, gds *g) {                 /* m_12fe2 */
    int dx = (a->dir == 1) ? -0xe : 0xe, dy = (a->x_min == 1) ? 0xe : -0xe;
    a->x += dx; a->box_x0 += dx; a->box_x1 += dx; a->hit_x0 += dx; a->hit_x1 += dx;
    a->y += dy; a->box_y0 += dy; a->box_y1 += dy; a->hit_y0 += dy; a->hit_y1 += dy;
    if (a->x >= 0x15f) { a->x = 0x15e; off_screen(a); }
    else if (a->x < 0) { a->x = 0; off_screen(a); }
    if (a->y < 2) { a->y = 2; off_screen(a); }
    else if (a->y > 0xd8) { a->y = 0xd8; off_screen(a); }
    a->cur_floor = g->mario->floor;
    a->frame = (int2)a->x_max;
    if (abs(a->x - g->mario->x) < 0x5e && abs(a->y - g->mario->y) < 0x5e) a->frame++;
    return 0;
}

static int bz_hit(actor_t *a, gds *g) {                 /* m_131de: stomped */
    if (g->unk_496 == 0) { SCORE(g, 500); a->state = 4; a->step = 4; a->unk_02a = 0; }
    else a->unk_04e = a->cmd;
    return 0;
}

static int bz_destroy(actor_t *a, gds *g) {             /* m_13244 */
    if (g->unk_496 == 0) {
        if (a->unk_04c == 0 && g->unk_53a == 0) SCORE(g, 500);
        else a->unk_04c = 0;
        a->state = 4; a->step = 4; a->unk_02a = 0;
    } else a->unk_04e = a->cmd;
    return 0;
}

static int bz_dying(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }   /* m_132d8 */

static int bz_gone(actor_t *a, gds *g) {                /* m_133da */
    (void)g;
    if (a->unk_02a-- < 2) { a->state = 0; a->cmd = 2; a->timer = 0; a->dir = -1; }
    return 0;
}

static int bz_struck(actor_t *a, gds *g) {              /* m_13312 */
    draw_frame(a, g);
    if (++a->unk_02a > 1) { a->step = 5; a->unk_02a = 0; }
    return 0;
}

static int bz_fall(actor_t *a, gds *g) {                /* m_13356 */
    a->y += 0x10;
    if (a->y > 0xe5) a->y = 0xe5;
    a->frame = (int2)(a->unk_02a + 9);
    draw_frame(a, g);
    if (++a->unk_02a > 2) a->unk_02a = 0;
    if (a->y > 0xd4) { a->step = 3; a->unk_02a = 2; }
    return 0;
}

int banzai(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, bz_init);
        H(0, 2, bz_again);
        H(1, 2, bz_wait);
        H(2, 1, bz_erase);
        H(2, 2, bz_appear);
        H(3, 1, bz_erase);
        H(3, 2, bz_moving);
        H(3, 3, bz_hit);
        H(3, 4, bz_destroy);
        H(4, 1, bz_erase);
        H(4, 2, bz_dying);
        W(0, bz_fly);
        B(3, bz_gone);
        B(4, bz_struck);
        B(5, bz_fall);
    }
    return obj_dispatch(a, g);
}
