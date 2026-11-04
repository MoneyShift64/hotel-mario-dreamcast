/* obj_m_koopa.c - "m_koopa" (the wind-up mecha koopa), from L7_s04_sub.o
 * (m_0ee44 .. m_10110). Comes out of its door (when Mario is more than 72 pixels away
 * or on another floor) and walks between the floor's walls (x_min/x_max: further in
 * at a lift), turning to follow Mario like the other walkers. It has a spring of
 * 650..681 ticks (unk_036); when that runs down it stops now and then (step 2) and
 * after 40 more ticks, unless it's standing at a lift, it stops for good (step 4).
 * Stomping it doesn't kill it: it's knocked over for 24 ticks (step 3) and pushed 4
 * pixels in the direction Mario was facing; a run-down one is only pushed. A fireball
 * or star (cmd 4) destroys it: 500 points, and it drops off the screen for good.
 * Frames: 1-4 walk right, 5-8 left, 9-11 turn, 12-15 knocked over, 16/20 run down,
 * 17/21 run down and stomped, 16-21 falling, 24 out of the door.
 * unk_25c: int32 push for the next tick.
 */
#include "obj_common.h"

static void draw_frame(actor_t *a, gds *g) { draw_at_xy(a, g, a->frame); }   /* m_10110 */

static int mk_init(actor_t *a, gds *g) {                /* m_0ee44 */
    uint16_t l0, l6;
    a->speed = HOTEL(g) == 8 ? 4 : 2;
    a->cur_floor = a->floor;
    a->x = a->drawn_x0 = a->drawn_x1 = g->door_x[a->col];
    a->y = a->drawn_y0 = a->drawn_y1 = (int2)(g->floor_y[a->floor] - 0x1e);
    a->timer = 0;
    if (a->delay != 0) a->delay = (int2)(6 * (*(int32_t *)(g->stage_info + 8) - a->delay));
    a->state = 1;
    l0 = CELL(g, a->floor, 0) & 0xf000;
    l6 = CELL(g, a->floor, 6) & 0xf000;
    a->x_min = (l0 == 0xf000 || l0 == 0xe000) ? 0x4c : 0x22;
    a->x_max = (l6 == 0xf000 || l6 == 0xe000) ? 0x114 : 0x13e;
    return 0;
}

static int mk_wait(actor_t *a, gds *g) {                /* m_0f152 */
    uint16_t *c;
    if (g->unk_53a != 0 || a->timer++ <= a->delay) return 0;
    c = &CELL(g, a->floor, a->col);
    if ((*c & 0xf000) == 0) return 0;
    if (door_far_from_mario(a, g, a->x + 0x10, 0xe) > 0x48 || a->cur_floor != g->mario->floor) {
        if ((*c & 0xf000) == 0x1000) { *c = (*c & 0xfff) | 0x2009; g->unk_054 = 2; }
        a->state = 2; a->step = 0;
    }
    return 0;
}

static int mk_emerge(actor_t *a, gds *g) {              /* m_0f292 */
    switch (a->step) {
    case 0: a->frame = 0x18; a->step = 1; CELL(g, a->floor, a->col) |= 4; break;
    case 1: a->frame = 10; a->step = 2; break;
    case 2:
        CELL(g, a->floor, a->col) = (CELL(g, a->floor, a->col) & 0xfffb) | 2;
        a->frame = (a->dir == 1) ? 0xb : 9;
        a->unk_02a = 0; a->step = 0; a->state = 3;
        a->unk_036 = (int2)((RND(g) & 0x1f) + 0x28a);
        break;
    }
    draw_frame(a, g);
    return 0;
}

static int mk_moving(actor_t *a, gds *g) {              /* m_0f3e8 */
    int r = CALL(a->walk_steps[a->step], a, g);
    draw_at(a, g, a->frame);
    return r;
}

static void knocked_box(actor_t *a) {
    a->unk_03c = 0;
    a->box_x0 = a->x + 9; a->box_x1 = a->x + 0x17;
    a->box_y0 = a->y + 10; a->box_y1 = a->y + 0x1d;
}

static int mk_walk(actor_t *a, gds *g) {                /* m_0f4b8 */
    int x;
    uint16_t *door;
    if (a->dir == 1) { a->x -= a->speed; a->frame = (int2)(a->unk_02a + 5); }
    else             { a->x += a->speed; a->frame = (int2)(a->unk_02a + 1); }
    x = a->x;
    if (++a->unk_02a > 3) a->unk_02a = 0;
    if (a->unk_02a == 0 && walker_follow(a, g, x, NULL)) { a->step = 1; a->unk_02a = 0; }
    if (--a->unk_036 < 0 && (RND(g) & 7) == 0) a->step = 2;
    if (x > a->x_max) { x = a->x = (int2)a->x_max; a->step = 1; a->unk_02a = 0; }
    else if (x < a->x_min) { x = a->x = (int2)a->x_min; a->step = 1; a->unk_02a = 0; }
    a->box_y0 = a->y + 8; a->box_y1 = a->y + 0x13;
    a->hit_y0 = a->y + 0xe; a->hit_y1 = a->y + 0x1d;
    if (a->dir == 1) { a->box_x0 = x + 4; a->box_x1 = x + 0x10; a->hit_x0 = x + 0xb; a->hit_x1 = x + 0x19; }
    else { a->box_x0 = x + 0x10; a->box_x1 = x + 0x1c; a->hit_x0 = x + 7; a->hit_x1 = x + 0x15; }
    a->unk_03c = 1;
    door = DOOR_CELL(a->cur_floor, x + 0x10);
    if (door && (*door & 0xf000) == 0x1000 && (RND(g) & 0x1f) == 1) {
        *door = (*door & 0xfff) | 0x2009;
        g->unk_054 = 2;
    }
    return 0;
}

