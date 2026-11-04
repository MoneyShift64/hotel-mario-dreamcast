/* obj_elvis_o.c - "elvis_o" (the hopping Elvis Goomba, hotel 7 stage 4), from L7_s04_sub.o
 * (m_00476 .. m_0175a). Comes out of its door 1-3 times (appear_count), walks along
 * its floor (turning to follow other walkers like the goomba), and now and then (1 in 32
 * steps) does a little hop: walk_steps[2] reads a 2-bit-per-step height pattern from
 * x_min (0x3a516b). Stomp or fireball: 350 points, it gets squashed / falls off and
 * comes back out of its door 100 ticks later until appear_count runs out.
 * unk_25c: int32 left limit (0x22, 0x4c by a lift), int32 right limit (0x138 / 0x10e).
 * x_max: the walk step to return to after a turn (0 walk, 2 hop).
 * Frames: 1-5 walk right, 6-10 walk left, 11-13 turn, 14-15 hit, 16-18 falling,
 * 19-20 squashed, 21-22 out of the door, 23-25 hop right, 26-28 hop left.
 */
#include "obj_common.h"

static void draw_frame(actor_t *a, gds *g) { draw_at_xy(a, g, a->frame); }   /* m_0175a */

static void place(actor_t *a, gds *g) {
    a->y = a->drawn_y0 = a->drawn_y1 = (int2)(g->floor_y[a->floor] - 0x27);
    a->cur_floor = a->floor;
    a->box_y0 = a->y + 0x10; a->box_y1 = a->y + 0x27;
    a->x = g->door_x[a->col];
    a->box_x1 = a->x + 0x22; a->box_x0 = a->x + 0x12;
}

static int eo_init(actor_t *a, gds *g) {                /* m_00476 */
    uint16_t l0, l6;
    a->speed = HOTEL(g) == 8 ? 6 : 4;
    a->y = a->drawn_y0 = a->drawn_y1 = (int2)(g->floor_y[a->floor] - 0x27);
    a->cur_floor = a->floor;
    if (a->start_mode == 0) a->state = 1;
    else { a->unk_02a = 1; a->state = 3; a->step = 0; }
    a->box_y0 = a->y + 0x10; a->box_y1 = a->y + 0x27;
    a->x = g->door_x[a->col];
    a->box_x1 = a->x + 0x22; a->box_x0 = a->x + 0x12;
    if (a->appear_count == 0) a->appear_count = 2; else a->appear_count--;
    a->unk_04a = a->unk_048 = (int2)(a->appear_count + 1);
    a->timer = 0;
    if (a->delay != 0) a->delay = (int2)(6 * (*(int32_t *)(g->stage_info + 8) - a->delay));
    l0 = CELL(g, a->floor, 0) & 0xf000;
    l6 = CELL(g, a->floor, 6) & 0xf000;
    wr25c(a, 0, (l0 == 0xf000 || l0 == 0xe000) ? 0x4c : 0x22);
    wr25c(a, 4, (l6 == 0xf000 || l6 == 0xe000) ? 0x10e : 0x138);
    return 0;
}

static int eo_again(actor_t *a, gds *g) {               /* m_00816: state 0, cmd 2 */
    if (g->unk_53a == 0 && a->appear_count-- > 0) {
        place(a, g);
        a->state = 1;
        a->timer = 0; a->delay = 100;
        a->x_min = 0; a->unk_04e = 0;
    }
    return 0;
}

