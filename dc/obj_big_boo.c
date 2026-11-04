/* obj_big_boo.c - "big_boo", from L5_s09_sub.o (m_0fe08 .. m_10996).
 * A big Boo flying freely (x 4 .. 310, y 10 .. 175), counting as on Mario's floor.
 * While Mario walks or jumps it chases him (2 px a tick, re-aiming up/down every 10
 * ticks) unless he faces it - then it covers its face (2 frames, 2 ticks each) until he
 * turns away or stops. Otherwise it drifts at random, changing course every 10 ticks
 * while it is more than 120 px from Mario. Touching it hurts (cmd 3 returns 1); only a
 * star (item_hit > 9) or the stage's end drops it off the screen (cmd 4 returns 2).
 * NB: it keeps its x / y speed in floor / col.  x_min: 1 chasing, 0 wandering.
 * Frames: 1 / 7 chase, 6 / 12 wander, 2-4 / 8-10 covering, 5 / 11 uncovering,
 * 13 / 14 turning, 14 falling.
 */
#include "obj_common.h"

static void draw_frame(actor_t *a, gds *g) { draw_at_xy(a, g, a->frame); }

static int bg_init(actor_t *a, gds *g) {                /* m_0fe08 */
    a->unk_02a = 0; a->unk_02c = 2; a->step = 0;
    a->y = a->drawn_y0 = a->drawn_y1 = (int2)(g->floor_y[a->floor] - 0x37);
    a->x = a->drawn_x0 = a->drawn_x1 = (int2)(g->door_x[a->col] + 8);
    a->unk_03c = 1;
    if (a->dir == -1) a->dir = 0;
    a->state = 3; a->step = 0;
    return 0;
}

static int bg_fly(actor_t *a, gds *g) {                 /* m_1004a */
    mario_t *m = g->mario;
    int aim;
    if (m->state_group == 0 || m->state_group == 3) {    /* chase */
        aim = (a->unk_02e <= ++a->unk_036);
        if (aim) { a->unk_036 = 0; a->unk_02e = 10; }
        if (a->x + 0x1e < m->x) {
            a->floor = 2;
            if (m->prev_state & 1) { a->step = 2; a->unk_02a = 0; a->unk_02c = 2; }
            if (a->dir == 1) a->step = 1;
        } else if (m->x < a->x + 0x1e) {
            a->floor = -2;
            if ((m->prev_state & 1) == 0) { a->step = 2; a->unk_02a = 0; a->unk_02c = 2; }
            if (a->dir == 0) a->step = 1;
        } else a->floor = 0;
        if (aim) {
            if ((m->state_group == 3 && m->y < a->y + 0x28) || m->y - 0x14 < a->y) a->col = -2;
            else if (a->y + 0x37 < m->y + 0x1e) a->col = 2;
            else a->col = 0;
        }
        a->x_min = 1;
        a->frame = (a->dir == 1) ? 7 : 1;
    } else {                                             /* wander */
        if (a->unk_036 < a->unk_02e) {
            if (abs(a->x + 0x10 - m->x) > 0x78 || abs(a->y + 0xc - m->y) > 0x78) a->unk_036++;
            aim = 0;
        } else { a->unk_036 = 0; a->unk_02e = 10; aim = 1; }
        if (aim) {
            a->floor = (int2)((RND(g) & 1) == 0 ? -2 : 2);
            if ((a->floor > 0 && a->dir == 1) || (a->floor < 0 && a->dir == 0)) a->step = 1;
            a->col = (int2)((RND(g) & 1) == 0 ? -2 : 2);
        }
        a->x_min = 0;
        a->frame = (a->dir == 1) ? 0xc : 6;
    }
    a->x = (int2)(a->x + a->floor);
    a->y = (int2)(a->y + a->col);
    if (a->x > 0x136) { a->x = 0x136; a->unk_036 = a->unk_02e; }
    else if (a->x < 4) { a->x = 4; a->unk_036 = a->unk_02e; }
    if (a->y < 10) { a->y = 10; a->col = 2; }
    else if (a->y > 0xaf) { a->y = 0xaf; a->col = -2; }
    a->cur_floor = m->floor;
    a->box_y0 = a->y + 6; a->box_y1 = a->y + 0x2c;
    a->hit_y0 = a->y + 0x21; a->hit_y1 = a->y + 0x35;
    if (a->dir == 1) { a->box_x0 = a->x + 0xc; a->box_x1 = a->x + 0x35; a->hit_x0 = a->x + 0x16; a->hit_x1 = a->x + 0x41; }
    else { a->box_x0 = a->x + 0x11; a->box_x1 = a->x + 0x3a; a->hit_x0 = a->x + 5; a->hit_x1 = a->x + 0x30; }
    a->unk_03c = 1;
    return 0;
}

