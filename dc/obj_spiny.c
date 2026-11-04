/* obj_spiny.c - "spiny" (Larry's hotel), from L3_s06_sub.o (m_08218 .. m_09446).
 * Walks the floor, turning towards Mario every 51 ticks; at either wall it flips over
 * (x_max = 1: upside down, a low box). The right way up it can't be stomped (cmd 3
 * answers 1, so Mario gets hurt); upside down it can (300 points).
 * Frames: 1-10 walk, 11-20 walk flipped, 21-26 turn, 27-29 / 40-42 destroyed,
 * 30-32 falling, 33-34 out of the door, 35-38 / 39 flipping.
 */
#include "obj_common.h"

static void at_door(actor_t *a, gds *g) {
    a->y = a->drawn_y0 = a->drawn_y1 = (int2)(g->floor_y[a->floor] - 0x21);
    a->cur_floor = a->floor;
    a->box_y0 = a->y + 0x17;
    a->box_y1 = a->y + 0x21;
    a->x = (int2)(g->door_x[a->col] - 4);
    a->box_x1 = a->x + 0x1f;
    a->box_x0 = a->x + 9;
}

static int sp_init(actor_t *a, gds *g) {                /* m_08218 */
    a->speed = HOTEL(g) == 8 ? 4 : 2;
    at_door(a, g);
    if (a->start_mode == 0) a->state = 1;
    else {
        a->unk_02a = 1; a->x_min = 0x33; a->state = 3; a->step = 0;
        if (a->dir == -1) a->dir = (g->mario->x < a->x) ? 1 : 0;
    }
    a->x_max = 0;
    a->appear_count = 0x5b;
    a->delay = 0xf;
    a->unk_04a = a->unk_048 = 1;
    return 0;
}

static int sp_dormant(actor_t *a, gds *g) {             /* m_085be */
    if (a->timer++ > a->delay) {
        a->timer = 0;
        if (a->appear_count-- > 0) {
            at_door(a, g);
            a->state = 1;
            a->x_max = 0;
            a->unk_04e = 0;
        }
    }
    return 0;
}

static int sp_wait(actor_t *a, gds *g) {                /* m_08682 */
    int mx, ax, dist;
    if (g->unk_53a != 0 || (CELL(g, a->floor, a->col) & 0xf000) != 0x2000 || g->mario->floor != a->floor)
        return 0;
    mx = g->mario->x + 0xe;
    ax = a->col * 0x2c + 0x3c;
    if (ax < mx) { if (a->dir == -1) a->dir = 0; dist = mx - ax; }
    else         { if (a->dir == -1) a->dir = 1; dist = ax - mx; }
    if (dist > 0x48) {
        CELL(g, a->floor, a->col) |= 4;
        a->state = 2; a->step = 0;
        a->unk_04a = a->unk_048 = 1;
    }
    return 0;
}

static int sp_emerge(actor_t *a, gds *g) {              /* m_087ae */
    switch (a->step) {
    case 0: a->frame = 0x21; a->step = 1; break;
    case 1: a->frame = 0x22; a->step = 2; break;
    case 2:
        CELL(g, a->floor, a->col) = (CELL(g, a->floor, a->col) & ~4) | 2;
        a->frame = (a->dir == 1) ? 0x17 : 0x15;
        a->unk_02a = 1; a->step = 0; a->state = 3; a->x_min = 0x33;
        break;
    }
    draw_at(a, g, a->frame);
    return 0;
}

static int sp_moving(actor_t *a, gds *g) {              /* m_0894c */
    int r = CALL(a->walk_steps[a->step], a, g);
    draw_at(a, g, a->frame);
    return r;
}

static int sp_walk(actor_t *a, gds *g) {                /* m_08a1c */
    mario_t *m = g->mario;
    if (a->unk_02e == 0) {
        if (a->unk_02a == 4) { a->unk_02a--; a->unk_02e = 1; } else a->unk_02a++;
    } else if (a->unk_02a < 1) { a->unk_02a++; a->unk_02e = 0; }
    else a->unk_02a--;
    if (a->dir == 1) { a->x -= a->speed; a->frame = (int2)(a->unk_02a + 6); }
    else             { a->x += a->speed; a->frame = (int2)(a->unk_02a + 1); }
    if (a->x_max != 0) a->frame += 10;
    if (a->x >= 0x137) { a->x = 0x136; a->step = 2; a->unk_02a = 0; }
    else if (a->x < 0x22) { a->x = 0x22; a->step = 2; a->unk_02a = 0; }
    else if (a->x_min-- < 2) {
        a->x_min = 0x33;
        if (m->floor == a->cur_floor && (m->state_group == 0 || m->state_group == 3) &&
            ((a->x < m->x && a->dir == 1) || (m->x < a->x && a->dir == 0))) {
            a->step = 1; a->unk_02a = 0;
        }
    }
    a->box_x0 = a->x + 9;
    a->box_x1 = a->x + 0x1f;
    return 0;
}