static int eo_wait(actor_t *a, gds *g) {                /* m_008d4 */
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

static int eo_erase(actor_t *a, gds *g) {               /* m_01638: (the eraser near the HUD) */
    int y = g->draw_field == 1 ? a->drawn_y0 : a->drawn_y1;
    if (y < 0x24 && 0x13 < y + 0x27) g->unk_532 = 1;
    return 0;
}

static int eo_emerge(actor_t *a, gds *g) {              /* m_00a14 */
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

static int eo_moving(actor_t *a, gds *g) {              /* m_00b4e */
    int r = CALL(a->walk_steps[a->step], a, g);
    draw_frame(a, g);
    return r;
}

static void clamp(actor_t *a, int next_step) {
    if (a->x > rd25c(a, 4)) { a->x = (int2)rd25c(a, 4); a->step = 1; a->x_max = next_step; a->unk_02a = 0; }
    else if (a->x < rd25c(a, 0)) { a->x = (int2)rd25c(a, 0); a->step = 1; a->x_max = next_step; a->unk_02a = 0; }
}

static int eo_walk(actor_t *a, gds *g) {                /* m_00b9a */
    uint16_t *door;
    walk_cycle(a);
    if (a->dir == 1) { a->x -= a->speed; a->frame = (int2)(a->unk_02a + 6); a->box_x0 = a->x + 4; a->box_x1 = a->x + 0x14; }
    else { a->x += a->speed; a->frame = (int2)(a->unk_02a + 1); a->box_x0 = a->x + 0x12; a->box_x1 = a->x + 0x22; }
    if (a->unk_02a == 0 && walker_follow(a, g, a->x, NULL)) { a->step = 1; a->unk_02a = 0; }
    if ((RND(g) & 0x1f) == 1) { a->x_min = 0x3a516b; a->unk_02a = 0; a->step = 2; }
    clamp(a, 0);
    a->box_y0 = a->y + 0x10; a->box_y1 = a->y + 0x27;
    door = DOOR_CELL(a->cur_floor, a->x + 0x12);
    if (door && (*door & 0xf000) == 0x1000 && (RND(g) & 0x1f) == 1) {
        *door = (*door & 0xfff) | 0x2009;
        g->unk_054 = 2;
    }
    return 0;
}

static int eo_turn(actor_t *a, gds *g) {                /* m_012ac */
    (void)g;
    a->frame = (int2)((a->dir == 1) ? 0xd - a->unk_02a : a->unk_02a + 0xb);
    if (++a->unk_02a > 2) {
        a->unk_02a = 1;
        a->step = (int2)a->x_max;
        a->dir = (a->dir == 0) ? 1 : 0;
    }
    a->box_x0 = a->x + 10; a->box_x1 = a->x + 0x1c;
    return 0;
}

static int eo_hop(actor_t *a, gds *g) {                 /* m_010e6 */
    (void)g;
    if (a->unk_02e == 0) {
        if (a->unk_02a == 2) { a->unk_02a--; a->unk_02e = 1; } else a->unk_02a++;
    } else if (a->unk_02a == 0) { a->unk_02a++; a->unk_02e = 0; }
    else a->unk_02a--;
    if (a->dir == 1) { a->x -= a->speed; a->frame = (int2)(a->unk_02a + 0x1a); a->box_x0 = a->x + 4; a->box_x1 = a->x + 0x14; }
    else { a->x += a->speed; a->frame = (int2)(a->unk_02a + 0x17); a->box_x0 = a->x + 0x12; a->box_x1 = a->x + 0x22; }
    if (a->x_min == 0) a->step = 0;
    else {
        int d = a->x_min & 3;
        a->x_min >>= 2;
        if (a->x_min < 0x3a5) a->y += d; else a->y -= d;
    }
    clamp(a, 2);
    a->box_y0 = a->y + 0x10; a->box_y1 = a->y + 0x27;
    return 0;
}

static int eo_hit(actor_t *a, gds *g) {                 /* m_0135a: stomped */
    if (g->unk_496 == 0) {
        SCORE(g, 350);
        a->unk_048--; a->unk_04a--;
        a->state = 4; a->unk_02a = 0;
        a->step = (a->step == 2) ? 4 : 1;      /* hit in mid-hop: falls */
    } else a->unk_04e = a->cmd;
    return 0;
}

static int eo_destroy(actor_t *a, gds *g) {             /* m_013ea */
    if (g->unk_496 == 0) {
        if (a->unk_04c == 0 && g->unk_53a == 0) { SCORE(g, 350); a->unk_04a--; }
        else a->unk_04c = 0;
        a->state = 4; a->step = 4; a->unk_02a = 0;
    } else a->unk_04e = a->cmd;
    return 0;
}

static int eo_dying(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }   /* m_01482 */

static int eo_squash1(actor_t *a, gds *g) {             /* m_01576 */
    a->frame = 0x13; draw_frame(a, g);
    if (++a->unk_02a > 0) a->step = 2;
    return 0;
}
static int eo_squash2(actor_t *a, gds *g) {             /* m_015bc */
    a->frame = 0x14; draw_frame(a, g);
    if (++a->unk_02a > 3) { a->step = 3; a->unk_02a = 2; }
    return 0;
}
static int eo_gone(actor_t *a, gds *g) {                /* m_01608 */
    (void)g;
    if (a->unk_02a-- < 2) { a->state = 0; a->dir = -1; }
    return 0;
}
static int eo_flip(actor_t *a, gds *g) {                /* m_014bc */
    a->frame = (int2)(a->unk_02a + 0xe); draw_frame(a, g);
    if (++a->unk_02a > 1) { a->step = 5; a->unk_02a = 0; }
    return 0;
}
static int eo_fall(actor_t *a, gds *g) {                /* m_0150a */
    a->y += 0x10;
    a->frame = (int2)(a->unk_02a + 0x10); draw_frame(a, g);
    if (++a->unk_02a > 2) a->unk_02a = 0;
    if (a->y > 0xca) { a->step = 3; a->unk_02a = 2; }
    return 0;
}

int elvis_o(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, eo_init);
        H(0, 2, eo_again);
        H(1, 2, eo_wait);
        H(2, 1, eo_erase);
        H(2, 2, eo_emerge);
        H(3, 1, eo_erase);
        H(3, 2, eo_moving);
        H(3, 3, eo_hit);
        H(3, 4, eo_destroy);
        H(4, 1, eo_erase);
        H(4, 2, eo_dying);
        W(0, eo_walk);
        W(1, eo_turn);
        W(2, eo_hop);
        B(1, eo_squash1);
        B(2, eo_squash2);
        B(3, eo_gone);
        B(4, eo_flip);
        B(5, eo_fall);
    }
    return obj_dispatch(a, g);
}
