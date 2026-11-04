/* obj_koopa.c - the koopa (green koopa troopa), ported from the stage modules.
 *
 * Source: L1_s06_sub.o, entry "koopa" (functions m_0049a .. m_01908). It is the goomba's
 * design with its own numbers: faster (4, 6 in hotel 8), taller (y = floor_y - 0x1c),
 * a 30-tick wait, separate head and shell boxes, 1-in-32 door opening, 100 points, and
 * a two-picture shell flip before it falls off the screen when destroyed.
 * Frames: 1-5 walk right, 6-10 left, 11-13 turn, 14-18 destroyed (shell), 19-20 hit,
 * 21-22 stepping out of its door.
 */
#include "world.h"

#define CELL(g, f, c) (((uint16_t *)((g)->stage_info + 0x12))[(f) * COLS + (c)])
#define RND(g) ((g)->unk_826 = (int4)hm_rnd((uint32_t *)&(g)->unk_826))
#define CALL(fn, a, g) ((int (*)(actor_t *, gds *))(fn))((a), (g))

static int nop(actor_t *a, gds *g) { (void)a; (void)g; return 0; }
static int erase(actor_t *a, gds *g) { (void)a; (void)g; return 0; }   /* m_01908 */

static void draw_at(actor_t *a, gds *g, int frame) {
    obj_draw(a, frame, a->x, a->y);
    if (g->draw_field == 1) a->drawn_x0 = a->x; else a->drawn_x1 = a->x;
}

static void place_at_door(actor_t *a, gds *g) {
    int2 v = g->door_x[a->col];
    a->x = a->drawn_x0 = a->drawn_x1 = v;
    v = (int2)(g->floor_y[a->floor] - 0x1c);
    a->y = a->drawn_y0 = a->drawn_y1 = v;
    a->cur_floor = a->floor;
}

/* m_0049a: state 0, cmd 0 - set up */
static int koopa_init(actor_t *a, gds *g) {
    uint16_t lift0, lift6;
    a->speed = (g->player[g->cur_player].hotel == 8) ? 6 : 4;
    place_at_door(a, g);
    if (a->start_mode == 1) {
        a->unk_02a = 1; a->state = 3; a->step = 0;
    } else if (a->start_mode == 2) {
        CELL(g, a->floor, a->col) |= 4;
        a->unk_02a = 1; a->state = 2; a->step = 0;
    } else {
        a->state = 1;
    }
    if (a->dir == -1) a->dir = (g->mario->x < a->x) ? 1 : 0;
    if (a->delay == 0) a->delay = 0x1e;
    if (a->appear_count == 0) a->appear_count = 1;
    else a->appear_count--;
    a->unk_048 = a->unk_04a = a->appear_count + 1;
    lift0 = CELL(g, a->floor, 0) & 0xf000;
    lift6 = CELL(g, a->floor, 6) & 0xf000;
    a->x_min = (lift0 == 0xf000 || lift0 == 0xe000) ? 0x4c : 0x22;
    a->x_max = (lift6 == 0xf000 || lift6 == 0xe000) ? 0x118 : 0x142;
    return 0;
}

/* m_0082c: state 0, cmd 2 - gone: wait, then come back to its door */
static int koopa_dormant(actor_t *a, gds *g) {
    if (a->timer++ > a->delay) {
        a->timer = 0;
        if (a->appear_count-- > 0) {
            place_at_door(a, g);
            a->state = 1;
            a->hit_y1 = a->hit_y0 = a->hit_x1 = a->hit_x0 = 0;
            a->box_y1 = a->box_y0 = a->box_x1 = a->box_x0 = 0;
            a->unk_04e = 0;
        }
    }
    return 0;
}

/* m_008e8: state 1, cmd 2 - behind its door: out when it's open and Mario is on this
 * floor more than 72 pixels away */
static int koopa_wait_door(actor_t *a, gds *g) {
    if ((CELL(g, a->floor, a->col) & 0xf000) == 0x2000 && g->mario->floor == a->floor) {
        int mx = g->mario->x + 0xe, ax = a->x + 0xe, dist;
        if (ax < mx) { if (a->dir == -1) a->dir = 0; dist = mx - ax; }
        else         { if (a->dir == -1) a->dir = 1; dist = ax - mx; }
        if (dist > 0x48) {
            CELL(g, a->floor, a->col) |= 4;
            a->state = 2;
            a->step = 0;
        }
    }
    return 0;
}

