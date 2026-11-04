/* obj_b_beetle.c - "b_beetle" (Buzzy Beetle: Larry's hotel), from L3_s01_sub.o
 * (m_0047e .. m_01a20). A low goomba-like walker that also turns to face Mario when he
 * is walking on its floor (not for 40 ticks after the follow rule fired).
 * 150 points. Frames: 1-10 walk, 11-13 turn, 14-19 destroyed, 20-21 hit, 22-23 out.
 */
#include "obj_common.h"

static void at_door(actor_t *a, gds *g) {
    a->y = a->drawn_y0 = a->drawn_y1 = (int2)(g->floor_y[a->floor] - 0x12);
    a->cur_floor = a->floor;
    a->box_y0 = a->y + 2;
    a->box_y1 = a->y + 0x12;
    a->x = g->door_x[a->col];
    a->box_x1 = a->x + 0x1a;
    a->box_x0 = a->x + 2;
}

static int bb_init(actor_t *a, gds *g) {                /* m_0047e */
    a->speed = HOTEL(g) == 8 ? 4 : 2;
    at_door(a, g);
    a->appear_count = 0x5a;
    a->delay = 8;
    if (a->start_mode == 0) a->state = 1;
    else {
        a->unk_02a = 1; a->state = 3; a->step = 0;
        if (a->dir == -1) a->dir = (g->mario->x < a->x) ? 1 : 0;
    }
    floor_limits(a, g, 0x4c, 0x22, 0x118, 0x142);
    a->unk_04a = a->unk_048 = 1;
    return 0;
}

static int bb_dormant(actor_t *a, gds *g) {             /* m_007c6 */
    if (a->timer++ > a->delay) {
        a->timer = 0;
        if (a->appear_count-- > 0) {
            at_door(a, g);
            a->state = 1;
            a->unk_036 = 0;
            a->unk_04e = 0;
        }
    }
    return 0;
}

static int bb_wait(actor_t *a, gds *g) {                /* m_00886 */
    if (g->unk_53a != 0 || (CELL(g, a->floor, a->col) & 0xf000) != 0x2000 || g->mario->floor != a->floor)
        return 0;
    if (door_far_from_mario(a, g, a->col * 0x2c + 0x3c, 0xe) > 0x48) {
        CELL(g, a->floor, a->col) |= 4;
        a->state = 2; a->step = 0;
        a->unk_04a = a->unk_048 = 1;
    }
    return 0;
}

static int bb_emerge(actor_t *a, gds *g) {              /* m_009b2 */
    switch (a->step) {
    case 0: a->frame = 0x16; a->step = 1; break;
    case 1: a->frame = 0x17; a->step = 2; break;
    case 2:
        CELL(g, a->floor, a->col) = (CELL(g, a->floor, a->col) & ~4) | 2;
        a->unk_02a = 1; a->step = 0; a->state = 3;
        a->frame = (a->dir == 1) ? 0xd : 0xb;
        break;
    }
    draw_at(a, g, a->frame);
    return 0;
}

static int bb_moving(actor_t *a, gds *g) {              /* m_00b48 */
    int r = CALL(a->walk_steps[a->step], a, g);
    draw_at(a, g, a->frame);
    return r;
}

static int bb_walk(actor_t *a, gds *g) {                /* m_00c18 */
    mario_t *m = g->mario;
    int same;
    if (--a->unk_02c < 1) { a->unk_02c = 1; walk_cycle(a); }
    if (a->dir == 1) { a->x -= a->speed; a->frame = (int2)(a->unk_02a + 6); }
    else             { a->x += a->speed; a->frame = (int2)(a->unk_02a + 1); }
    if (a->unk_02a == 0) {
        int turn = walker_follow(a, g, a->x, &same);
        if (same) a->unk_036 = 0x28;
        if (turn) { a->step = 1; a->unk_02a = 0; }
    }
    if (a->x > a->x_max) { a->x = (int2)a->x_max; a->step = 1; a->unk_02a = 0; }
    else if (a->x < a->x_min) { a->x = (int2)a->x_min; a->step = 1; a->unk_02a = 0; }
    else if (a->unk_036 == 0) {
        if (m->floor == a->cur_floor && m->state_group == 0 && a->x_min < m->x && m->x < a->x_max &&
            ((a->x < m->x && a->dir == 1) || (m->x < a->x && a->dir == 0))) {
            a->step = 1; a->unk_02a = 0;
        }
    } else a->unk_036--;
    a->box_x0 = a->x + 2;
    a->box_x1 = a->x + 0x1a;
    maybe_open_door(a, g, a->x + 0xe, 0x3f);
    return 0;
}

