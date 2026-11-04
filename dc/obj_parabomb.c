/* obj_parabomb.c - "parabomb" (para-bomb: a bob-omb on a parachute), from L2_s07_sub.o
 * (m_0d3a2 .. m_0e684). Drifts down from the top of the screen to a random floor, walks
 * like a goomba until its fuse (90-153 ticks) runs out, flashes, then explodes: the
 * blast hurts Mario and destroys any other object it touches. Stomping it lights the
 * fuse at once; stomping it while it lands or explodes hurts.
 * unk_25c holds int32s: +0 the blast's shrinking (a nibble a tick), +4 the blast y,
 * +8 floor, +0xc landing y, +0x10 landing floor, +0x14 the x it started drifting from.
 * Frames: 1-10 walk, 11-13 turn, 14-18 drifting, 19-21 landing, 22-28 blast,
 * 29-32 parachute opening, 34-36 fuse.
 */
#include "obj_common.h"

static void at_top(actor_t *a, gds *g, int land_floor) {
    a->y = a->drawn_y1 = a->drawn_y0 = 1;
    a->cur_floor = a->floor;
    a->state = 1;
    a->box_y0 = a->y + 0x32;
    a->box_y1 = a->y + 0x48;
    wr25c(a, 0xc, g->floor_y[land_floor] - 0x48);
    wr25c(a, 0x10, land_floor);
    a->x = (int2)(g->door_x[a->col] - 0x14);
    a->box_x1 = a->x + 0x2a;
    a->box_x0 = a->x + 0x1c;
}

static int pb_init(actor_t *a, gds *g) {                /* m_0d3a2 */
    a->speed = HOTEL(g) == 8 ? 4 : 2;
    at_top(a, g, a->floor);
    a->delay = (int2)(6 * (*(int32_t *)(g->stage_info + 8) - a->delay));
    a->timer = 0;
    a->appear_count = 0x5a;
    return 0;
}

static int pb_again(actor_t *a, gds *g) {               /* m_0d6a6 */
    if (a->appear_count > 0) {
        a->appear_count--;
        at_top(a, g, smod((int)(RND(g) & 0xffff), 5));
        a->delay = 0x32;
        a->unk_04e = 0;
    }
    return 0;
}

static int pb_wait(actor_t *a, gds *g) {                /* m_0d78c */
    mario_t *m = g->mario;
    if (g->unk_53a != 0 || a->timer++ <= a->delay) return 0;
    /* not straight onto Mario's head on the top floor */
    if (m->floor == 0 && (m->state_group == 0 || m->state_group == 3) && abs(m->x - a->x - 0x14) < 0x22)
        return 0;
    a->timer = 0;
    a->unk_02a = 0;
    a->state = 3;
    a->step = 4;
    a->box_y0 = 1;
    a->box_y1 = 0x14;
    a->box_x0 = a->x + 0x1c;
    a->box_x1 = a->x + 0x2a;
    if (a->dir == -1) a->dir = (RND(g) & 1) == 0 ? 1 : 0;
    return 0;
}

static int pb_moving(actor_t *a, gds *g) {              /* m_0d8c8 */
    int r = CALL(a->walk_steps[a->step], a, g);
    draw_at_xy(a, g, a->frame);
    return r;
}

static void blast_setup(actor_t *a) {
    a->step = 3;
    wr25c(a, 0, 0x0f44458a);
    wr25c(a, 4, a->y + 0x16);
    wr25c(a, 8, a->cur_floor);
    a->unk_02a = 0;
}