static int bg_turn(actor_t *a, gds *g) {                /* m_10754 */
    (void)g;
    a->unk_03c = 0;
    a->box_x0 = a->x + 0x11; a->box_x1 = a->x + 0x35;
    a->box_y0 = a->y + 6; a->box_y1 = a->y + 0x30;
    a->frame = (a->x_min == 0) ? 0xe : 0xd;
    a->dir = (a->dir == 1) ? 0 : 1;
    a->step = 0;
    return 0;
}

static int bg_cover(actor_t *a, gds *g) {               /* m_10560 */
    (void)g;
    a->frame = (int2)((a->dir == 1) ? a->unk_02a + 8 : a->unk_02a + 2);
    if (a->unk_02c-- < 2) { a->unk_02c = 2; if (++a->unk_02a > 1) a->step = 3; }
    return 0;
}

static int bg_shy(actor_t *a, gds *g) {                 /* m_105d8 */
    mario_t *m = g->mario;
    int moving = (m->state_group == 0 || m->state_group == 3);
    if (a->dir == 1) {
        a->frame = 10;
        if (a->x + 0x1e < m->x || (m->prev_state & 1) != 0 || !moving) { a->unk_02a = 0; a->unk_02c = 2; a->step = 4; }
    } else {
        a->frame = 4;
        if (m->x < a->x + 0x1e || (m->prev_state & 1) == 0 || !moving) { a->unk_02a = 0; a->unk_02c = 2; a->step = 4; }
    }
    return 0;
}

static int bg_uncover(actor_t *a, gds *g) {             /* m_106de */
    (void)g;
    a->frame = (int2)((a->dir == 1) ? a->unk_02a + 0xb : a->unk_02a + 5);
    if (a->unk_02c-- < 2) { a->unk_02c = 2; if (++a->unk_02a > 1) a->step = 0; }
    return 0;
}

static int bg_moving(actor_t *a, gds *g) {              /* m_0ff6e */
    int r = CALL(a->walk_steps[a->step], a, g);
    draw_frame(a, g);
    return r;
}

static int bg_hit(actor_t *a, gds *g) { (void)a; (void)g; return 1; }   /* m_107f4 */

static int bg_destroy(actor_t *a, gds *g) {             /* m_10818 */
    if (g->unk_53a != 0 || g->item_hit > 9) a->state = 4;
    return 2;
}

static int bg_fall(actor_t *a, gds *g) {                /* m_10864 */
    a->frame = 0xe;
    a->y += 0x14;
    if (a->y > 0xb6) a->state = 0;
    if (a->y > 0xcb) a->y = 0xcb;
    if (a->state != 0) draw_frame(a, g);                 /* (state 0: erased, gone) */
    return 0;
}

int big_boo(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, bg_init);
        H(3, 1, obj_erase);
        H(3, 2, bg_moving);
        H(3, 3, bg_hit);
        H(3, 4, bg_destroy);
        H(4, 1, obj_erase);
        H(4, 2, bg_fall);
        W(0, bg_fly);
        W(1, bg_turn);
        W(2, bg_cover);
        W(3, bg_shy);
        W(4, bg_uncover);
    }
    return obj_dispatch(a, g);
}