/* m_009ea: state 2, cmd 2 - stepping out of the doorway */
static int koopa_emerge(actor_t *a, gds *g) {
    switch (a->step) {
    case 0: a->frame = 0x15; a->step = 1; break;
    case 1: a->frame = 0x16; a->step = 2; break;
    case 2:
        CELL(g, a->floor, a->col) = (CELL(g, a->floor, a->col) & ~4) | 2;
        a->unk_02a = 1;
        a->unk_02e = 0;
        a->step = 0;
        a->state = 3;
        a->frame = (a->dir == 1) ? 0xd : 0xb;
        break;
    }
    draw_at(a, g, a->frame);
    return 0;
}

/* m_00c44: walk step */
static int koopa_walk(actor_t *a, gds *g) {
    int2 x;
    uint16_t *door;
    if (a->unk_02e == 0) {
        if (a->unk_02a == 4) { a->unk_02a--; a->unk_02e = 1; } else a->unk_02a++;
    } else if (a->unk_02a == 0) { a->unk_02a++; a->unk_02e = 0; }
    else a->unk_02a--;
    if (a->dir == 1) { x = (int2)(a->x - a->speed); a->frame = (int2)(a->unk_02a + 6); }
    else             { x = (int2)(a->x + a->speed); a->frame = (int2)(a->unk_02a + 1); }
    a->x = x;

    /* at the start of each cycle on Mario's floor (same rule as the goomba) */
    if (a->unk_02a == 0 && a->cur_floor >= 1 && g->mario->floor == a->cur_floor) {
        mario_t *m = g->mario;
        int above = a->cur_floor - 1, i, same_dir = 0, other_dir = 0;
        actor_t *p;
        for (i = 0, p = g->room->actors; i < 24 && p->type != 0; i++, p++) {
            int near;
            if (p->state != 3 || p->cur_floor != above || p->speed != a->speed) continue;
            near = (m->x < x) ? (x - 0x3c < p->x && p->x < x + 8) : (p->x < x + 0x3c && x - 8 < p->x);
            if (!near) continue;
            if (p->dir == a->dir) same_dir = 1; else other_dir = 1;
        }
        if (same_dir) {
            if (!other_dir) { a->step = 1; a->unk_02a = 0; }
            else if ((a->dir == 1 && m->x < x) || (a->dir == 0 && x < m->x)) { a->step = 1; a->unk_02a = 0; }
        }
    }
    if (x > a->x_max)      { x = a->x = (int2)a->x_max; a->step = 1; a->unk_02a = 0; }
    else if (x < a->x_min) { x = a->x = (int2)a->x_min; a->step = 1; a->unk_02a = 0; }
    a->box_y0 = a->y + 4;   a->box_y1 = a->y + 0xd;      /* head */
    a->hit_y0 = a->y + 0xb; a->hit_y1 = a->y + 0x1b;     /* shell */
    if (a->dir == 1) {
        a->box_x0 = x + 6;   a->box_x1 = x + 0x10;
        a->hit_x0 = x + 0xb; a->hit_x1 = x + 0x17;
    } else {
        a->box_x0 = x + 0xc; a->box_x1 = x + 0x16;
        a->hit_x0 = x + 5;   a->hit_x1 = x + 0x11;
    }
    a->unk_03c = 1;

    /* passing a closed door: 1 in 32 chance of opening it */
    door = hm_door_cell_at(&hm_world->doors, a->cur_floor, a->x + 0xe);
    if (door && (*door & 0xf000) == 0x1000) {
        if ((RND(g) & 0x1f) == 1) {
            *door = (*door & 0xfff) | 0x2009;
            g->unk_054 = 2;
        }
    }
    return 0;
}

/* m_0117e: turning round */
static int koopa_turn(actor_t *a, gds *g) {
    (void)g;
    a->unk_03c = 0;
    a->box_x0 = a->x + 8;  a->box_x1 = a->x + 0x14;
    a->box_y0 = a->y + 3;  a->box_y1 = a->y + 0x1b;
    a->frame = (int2)((a->dir == 1) ? 0xd - a->unk_02a : a->unk_02a + 0xb);
    if (++a->unk_02a > 2) {
        a->unk_02a = 1;
        a->unk_02e = 0;
        a->step = 0;
        a->dir = (a->dir == 1) ? 0 : 1;
    }
    return 0;
}

/* m_00b74: state 3, cmd 2 - walk_steps[step], then draw */
static int koopa_walking(actor_t *a, gds *g) {
    int r = CALL(a->walk_steps[a->step], a, g);
    draw_at(a, g, a->frame);
    return r;
}

