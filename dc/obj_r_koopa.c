/* obj_r_koopa.c - "r_koopa" (red koopa: Lemmy's hotel), from L4_s03_sub.o
 * (m_0437a .. m_05430). The koopa design, but it comes out after its delay (time-left
 * based) whatever Mario does, opening its door itself; doors it passes open 1 in 8;
 * 200 points; destroyed it flips and falls (frames 14-15, then 16-18).
 */
#include "obj_common.h"

static void at_door(actor_t *a, gds *g) {
    a->x = a->drawn_x0 = a->drawn_x1 = g->door_x[a->col];
    a->y = a->drawn_y0 = a->drawn_y1 = (int2)(g->floor_y[a->floor] - 0x1c);
    a->cur_floor = a->floor;
}

static void draw_frame(actor_t *a, gds *g) { draw_at_xy(a, g, a->frame); }   /* m_05430 */

static int rk_init(actor_t *a, gds *g) {                /* m_0437a */
    a->speed = HOTEL(g) == 8 ? 6 : 4;
    at_door(a, g);
    if (a->start_mode == 0) a->state = 1;
    else {
        a->unk_02a = 1; a->state = 3; a->step = 0;
        if (a->dir == -1) a->dir = (g->mario->x < a->x) ? 1 : 0;
    }
    a->appear_count = 1;
    a->unk_04a = a->unk_048 = a->appear_count + 1;
    a->timer = 0;
    if (a->delay != 0) a->delay = (int2)(6 * (*(int32_t *)(g->stage_info + 8) - a->delay));
    floor_limits(a, g, 0x4c, 0x22, 0x118, 0x142);
    return 0;
}

static int rk_dormant(actor_t *a, gds *g) {             /* m_046c4 */
    if (a->appear_count-- > 0) {
        at_door(a, g);
        a->state = 1;
        a->hit_y1 = a->hit_y0 = a->hit_x1 = a->hit_x0 = 0;
        a->box_y1 = a->box_y0 = a->box_x1 = a->box_x0 = 0;
        a->timer = 0;
        a->delay = 0x32;
        a->unk_04e = 0;
    }
    return 0;
}

static int rk_wait(actor_t *a, gds *g) {                /* m_0476e */
    uint16_t *c = &CELL(g, a->floor, a->col);
    if (g->unk_53a != 0 || a->timer++ <= a->delay || (*c & 0xf000) == 0) return 0;
    if (door_far_from_mario(a, g, a->x + 0xe, 0xe) > 0x48 || a->cur_floor != g->mario->floor) {
        if ((*c & 0xf000) == 0x1000) {
            *c = (*c & 0xfff) | 0x2009;
            g->unk_054 = 2;
        }
        a->state = 2; a->step = 0;
    }
    return 0;
}

static int rk_emerge(actor_t *a, gds *g) {              /* m_048ae */
    switch (a->step) {
    case 0: a->frame = 0x15; a->step = 1; CELL(g, a->floor, a->col) |= 4; break;
    case 1: a->frame = 0x16; a->step = 2; break;
    case 2:
        CELL(g, a->floor, a->col) = (CELL(g, a->floor, a->col) & ~4) | 2;
        a->frame = (a->dir == 1) ? 0xd : 0xb;
        a->unk_02a = 1; a->step = 0; a->state = 3;
        break;
    }
    draw_frame(a, g);
    return 0;
}

static int rk_moving(actor_t *a, gds *g) {              /* m_049e8 */
    int r = CALL(a->walk_steps[a->step], a, g);
    draw_frame(a, g);
    return r;
}