static int sp_turn(actor_t *a, gds *g) {                /* m_08c12 */
    (void)g;
    a->frame = (int2)((a->dir == 1) ? 0x17 - a->unk_02a : a->unk_02a + 0x15);
    if (a->x_max != 0) a->frame += 3;
    if (++a->unk_02a > 2) {
        a->unk_02a = 0; a->step = 0;
        a->dir = (a->dir == 0) ? 1 : 0;
    }
    return 0;
}

static int sp_flip(actor_t *a, gds *g) {                /* m_08cbc: at a wall */
    (void)g;
    a->frame = 0x23;
    if (a->x_max != 0) a->frame += 4;
    if (a->unk_02a != 0) a->frame += 2;
    if (a->dir == 1) a->frame += 1;
    if (a->unk_02a++ != 0) {
        a->step = 0; a->unk_02a = 1;
        a->x_max = (a->x_max == 0);
        if (a->x_max == 0) { a->box_y0 = a->y + 0x17; a->box_y1 = a->y + 0x21; }
        else { a->box_y0 = a->y; a->box_y1 = a->y + 10; }
        a->dir = (a->dir == 1) ? 0 : 1;
    }
    return 0;
}

static int sp_hit(actor_t *a, gds *g) {                 /* m_08db4 */
    if (a->x_max == 0) return 1;                        /* spikes up: it hurts Mario */
    if (g->unk_496 == 0) {
        SCORE(g, 300);
        a->state = 4; a->step = 4;
        a->unk_04a = a->unk_048 = 0;
    } else a->unk_04e = a->cmd;
    return 0;
}

static int sp_destroy(actor_t *a, gds *g) {             /* m_08e44 */
    if (g->unk_496 == 0) {
        if (a->unk_04c == 0 && g->unk_53a == 0) { SCORE(g, 300); a->unk_04a = 0; }
        else a->unk_04c = 0;
        a->state = 4; a->step = 4;
    } else a->unk_04e = a->cmd;
    return 0;
}

static int sp_dying(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }

static int sp_gone(actor_t *a, gds *g) {                /* m_0940c */
    (void)g;
    if (a->unk_02a-- < 2) { a->state = 0; a->cmd = 2; a->timer = 0; a->dir = -1; }
    return 0;
}
static int sp_die_1(actor_t *a, gds *g) {               /* m_08f12 */
    a->unk_02a = (int2)(a->x_max == 0 ? 0x1b : 0x28);
    draw_at(a, g, a->unk_02a);
    a->step = 5;
    return 0;
}
static int sp_die_2(actor_t *a, gds *g) {               /* m_08fe6 */
    (void)g;
    a->unk_02a = (int2)(a->x_max == 0 ? 0x1c : 0x2a);
    draw_plain(a, a->unk_02a);
    a->unk_02a = 0x1d;
    a->step = 6;
    return 0;
}
static int sp_fall(actor_t *a, gds *g) {                /* m_090b4 / m_0918a / m_09260 / m_09336 */
    a->y += 0x10;
    draw_at_y(a, g, a->unk_02a);
    if (a->step == 9) { a->unk_02a -= 3; a->step = 6; }
    else { a->unk_02a++; a->step++; }
    if (a->y > 0xd0) { a->step = 3; a->unk_02a = 2; }
    return 0;
}

int spiny(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, sp_init);
        H(0, 2, sp_dormant);
        H(1, 2, sp_wait);
        H(2, 1, obj_erase);
        H(2, 2, sp_emerge);
        H(3, 1, obj_erase);
        H(3, 2, sp_moving);
        H(3, 3, sp_hit);
        H(3, 4, sp_destroy);
        H(4, 1, obj_erase);
        H(4, 2, sp_dying);
        W(0, sp_walk);
        W(1, sp_turn);
        W(2, sp_flip);
        B(3, sp_gone);
        B(4, sp_die_1);
        B(5, sp_die_2);
        B(6, sp_fall);
        B(7, sp_fall);
        B(8, sp_fall);
        B(9, sp_fall);
    }
    return obj_dispatch(a, g);
}