/* m_01248: state 3, cmd 3 - hit (stomped, a fireball, a kill star) */
static int koopa_hit(actor_t *a, gds *g) {
    if (g->unk_496 == 0) {
        g->player[g->cur_player].score += 100;
        a->unk_048--; a->unk_04a--;
        a->state = 4; a->step = 1; a->unk_02a = 0;
    } else a->unk_04e = a->cmd;
    return 0;
}

/* m_012b4: state 3, cmd 4 - destroyed */
static int koopa_destroy(actor_t *a, gds *g) {
    if (g->unk_496 == 0) {
        if (a->unk_04c == 0 && g->unk_53a == 0) {
            g->player[g->cur_player].score += 100;
            a->unk_04a--;
        } else a->unk_04c = 0;
        a->state = 4; a->step = 4; a->unk_02a = 0xe;
    } else a->unk_04e = a->cmd;
    return 0;
}

/* m_0134c: state 4, cmd 2 - steps_b[step] */
static int koopa_dying(actor_t *a, gds *g) {
    CALL(a->steps_b[a->step], a, g);
    return 0;
}

static int koopa_hit_1(actor_t *a, gds *g) {           /* m_01760 */
    draw_at(a, g, 0x13);
    if (++a->unk_02a > 0) a->step = 2;
    return 0;
}
static int koopa_hit_2(actor_t *a, gds *g) {           /* m_01814 */
    draw_at(a, g, 0x14);
    if (++a->unk_02a > 3) { a->step = 3; a->unk_02a = 2; }
    return 0;
}
static int koopa_gone(actor_t *a, gds *g) {            /* m_018ce: back to dormant */
    (void)g;
    if (a->unk_02a-- < 2) { a->state = 0; a->cmd = 2; a->timer = 0; a->dir = -1; }
    return 0;
}
static int koopa_flip(actor_t *a, gds *g) {            /* m_01386 / m_01438: shell flips over */
    draw_at(a, g, a->unk_02a);
    a->unk_02a++;
    a->step = (int2)(a->step == 4 ? 5 : 6);
    return 0;
}
static int koopa_fall(actor_t *a, gds *g) {            /* m_014de / m_015b4 / m_0168a */
    a->y += 0x10;
    obj_draw(a, a->unk_02a, a->x, a->y);
    if (g->draw_field == 1) a->drawn_y0 = a->y; else a->drawn_y1 = a->y;
    if (a->step == 8) { a->unk_02a -= 2; a->step = 6; }
    else { a->unk_02a++; a->step++; }
    if (a->y > 0xd5) { a->step = 3; a->unk_02a = 2; }
    return 0;
}

int koopa(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        int i;
        for (i = 0; i < 25; i++) a->handlers[i] = (code *)nop;
        for (i = 0; i < 24; i++) a->walk_steps[i] = (code *)nop;
        for (i = 0; i < 16; i++) a->steps_b[i] = (code *)nop;
        a->handlers[0] = (code *)koopa_init;
        a->handlers[2] = (code *)koopa_dormant;
        a->handlers[7] = (code *)koopa_wait_door;
        a->handlers[0xb] = (code *)erase;
        a->handlers[0xc] = (code *)koopa_emerge;
        a->handlers[0x10] = (code *)erase;
        a->handlers[0x11] = (code *)koopa_walking;
        a->handlers[0x12] = (code *)koopa_hit;
        a->handlers[0x13] = (code *)koopa_destroy;
        a->handlers[0x15] = (code *)erase;
        a->handlers[0x16] = (code *)koopa_dying;
        a->walk_steps[0] = (code *)koopa_walk;
        a->walk_steps[1] = (code *)koopa_turn;
        a->steps_b[1] = (code *)koopa_hit_1;
        a->steps_b[2] = (code *)koopa_hit_2;
        a->steps_b[3] = (code *)koopa_gone;
        a->steps_b[4] = (code *)koopa_flip;
        a->steps_b[5] = (code *)koopa_flip;
        a->steps_b[6] = (code *)koopa_fall;
        a->steps_b[7] = (code *)koopa_fall;
        a->steps_b[8] = (code *)koopa_fall;
    }
    if (a->state < 0 || a->state > 4 || a->cmd < 0 || a->cmd > 4) return 0;
    return CALL(a->handlers[a->state * 5 + a->cmd], a, g);
}