static int bb_turn(actor_t *a, gds *g) {                /* m_011e6 */
    (void)g;
    a->frame = (int2)((a->dir == 1) ? 0xd - a->unk_02a : a->unk_02a + 0xb);
    if (++a->unk_02a > 2) {
        a->unk_02a = 0; a->step = 0;
        a->dir = (a->dir == 0) ? 1 : 0;
    }
    return 0;
}

static int bb_hit(actor_t *a, gds *g) {                 /* m_0127c */
    if (g->unk_496 == 0) {
        SCORE(g, 150);
        a->state = 4; a->step = 1; a->unk_02a = 0;
        a->unk_04a = a->unk_048 = 0;
    } else a->unk_04e = a->cmd;
    return 0;
}

static int bb_destroy(actor_t *a, gds *g) {             /* m_012ec */
    if (g->unk_496 == 0) {
        if (a->unk_04c == 0 && g->unk_53a == 0) { SCORE(g, 150); a->unk_04a = 0; }
        else a->unk_04c = 0;
        a->state = 4; a->step = 4; a->unk_02a = 0xe; a->unk_02c = 0;
    } else a->unk_04e = a->cmd;
    return 0;
}

static int bb_dying(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }

static int bb_hit_1(actor_t *a, gds *g) {               /* m_01874 */
    draw_at(a, g, 0x14);
    if (++a->unk_02a > 0) a->step = 2;
    return 0;
}
static int bb_hit_2(actor_t *a, gds *g) {               /* m_01928 */
    draw_at(a, g, 0x15);
    if (++a->unk_02a > 3) { a->step = 3; a->unk_02a = 2; }
    return 0;
}
static int bb_gone(actor_t *a, gds *g) {                /* m_019e2 */
    (void)g;
    if (a->unk_02a-- < 2) { a->state = 0; a->cmd = 2; a->timer = 0; a->unk_02c = 0; a->dir = -1; }
    return 0;
}
static int bb_flip(actor_t *a, gds *g) {                /* m_013c4 / m_01476 */
    if (a->step == 4) draw_at(a, g, a->unk_02a); else draw_plain(a, a->unk_02a);
    a->unk_02a++;
    a->step = (int2)(a->step == 4 ? 5 : 6);
    return 0;
}
static int bb_fall(actor_t *a, gds *g) {                /* m_0151c / m_015f2 / m_016c8 / m_0179e */
    a->y += 0x10;
    draw_at_y(a, g, a->unk_02a);
    if (a->step == 9) { a->unk_02a -= 3; a->step = 6; }
    else { a->unk_02a++; a->step++; }
    if (a->y > 0xdf) { a->step = 3; a->unk_02a = 2; }
    return 0;
}

int b_beetle(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, bb_init);
        H(0, 2, bb_dormant);
        H(1, 2, bb_wait);
        H(2, 1, obj_erase);
        H(2, 2, bb_emerge);
        H(3, 1, obj_erase);
        H(3, 2, bb_moving);
        H(3, 3, bb_hit);
        H(3, 4, bb_destroy);
        H(4, 1, obj_erase);
        H(4, 2, bb_dying);
        W(0, bb_walk);
        W(1, bb_turn);
        B(1, bb_hit_1);
        B(2, bb_hit_2);
        B(3, bb_gone);
        B(4, bb_flip);
        B(5, bb_flip);
        B(6, bb_fall);
        B(7, bb_fall);
        B(8, bb_fall);
        B(9, bb_fall);
    }
    return obj_dispatch(a, g);
}

/* "gb_beetle" (hotel 8) runs exactly the same code with its own pictures */
int gb_beetle(actor_t *a, gds *g) { return b_beetle(a, g); }