static int pb_walk(actor_t *a, gds *g) {                /* m_0d9a4 */
    if (a->unk_02e == 0) {
        if (a->unk_02a == 4) { a->unk_02a--; a->unk_02e = 1; } else a->unk_02a++;
    } else if (a->unk_02a == 0) { a->unk_02a++; a->unk_02e = 0; }
    else a->unk_02a--;
    if (a->dir == 1) { a->x -= a->speed; a->frame = (int2)(a->unk_02a + 6); }
    else             { a->x += a->speed; a->frame = (int2)(a->unk_02a + 1); }
    if (a->unk_02a == 0 && a->cur_floor >= 1 && g->mario->floor == a->cur_floor) {
        mario_t *m = g->mario;
        int above = a->cur_floor - 1, i, same_dir = 0, other_dir = 0, x = a->x;
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
    if (a->x <= a->x_min && a->dir == 1) { a->step = 1; a->unk_02a = 0; }
    else if (a->x >= a->x_max && a->dir == 0) { a->step = 1; a->unk_02a = 0; }
    if (--a->unk_02c <= 0) { a->step = 2; a->unk_02a = 0; }       /* the fuse */
    a->box_x0 = a->x + 0x1c;
    a->box_x1 = a->x + 0x2a;
    return 0;
}

static int pb_turn(actor_t *a, gds *g) {                /* m_0de24 */
    (void)g;
    a->frame = (int2)((a->dir == 1) ? 0xd - a->unk_02a : a->unk_02a + 0xb);
    if (++a->unk_02a > 2) {
        a->unk_02a = 0;
        a->step = 0;
        a->dir = (a->dir == 0) ? 1 : 0;
    }
    if (a->unk_02c-- < 2) { a->step = 2; a->unk_02a = 0; }
    return 0;
}

static int pb_fuse(actor_t *a, gds *g) {                /* m_0e1e0: flashing */
    (void)g;
    if (++a->unk_02a < 0xf) {
        a->frame++;
        if (a->frame > 0x24 || a->frame < 0x22) a->frame = 0x22;
    } else {
        blast_setup(a);
        a->frame = 0x15;
    }
    return 0;
}

/* m_0e684: the blast destroys every other object it touches (no points: unk_04c) */
static void blast_hits(actor_t *a, gds *g) {
    actor_t *p;
    int i;
    for (i = 0, p = g->room->actors; i < 24 && p->type != 0; i++, p++) {
        int hit;
        if (p->state != 3 || p == a) continue;
        hit = !(p->box_y1 < a->box_y0 || a->box_y1 < p->box_y0 || p->box_x1 < a->box_x0 || a->box_x1 < p->box_x0);
        if (!hit && p->unk_03c != 0)
            hit = a->box_y0 <= p->hit_y1 && p->hit_y0 <= a->box_y1 && a->box_x0 <= p->hit_x1 && p->hit_x0 <= a->box_x1;
        if (hit) {
            p->cmd = 4;
            p->unk_04c = 1;
            CALL(p->handler, p, g);
        }
    }
}

static int pb_blast(actor_t *a, gds *g) {               /* m_0e272 */
    int32_t pat = rd25c(a, 0);
    int n = pat & 0xf;
    wr25c(a, 0, pat >> 4);
    a->y = (int2)rd25c(a, 4);
    a->cur_floor = g->mario->floor;         /* (it reaches Mario whatever his floor) */
    a->box_x0 = (int2)(n * 2 + a->x);
    a->box_x1 = (int2)(a->x + 0x48 - n * 2);
    a->box_y0 = (int2)(n * 2 + a->y);
    a->box_y1 = (int2)(a->y + 0x48 - n * 2);
    a->frame = (int2)(a->unk_02a + 0x16);
    blast_hits(a, g);
    if (++a->unk_02a < 7) {
        if (a->frame == 0x16) g->unk_054 = 0xd;
    } else {
        a->state = 4; a->step = 3; a->unk_02a = 2;
    }
    return 0;
}

static int pb_open(actor_t *a, gds *g) {                /* m_0ded4: the parachute opens */
    a->cur_floor = g->mario->floor;
    a->box_y0 += 7;
    a->box_y1 += 7;
    a->frame = (int2)(a->unk_02a + 0x1d);
    a->unk_02a++;
    if (a->frame > 0x20) {
        if (g->unk_53a == 0) {
            a->unk_02a = 2; a->unk_02e = 0; a->step = 5;
            wr25c(a, 0x14, a->x);
        } else blast_setup(a);
    }
    return 0;
}

static int pb_drift(actor_t *a, gds *g) {               /* m_0df76 */
    a->x = (int2)((a->unk_02a - 2) * 8 + (int16_t)rd25c(a, 0x14));
    a->box_x0 = a->x + 0x1c;
    a->box_x1 = a->x + 0x2a;
    a->cur_floor = g->mario->floor;
    a->y += 2;
    a->box_y0 = a->y + 0x32;
    a->box_y1 = a->y + 0x46;
    if (a->unk_02e == 0) {
        if (a->unk_02a < 4) a->frame = (int2)(a->unk_02a++ + 0xe);
        else { a->unk_02e = 1; a->frame = (int2)(a->unk_02a + 0xe); }
    } else if (a->unk_02a < 1) { a->unk_02e = 0; a->frame = (int2)(a->unk_02a + 0xe); }
    else a->frame = (int2)(a->unk_02a-- + 0xe);
    if (a->y >= rd25c(a, 0xc)) {
        a->y = (int2)rd25c(a, 0xc);
        a->box_y0 = a->y + 0x32;
        a->box_y1 = a->y + 0x48;
        a->box_x0 = a->x + 0x1c;
        a->box_x1 = a->x + 0x2a;
        a->cur_floor = (int2)rd25c(a, 0x10);
        a->unk_02a = 0;
        a->step = 6;
    }
    return 0;
}

static int pb_land(actor_t *a, gds *g) {                /* m_0e0b8 */
    uint16_t l0, l6;
    a->frame = (int2)(a->unk_02a + 0x13);
    if (++a->unk_02a > 3) {
        a->frame = (a->dir == 1) ? 0xd : 0xb;
        a->step = 0;
        a->unk_02a = 1;
        a->unk_02c = (int2)((RND(g) & 0x3f) + 0x5a);
        l0 = CELL(g, a->cur_floor, 0) & 0xf000;
        l6 = CELL(g, a->cur_floor, 6) & 0xf000;
        a->x_min = (l0 == 0xf000 || l0 == 0xe000) ? 0x38 : 0xe;
        a->x_max = (l6 == 0xf000 || l6 == 0xe000) ? 0x100 : 0x12a;
    }
    return 0;
}

static int pb_hit(actor_t *a, gds *g) {                 /* m_0e33a: stomped */
    switch (a->step) {
    case 0: case 1: case 6:
        if (g->unk_496 == 0) { a->step = 2; a->unk_02a = 0; }
        else a->unk_04e = a->cmd;
        return 0;
    case 3: case 4: case 5:
        return 1;                           /* (Mario gets hurt) */
    }
    return 0;
}

static int pb_destroy(actor_t *a, gds *g) {             /* m_0e426 */
    if (g->unk_496 == 0) {
        if (a->step != 3 && a->step != 4) {
            if (a->unk_04c != 0 || g->unk_53a != 0) a->unk_04c = 0;
            blast_setup(a);
        }
    } else a->unk_04e = a->cmd;
    return 2;
}

static int pb_dying(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }

static int pb_gone(actor_t *a, gds *g) {                /* m_0e528 */
    (void)g;
    if (a->unk_02a-- < 2) { a->state = 0; a->cmd = 2; a->timer = 0; a->dir = -1; }
    return 0;
}

int parabomb(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, pb_init);
        H(0, 2, pb_again);
        H(1, 2, pb_wait);
        H(3, 1, obj_erase);
        H(3, 2, pb_moving);
        H(3, 3, pb_hit);
        H(3, 4, pb_destroy);
        H(4, 1, obj_erase);
        H(4, 2, pb_dying);
        W(0, pb_walk);
        W(1, pb_turn);
        W(2, pb_fuse);
        W(3, pb_blast);
        W(4, pb_open);
        W(5, pb_drift);
        W(6, pb_land);
        B(3, pb_gone);
    }
    return obj_dispatch(a, g);
}
