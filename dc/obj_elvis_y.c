/* obj_elvis_y.c - "elvis_y" (the small Elvis Goomba, hotel 7 stage 7), from L7_s07_sub.o
 * (m_00468 .. m_01278). Comes out of its door 1-3 times (appear_count) and paces its
 * floor: 1 step in 32 it turns round, unless Mario is on its floor within 30 pixels
 * (and not jumping). Stomp or fireball: 400 points, it gets squashed / falls off, and
 * comes back out of its door 100 ticks later until appear_count runs out.
 * x_min / x_max: its walls (0x22 or 0x4c by a lift; 0x146 or 0x11c).
 * Frames: 1-5 walk right, 6-10 walk left, 11-13 turn, 14-15 hit, 16-18 falling,
 * 19-20 squashed, 21-22 out of the door.
 */
#include "obj_common.h"

static void draw_frame(actor_t *a, gds *g) { draw_at_xy(a, g, a->frame); }   /* m_01278 */

static void place(actor_t *a, gds *g) {
    int x = g->door_x[a->col], fy = g->floor_y[a->floor];
    a->x = a->drawn_x0 = a->drawn_x1 = (int2)x;
    a->y = a->drawn_y0 = a->drawn_y1 = (int2)(fy - 0x1e);
    a->box_x0 = x + 9; a->box_x1 = x + 0xf; a->box_y0 = fy - 0x17; a->box_y1 = fy - 1;
    a->hit_x0 = x + 5; a->hit_x1 = x + 0x13; a->hit_y0 = fy - 0x12; a->hit_y1 = fy - 1;
}

static int ey_init(actor_t *a, gds *g) {                /* m_00468 */
    uint16_t l0, l6;
    a->speed = HOTEL(g) == 8 ? 6 : 4;
    a->cur_floor = a->floor;
    place(a, g);
    a->unk_03c = 1;
    if (a->start_mode == 0) a->state = 1;
    else { a->unk_02a = 1; a->state = 3; a->step = 0; }
    if (a->appear_count == 0) a->appear_count = 2; else a->appear_count--;
    a->unk_04a = a->unk_048 = (int2)(a->appear_count + 1);
    a->timer = 0;
    if (a->delay != 0) a->delay = (int2)(6 * (*(int32_t *)(g->stage_info + 8) - a->delay));
    l0 = CELL(g, a->floor, 0) & 0xf000;
    l6 = CELL(g, a->floor, 6) & 0xf000;
    a->x_min = (l0 == 0xf000 || l0 == 0xe000) ? 0x4c : 0x22;
    a->x_max = (l6 == 0xf000 || l6 == 0xe000) ? 0x11c : 0x146;
    return 0;
}

static int ey_again(actor_t *a, gds *g) {               /* m_007da: state 0, cmd 2 */
    if (g->unk_53a == 0 && a->appear_count-- > 0) {
        a->state = 1;
        a->timer = 0; a->delay = 100; a->unk_04e = 0;
        place(a, g);
    }
    return 0;
}

static int ey_wait(actor_t *a, gds *g) {                /* m_008ae */
    uint16_t *c;
    if (g->unk_53a != 0 || a->timer++ <= a->delay) return 0;
    c = &CELL(g, a->floor, a->col);
    if ((*c & 0xf000) == 0) return 0;
    if (door_far_from_mario(a, g, a->x + 0xc, 0xe) > 0x48 || a->cur_floor != g->mario->floor) {
        if ((*c & 0xf000) == 0x1000) { *c = (*c & 0xfff) | 0x2009; g->unk_054 = 2; }
        a->state = 2; a->step = 0;
    }
    return 0;
}

