/* obj_s_koopa.c - "s_koopa" (the bony koopa, first seen in hotel 5), from L5_s07_sub.o
 * (m_039d0 .. m_04ca2). Walks like the beetle (turning to face a walking Mario, with a
 * 35-tick pause after the follow rule). Stomping it only knocks it to a heap of bones:
 * it lies there for 64 ticks and puts itself back together. Only a fireball or a star
 * (cmd 4, also while it's a heap) finishes it: 500 points, and it falls off the screen.
 * x_max: left limit; unk_25c: int32 right limit.
 * Frames: 1-10 walk, 11-13 turn, 14-18 falling, 19-24 getting up, 19-23 collapsing
 * (0x17 down), 24-25 out of the door, 26-35 the heap.
 */
#include "obj_common.h"

static void draw_frame(actor_t *a, gds *g) { draw_at_xy(a, g, a->frame); }   /* m_04ca2 */

static int sk_init(actor_t *a, gds *g) {                /* m_039d0 */
    uint16_t l0, l6;
    a->speed = HOTEL(g) == 8 ? 4 : 2;
    a->cur_floor = a->floor;
    a->x = a->drawn_x0 = a->drawn_x1 = (int2)(g->door_x[a->col] - 0xc);
    a->y = a->drawn_y0 = a->drawn_y1 = (int2)(g->floor_y[a->floor] - 0x3a);
    if (a->start_mode == 0) a->state = 1;
    else {
        a->unk_02a = 1; a->state = 3; a->step = 0;
        if (a->dir == -1) a->dir = (g->mario->x < a->x) ? 1 : 0;
    }
    a->timer = 0;
    if (a->delay != 0) a->delay = (int2)(6 * (*(int32_t *)(g->stage_info + 8) - a->delay));
    l0 = CELL(g, a->floor, 0) & 0xf000;
    l6 = CELL(g, a->floor, 6) & 0xf000;
    a->x_max = (l0 == 0xf000 || l0 == 0xe000) ? 0x40 : 0x16;
    wr25c(a, 0, (l6 == 0xf000 || l6 == 0xe000) ? 0x108 : 0x132);
    return 0;
}

static int sk_wait(actor_t *a, gds *g) {                /* m_03de2 */
    uint16_t *c, v;
    if (g->unk_53a != 0 || ++a->timer <= a->delay) return 0;
    c = &CELL(g, a->floor, a->col);
    v = *c & 0xf000;
    if (v != 0x2000 && v != 0x1000) return 0;
    if (door_far_from_mario(a, g, a->x + 0x1c, 0xe) > 0x48 || g->mario->floor != a->cur_floor) {
        a->state = 2; a->step = 0;
        if (v == 0x1000) { *c = (*c & 0xfff) | 0x2009; g->unk_054 = 2; }
    }
    return 0;
}

static int sk_emerge(actor_t *a, gds *g) {              /* m_03f38 */
    switch (a->step) {
    case 0: CELL(g, a->floor, a->col) |= 4; a->frame = 0x18; a->step = 1; break;
    case 1: a->frame = 0x19; a->step = 2; break;
    case 2:
        CELL(g, a->floor, a->col) = (CELL(g, a->floor, a->col) & ~4) | 2;
        a->unk_02a = 1; a->step = 0; a->state = 3;
        a->frame = (a->dir == 1) ? 0xd : 0xb;
        break;
    }
    draw_frame(a, g);
    return 0;
}

static int sk_moving(actor_t *a, gds *g) {              /* m_04064 */
    int r = CALL(a->walk_steps[a->step], a, g);
    draw_frame(a, g);
    return r;
}

static int sk_walk(actor_t *a, gds *g) {                /* m_040b0 */
    mario_t *m = g->mario;
    int2 x;
    int same;
    if (--a->unk_02c < 1) { a->unk_02c = 1; walk_cycle(a); }
    if (a->dir == 1) { x = (int2)(a->x - a->speed); a->frame = (int2)(a->unk_02a + 6); }
    else             { x = (int2)(a->x + a->speed); a->frame = (int2)(a->unk_02a + 1); }
    a->x = x;
    if (a->unk_02a == 0) {
        int turn = walker_follow(a, g, x, &same);
        if (same) a->unk_036 = 0x23;
        if (turn) { a->step = 1; a->unk_02a = 0; }
    }
    if (x > rd25c(a, 0)) { x = a->x = (int2)rd25c(a, 0); a->step = 1; a->unk_02a = 0; }
    else if (x < a->x_max) { x = a->x = (int2)a->x_max; a->step = 1; a->unk_02a = 0; }
    else if (a->unk_036 == 0) {
        if (m->floor == a->cur_floor && m->state_group == 0 && a->x_max < m->x && m->x < rd25c(a, 0) &&
            ((x < m->x && a->dir == 1) || (m->x < x && a->dir == 0))) {
            a->step = 1; a->unk_02a = 0;
        }
    } else a->unk_036--;
    a->box_y0 = a->y + 0x22; a->box_y1 = a->y + 0x30;
    a->hit_y0 = a->y + 0x2a; a->hit_y1 = a->y + 0x39;
    if (a->dir == 1) { a->box_x0 = x + 0x12; a->box_x1 = x + 0x1e; a->hit_x0 = x + 0x18; a->hit_x1 = x + 0x25; }
    else { a->box_x0 = x + 0x1a; a->box_x1 = x + 0x26; a->hit_x0 = x + 0x13; a->hit_x1 = x + 0x20; }
    a->unk_03c = 1;
    maybe_open_door(a, g, x + 0x1c, 0x3f);
    return 0;
}