static int rk_walk(actor_t *a, gds *g) {                /* m_04ab8 */
    int2 x;
    walk_cycle(a);
    if (a->dir == 1) { x = (int2)(a->x - a->speed); a->frame = (int2)(a->unk_02a + 6); }
    else             { x = (int2)(a->x + a->speed); a->frame = (int2)(a->unk_02a + 1); }
    a->x = x;
    if (a->unk_02a == 0 && walker_follow(a, g, x, NULL)) { a->step = 1; a->unk_02a = 0; }
    if (x > a->x_max)      { x = a->x = (int2)a->x_max; a->step = 1; a->unk_02a = 0; }
    else if (x < a->x_min) { x = a->x = (int2)a->x_min; a->step = 1; a->unk_02a = 0; }
    a->box_y0 = a->y + 4;   a->box_y1 = a->y + 0xd;
    a->hit_y0 = a->y + 0xb; a->hit_y1 = a->y + 0x1b;
    if (a->dir == 1) { a->box_x0 = x + 6; a->box_x1 = x + 0x10; a->hit_x0 = x + 0xb; a->hit_x1 = x + 0x17; }
    else { a->box_x0 = x + 0xc; a->box_x1 = x + 0x16; a->hit_x0 = x + 5; a->hit_x1 = x + 0x11; }
    a->unk_03c = 1;
    maybe_open_door(a, g, x + 0xe, 7);
    return 0;
}

static int rk_turn(actor_t *a, gds *g) {                /* m_04ff2 */
    (void)g;
    a->unk_03c = 0;
    a->box_x0 = a->x + 8; a->box_x1 = a->x + 0x14;
    a->box_y0 = a->y + 3; a->box_y1 = a->y + 0x1b;
    a->frame = (int2)((a->dir == 1) ? 0xd - a->unk_02a : a->unk_02a + 0xb);
    if (++a->unk_02a > 2) {
        a->unk_02a = 1; a->step = 0;
        a->dir = (a->dir == 0) ? 1 : 0;
    }
    return 0;
}

static int rk_hit(actor_t *a, gds *g) {                 /* m_050b6 */
    if (g->unk_496 == 0) {
        SCORE(g, 200);
        a->unk_048--; a->unk_04a--;
        a->state = 4; a->step = 1; a->unk_02a = 0;
    } else a->unk_04e = a->cmd;
    return 0;
}

static int rk_destroy(actor_t *a, gds *g) {             /* m_05124 */
    if (g->unk_496 == 0) {
        if (a->unk_04c == 0 && g->unk_53a == 0) { SCORE(g, 200); a->unk_04a--; }
        else a->unk_04c = 0;
        a->state = 4; a->step = 4; a->unk_02a = 0;
    } else a->unk_04e = a->cmd;
    return 0;
}

static int rk_dying(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }

static int rk_hit_1(actor_t *a, gds *g) {               /* m_052b0 */
    a->frame = 0x13; draw_frame(a, g);
    if (++a->unk_02a > 0) a->step = 2;
    return 0;
}
static int rk_hit_2(actor_t *a, gds *g) {               /* m_052f6 */
    a->frame = 0x14; draw_frame(a, g);
    if (++a->unk_02a > 3) { a->step = 3; a->unk_02a = 2; }
    return 0;
}
static int rk_gone(actor_t *a, gds *g) {                /* m_05342 */
    (void)g;
    if (a->unk_02a-- < 2) { a->state = 0; a->dir = -1; }
    return 0;
}
static int rk_flip(actor_t *a, gds *g) {                /* m_051f6 */
    a->frame = (int2)(a->unk_02a + 0xe); draw_frame(a, g);
    if (++a->unk_02a > 1) { a->step = 5; a->unk_02a = 0; }
    return 0;
}
static int rk_fall(actor_t *a, gds *g) {                /* m_05244 */
    a->y += 0x10;
    a->frame = (int2)(a->unk_02a + 0x10); draw_frame(a, g);
    if (++a->unk_02a > 2) a->unk_02a = 0;
    if (a->y > 0xd5) { a->step = 3; a->unk_02a = 2; }
    return 0;
}

int r_koopa(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, rk_init);
        H(0, 2, rk_dormant);
        H(1, 2, rk_wait);
        H(2, 1, obj_erase);
        H(2, 2, rk_emerge);
        H(3, 1, obj_erase);
        H(3, 2, rk_moving);
        H(3, 3, rk_hit);
        H(3, 4, rk_destroy);
        H(4, 1, obj_erase);
        H(4, 2, rk_dying);
        W(0, rk_walk);
        W(1, rk_turn);
        B(1, rk_hit_1);
        B(2, rk_hit_2);
        B(3, rk_gone);
        B(4, rk_flip);
        B(5, rk_fall);
    }
    return obj_dispatch(a, g);
}