static int mk_turn(actor_t *a, gds *g) {                /* m_0fc80 */
    (void)g;
    a->box_x0 = a->x + 0xb; a->box_x1 = a->x + 0x15;
    a->box_y0 = a->y + 6; a->box_y1 = a->y + 0x1d;
    a->hit_x0 = a->x + 7; a->hit_x1 = a->x + 0x19;
    a->hit_y0 = a->y + 0x12; a->hit_y1 = a->y + 0x15;
    a->unk_03c = 1;
    a->frame = (int2)((a->dir == 1) ? 0xb - a->unk_02a : a->unk_02a + 9);
    if (++a->unk_02a > 2) {
        a->unk_02a = 0; a->step = 0;
        a->dir = (a->dir == 0) ? 1 : 0;
    }
    return 0;
}

static int at_lift(gds *g, int floor, int x) {
    uint16_t *c = DOOR_CELL(floor, x);
    return c && ((*c & 0xf000) == 0xf000 || (*c & 0xf000) == 0xe000);
}

static int mk_pause(actor_t *a, gds *g) {               /* m_0f9de: the spring's run down */
    if (--a->unk_036 < -0x28 && !at_lift(g, a->cur_floor, a->x + 0x18) && !at_lift(g, a->cur_floor, a->x + 8)) {
        a->step = 4;
        knocked_box(a);
        a->frame = (a->dir == 1) ? 0x14 : 0x10;
    } else if ((RND(g) & 0xf) == 0) a->step = 0;
    return 0;
}

static int mk_knocked(actor_t *a, gds *g) {             /* m_0fb58 */
    (void)g;
    if (++a->unk_02c > 1) {
        a->unk_02c = 0;
        if (++a->unk_02a > 0xb) { a->step = 0; a->unk_02a = 0; }
        a->frame = (int2)((a->unk_02a & 1) + (a->dir == 1 ? 0xe : 0xc));
    }
    a->x += (int2)rd25c(a, 0);
    wr25c(a, 0, 0);
    knocked_box(a);
    return 0;
}

static int mk_stopped(actor_t *a, gds *g) {             /* m_0fc24 */
    (void)g;
    a->x += (int2)rd25c(a, 0);
    wr25c(a, 0, 0);
    knocked_box(a);
    return 0;
}

static int mk_hit(actor_t *a, gds *g) {                 /* m_0fd6c: stomped */
    if (g->unk_496 == 0) {
        if (a->step == 4) a->frame = (a->dir == 1) ? 0x15 : 0x11;
        else {
            a->step = 3; a->unk_02a = 0; a->unk_02c = 0;
            a->frame = (a->dir == 1) ? 0xe : 0xc;
        }
        if ((g->mario->prev_state & 1) == 0) { if (a->x < 0x13e) wr25c(a, 0, 4); }
        else if (a->x > 0x22) wr25c(a, 0, -4);
    } else a->unk_04e = a->cmd;
    return 0;
}

static int mk_destroy(actor_t *a, gds *g) {             /* m_0fe7e */
    if (g->unk_496 == 0) {
        if (a->unk_04c == 0 && g->unk_53a == 0) SCORE(g, 500);
        else a->unk_04c = 0;
        a->state = 4; a->step = 4; a->unk_02a = 0;
    } else a->unk_04e = a->cmd;
    return 0;
}

static int mk_dying(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }   /* m_0ff12 */

static int mk_struck(actor_t *a, gds *g) {              /* m_0ff4c */
    draw_frame(a, g);
    if (++a->unk_02a > 1) { a->step = 5; a->unk_02a = 0; }
    return 0;
}

static int mk_fall(actor_t *a, gds *g) {                /* m_0ff90 */
    a->y += 0x10;
    a->frame = (int2)(a->unk_02a == 1 ? 0x15 : a->unk_02a + 0x10);
    draw_frame(a, g);
    if (++a->unk_02a > 3) a->unk_02a = 0;
    if (a->y > 0xd3) { a->step = 3; a->unk_02a = 2; }
    return 0;
}

static int mk_gone(actor_t *a, gds *g) {                /* m_10022 */
    (void)g;
    if (a->unk_02a-- < 2) { a->state = 0; a->dir = -1; }
    return 0;
}

int m_koopa(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, mk_init);
        H(1, 2, mk_wait);
        H(2, 1, obj_erase);
        H(2, 2, mk_emerge);
        H(3, 1, obj_erase);
        H(3, 2, mk_moving);
        H(3, 3, mk_hit);
        H(3, 4, mk_destroy);
        H(4, 1, obj_erase);
        H(4, 2, mk_dying);
        W(0, mk_walk);
        W(1, mk_turn);
        W(2, mk_pause);
        W(3, mk_knocked);
        W(4, mk_stopped);
        B(3, mk_gone);
        B(4, mk_struck);
        B(5, mk_fall);
    }
    return obj_dispatch(a, g);
}