static int sk_turn(actor_t *a, gds *g) {                /* m_046ce */
    (void)g;
    a->unk_03c = 0;
    a->box_x0 = a->x + 0x16; a->box_x1 = a->x + 0x22;
    a->box_y0 = a->y + 0x20; a->box_y1 = a->y + 0x39;
    a->frame = (int2)((a->dir == 1) ? 0xd - a->unk_02a : a->unk_02a + 0xb);
    if (++a->unk_02a > 2) {
        a->unk_02a = 0; a->step = 0;
        a->dir = (a->dir == 0) ? 1 : 0;
    }
    return 0;
}

static int sk_hit(actor_t *a, gds *g) {                 /* m_04790: stomped - collapses */
    if (g->unk_496 == 0) {
        a->state = 4; a->step = 1; a->unk_02a = 0; a->unk_02c = 0;
        a->y += 4;
    } else a->unk_04e = a->cmd;
    return 0;
}

static int sk_destroy(actor_t *a, gds *g) {             /* m_047ec */
    if (g->unk_496 == 0) {
        if (a->unk_04c == 0 && g->unk_53a == 0) SCORE(g, 500);
        else a->unk_04c = 0;
        a->state = 4; a->step = 4; a->unk_02a = 0; a->unk_02c = 0;
        a->y += 0xf;
        a->x_min = 0;
    } else a->unk_04e = a->cmd;
    return 0;
}

static int sk_dying(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }

static int sk_collapse(actor_t *a, gds *g) {            /* m_0493c */
    a->frame = (int2)(0x17 - a->unk_02a);
    draw_frame(a, g);
    if (g->unk_53a == 0 && ++a->unk_02c > 1) {
        if (++a->unk_02a < 5) {
            if (a->frame == 0x17) g->unk_054 = 0xc;
        } else { a->step = 6; a->unk_02a = 0; a->unk_02c = 0; }
    }
    return 0;
}

static int sk_get_up(actor_t *a, gds *g) {              /* m_04a66 */
    a->frame = (int2)(a->unk_02a + 0x13);
    if (++a->unk_02c > 4 && ++a->unk_02a > 5) {
        a->y -= 4;
        if (g->unk_53a == 0) {
            a->state = 3; a->step = 0; a->unk_02c = 0; a->unk_02a = 1; a->unk_036 = 0;
            a->frame = (a->dir == 1) ? 0xd : 0xb;
        } else {
            a->state = 4; a->step = 4; a->unk_02a = 0; a->unk_02c = 0;
            a->y += 0xf; a->x_min = 0; a->frame = 0xe;
        }
    }
    draw_frame(a, g);
    return 0;
}

static int sk_gone(actor_t *a, gds *g) {                /* m_04b42 */
    (void)g;
    if (a->unk_02a-- < 2) { a->state = 0; a->cmd = 2; a->timer = 0; a->unk_02c = 0; a->dir = -1; }
    return 0;
}

static int sk_fall(actor_t *a, gds *g) {                /* m_048c8 */
    a->y += (int2)a->x_min;
    a->x_min += 2;
    if (a->y > 200) a->y = 200;
    a->frame = (int2)(a->unk_02a + 0xe);
    draw_frame(a, g);
    if (++a->unk_02a > 4) { a->step = 3; a->unk_02a = 2; }
    return 0;
}

static int sk_heap(actor_t *a, gds *g) {                /* m_049e2 */
    a->frame = (int2)(a->unk_02a + 0x1a);
    draw_frame(a, g);
    if (g->unk_53a == 0 && ++a->unk_02c > 0x40 && ++a->unk_02a > 9) {
        a->step = 2; a->unk_02a = 0; a->unk_02c = 0;
    }
    return 0;
}

int s_koopa(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, sk_init);
        H(1, 2, sk_wait);
        H(2, 1, obj_erase);
        H(2, 2, sk_emerge);
        H(3, 1, obj_erase);
        H(3, 2, sk_moving);
        H(3, 3, sk_hit);
        H(3, 4, sk_destroy);
        H(4, 1, obj_erase);
        H(4, 2, sk_dying);
        H(4, 4, sk_destroy);
        W(0, sk_walk);
        W(1, sk_turn);
        B(1, sk_collapse);
        B(2, sk_get_up);
        B(3, sk_gone);
        B(4, sk_fall);
        B(6, sk_heap);
    }
    return obj_dispatch(a, g);
}