static int ey_emerge(actor_t *a, gds *g) {              /* m_009ee */
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

static int ey_moving(actor_t *a, gds *g) {              /* m_00b28 (x kept only) */
    int r = CALL(a->walk_steps[a->step], a, g);
    draw_at(a, g, a->frame);
    return r;
}

static int ey_walk(actor_t *a, gds *g) {                /* m_00bf8 */
    mario_t *m = g->mario;
    uint16_t *door;
    int x;
    walk_cycle(a);
    if (a->dir == 1) { a->x -= a->speed; a->frame = (int2)(a->unk_02a + 6); }
    else { a->x += a->speed; a->frame = (int2)(a->unk_02a + 1); }
    x = a->x;
    if (x > a->x_max) { a->x = (int2)a->x_max; a->step = 1; a->unk_02a = 0; }
    else if (x < a->x_min) { a->x = (int2)a->x_min; a->step = 1; a->unk_02a = 0; }
    else if ((RND(g) & 0x1f) == 1 &&
             (m->floor != a->cur_floor || x + 0x1e < m->x || m->x < x - 0x1e || m->state_group == 3)) {
        a->step = 1; a->unk_02a = 0;
    }
    x = a->x;
    a->box_x0 = x + 9; a->box_x1 = x + 0xf;
    a->hit_x0 = x + 5; a->hit_x1 = x + 0x13;
    door = DOOR_CELL(a->cur_floor, x + 0xc);
    if (door && (*door & 0xf000) == 0x1000 && (RND(g) & 0x1f) == 1) {
        *door = (*door & 0xfff) | 0x2009;
        g->unk_054 = 2;
    }
    return 0;
}

static int ey_turn(actor_t *a, gds *g) {                /* m_00e66 */
    (void)g;
    a->frame = (int2)((a->dir == 1) ? 0xd - a->unk_02a : a->unk_02a + 0xb);
    if (++a->unk_02a > 2) {
        a->unk_02a = 1; a->step = 0;
        a->dir = (a->dir == 0) ? 1 : 0;
    }
    return 0;
}

static int ey_hit(actor_t *a, gds *g) {                 /* m_00efe: stomped */
    if (g->unk_496 == 0) {
        SCORE(g, 400);
        a->unk_048--; a->unk_04a--;
        a->state = 4; a->step = 1; a->unk_02a = 0;
    } else a->unk_04e = a->cmd;
    return 0;
}

static int ey_destroy(actor_t *a, gds *g) {             /* m_00f6c */
    if (g->unk_496 == 0) {
        if (a->unk_04c == 0 && g->unk_53a == 0) { SCORE(g, 400); a->unk_04a--; }
        else a->unk_04c = 0;
        a->state = 4; a->step = 4; a->unk_02a = 0;
    } else a->unk_04e = a->cmd;
    return 0;
}

static int ey_dying(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }   /* m_01004 */

static int ey_squash1(actor_t *a, gds *g) {             /* m_010f8 */
    a->frame = 0x13; draw_frame(a, g);
    if (++a->unk_02a > 0) a->step = 2;
    return 0;
}
static int ey_squash2(actor_t *a, gds *g) {             /* m_0113e */
    a->frame = 0x14; draw_frame(a, g);
    if (++a->unk_02a > 3) { a->step = 3; a->unk_02a = 2; }
    return 0;
}
static int ey_gone(actor_t *a, gds *g) {                /* m_0118a */
    (void)g;
    if (a->unk_02a-- < 2) { a->state = 0; a->dir = -1; }
    return 0;
}
static int ey_flip(actor_t *a, gds *g) {                /* m_0103e */
    a->frame = (int2)(a->unk_02a + 0xe); draw_frame(a, g);
    if (++a->unk_02a > 1) { a->step = 5; a->unk_02a = 0; }
    return 0;
}
static int ey_fall(actor_t *a, gds *g) {                /* m_0108c */
    a->y += 0x10;
    a->frame = (int2)(a->unk_02a + 0x10); draw_frame(a, g);
    if (++a->unk_02a > 2) a->unk_02a = 0;
    if (a->y > 0xd3) { a->step = 3; a->unk_02a = 2; }
    return 0;
}

int elvis_y(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, ey_init);
        H(0, 2, ey_again);
        H(1, 2, ey_wait);
        H(2, 1, obj_erase);
        H(2, 2, ey_emerge);
        H(3, 1, obj_erase);
        H(3, 2, ey_moving);
        H(3, 3, ey_hit);
        H(3, 4, ey_destroy);
        H(4, 1, obj_erase);
        H(4, 2, ey_dying);
        W(0, ey_walk);
        W(1, ey_turn);
        B(1, ey_squash1);
        B(2, ey_squash2);
        B(3, ey_gone);
        B(4, ey_flip);
        B(5, ey_fall);
    }
    return obj_dispatch(a, g);
}
