/* obj_m_ninja.c - "m_ninja" (Ninji: Roy's hotel and hotel 8), from L2_s04_sub.o
 * (m_07964 .. m_08d1c). Comes out of its door, crouches, then hops (4 pixels a step,
 * an arc coded in x_max two bits at a time) - towards Mario if he's on its floor.
 * Frames: 1-10 hop right/left, 11-13 turn, 14-18 destroyed, 19-20 hit, 21-22 out of the
 * door, 23-32 crouch.
 */
#include "obj_common.h"

static void place(actor_t *a, gds *g) {
    a->y = a->drawn_y0 = a->drawn_y1 = (int2)(g->floor_y[a->floor] - 0x11);
    a->cur_floor = a->floor;
    a->state = 1;
    a->box_y0 = a->y + 4;
    a->box_y1 = a->y + 0x11;
    a->x = (int2)(g->door_x[a->col] + 4);
    a->box_x1 = a->x + 0x10;
    a->box_x0 = a->x + 2;
}

static int ninja_init(actor_t *a, gds *g) {             /* m_07964 */
    a->delay = 0x50;
    place(a, g);
    a->appear_count = 2;
    a->unk_04a = a->unk_048 = a->appear_count + 1;
    return 0;
}

/* m_08d1c: a door two floors away from Mario (the first door of that floor from the
 * right; failing that, the original's fallback search) */
static void pick_door(actor_t *a, gds *g) {
    int fl = g->mario->floor, col = 0, c, f, found = 0;
    uint16_t v;
    fl = fl < 3 ? fl + 2 : fl - 2;
    for (c = 6; c >= 0; c--) {
        v = CELL(g, fl, c) & 0xf000;
        if (v == 0x2000 || v == 0x1000) { found = 1; col = c; break; }
    }
    if (!found) {                           /* (it looks along the same floor again) */
        for (f = (g->mario->floor < 3 ? 4 : 0); g->mario->floor < 3 ? f >= 0 : f < 5;
             f += (g->mario->floor < 3 ? -1 : 1)) {
            for (c = 6; c >= 0; c--) {
                v = CELL(g, fl, c) & 0xf000;
                if (v == 0x2000 || v == 0x1000) { found = 1; fl = f; col = c; break; }
            }
            if (found) break;
        }
    }
    v = CELL(g, fl, col) & 0xf000;
    if (v == 0x2000 || v == 0x1000) { a->floor = (int2)fl; a->col = (int2)col; }
}

static int ninja_dormant(actor_t *a, gds *g) {          /* m_07bd8 */
    if (a->timer++ > a->delay) {
        a->timer = 0;
        if (a->appear_count-- > 0) {
            pick_door(a, g);
            place(a, g);
            a->unk_04e = 0;
        }
    }
    return 0;
}

static int ninja_wait_door(actor_t *a, gds *g) {        /* m_07ca2 */
    uint16_t *c = &CELL(g, a->floor, a->col), v = *c & 0xf000;
    int mx, ax, dist;
    if (g->unk_53a != 0 || (v != 0x2000 && v != 0x1000)) return 0;
    mx = g->mario->x + 0xe;
    ax = a->x + 0x10;
    if (ax < mx) { if (a->dir == -1) a->dir = 0; dist = mx - ax; }
    else         { if (a->dir == -1) a->dir = 1; dist = ax - mx; }
    if (g->mario->floor != a->floor || dist > 0x48) {
        if ((*c & 0xf000) == 0x1000) {      /* it opens its own door */
            *c = (*c & 0xfff) | 0x2009;
            g->unk_054 = 2;
        }
        a->state = 2;
        a->step = 0;
    }
    return 0;
}

static int ninja_emerge(actor_t *a, gds *g) {           /* m_07df6 */
    switch (a->step) {
    case 0:
        a->x = (int2)(g->door_x[a->col] + 4);
        a->unk_02a = 0x15; a->step = 1;
        CELL(g, a->floor, a->col) |= 4;
        break;
    case 1: a->unk_02a = 0x16; a->step = 2; break;
    case 2:
        CELL(g, a->floor, a->col) = (CELL(g, a->floor, a->col) & ~4) | 2;
        a->unk_02a = 1; a->step = 0; a->state = 3; a->x_min = 0;
        break;
    }
    draw_at(a, g, a->unk_02a);
    return 0;
}

static int ninja_moving(actor_t *a, gds *g) {           /* m_07f8e */
    int r = CALL(a->walk_steps[a->step], a, g);
    draw_at_xy(a, g, a->frame);
    return r;
}

static void boxes(actor_t *a) {
    a->box_x0 = a->x + 2; a->box_x1 = a->x + 0x10;
    a->box_y0 = a->y + 4; a->box_y1 = a->y + 0x11;
}

static int ninja_crouch(actor_t *a, gds *g) {           /* m_0806a */
    if (a->x_min == 0) {
        uint16_t *door;
        a->x_min = 1;
        a->unk_02c = (g->mario->floor == a->cur_floor && abs(g->mario->x - a->x) < 0x49) ? 0 : 10;
        door = DOOR_CELL(a->cur_floor, a->x + 0xa);
        if (door && (*door & 0xf000) == 0x1000 && (RND(g) & 3) == 1) {
            *door = (*door & 0xfff) | 0x2009;
            g->unk_054 = 2;
        }
    } else if (a->unk_02c < 1) a->unk_02a++;
    else a->unk_02c--;
    a->frame = (int2)(a->unk_02a + 0x17);
    if (a->unk_02a == 9) { a->unk_02a = 0; a->step = 1; }
    boxes(a);
    return 0;
}

static int ninja_face(actor_t *a, gds *g) {             /* m_081f8 */
    mario_t *m = g->mario;
    if (a->x >= g->door_x[6] + 4) a->dir = 1;
    else if (a->x <= g->door_x[0] + 4) a->dir = 0;
    else if (m->floor == a->cur_floor) {
        if (m->state_group == 3) a->dir = (m->x < a->x) ? 0 : 1;   /* (away from a jumping Mario) */
        else a->dir = (m->x < a->x) ? 1 : 0;
    } else a->dir = (RND(g) & 1) == 0 ? 1 : 0;
    a->frame = (a->dir == 1) ? 0xd : 0xb;
    a->step = 4;
    return 0;
}

static int ninja_hop_start(actor_t *a, gds *g) {        /* m_08364 */
    (void)g;
    a->unk_02a = 0;
    a->frame = (int2)(a->dir == 1 ? 6 : 1);
    a->x_max = 0x3e516f;                    /* the hop's height steps, 2 bits each */
    a->step = 5;
    return 0;
}

static int ninja_hop(actor_t *a, gds *g) {              /* m_083a8 */
    (void)g;
    if (++a->unk_02a < 0xc) {
        int dy = (int)(a->x_max & 3) * 2, n = a->unk_02a;
        a->x += (a->dir == 1) ? -4 : 4;
        a->y += (a->unk_02a < 6) ? -dy : dy;
        a->x_max >>= 2;
        boxes(a);
        if (n == 1 || n == 3 || n == 7 || n == 0xb) a->frame++;
    } else {
        a->unk_02a = 0; a->step = 0; a->x_min = 0;
    }
    return 0;
}

static int ninja_hit(actor_t *a, gds *g) {              /* m_084ac */
    if (g->unk_496 == 0) {
        SCORE(g, 200);
        a->unk_048--; a->unk_04a--;
        a->state = 4;
        if (a->step == 5) { a->step = 4; a->unk_02a = 0xe; a->unk_02c = 0; }
        else { a->step = 1; a->unk_02a = 0; }
    } else a->unk_04e = a->cmd;
    return 0;
}

static int ninja_destroy(actor_t *a, gds *g) {          /* m_08546 */
    if (g->unk_496 == 0) {
        if (a->unk_04c == 0 && g->unk_53a == 0) { SCORE(g, 200); a->unk_04a--; }
        else a->unk_04c = 0;
        a->state = 4; a->step = 4; a->unk_02a = 0xe; a->unk_02c = 0;
    } else a->unk_04e = a->cmd;
    return 0;
}

static int ninja_dying(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }

static int ninja_hit_1(actor_t *a, gds *g) {            /* m_08a52 */
    draw_at(a, g, 0x13);
    if (++a->unk_02a > 0) a->step = 2;
    return 0;
}
static int ninja_hit_2(actor_t *a, gds *g) {            /* m_08b06 */
    draw_at(a, g, 0x14);
    if (++a->unk_02a > 3) { a->step = 3; a->unk_02a = 2; }
    return 0;
}
static int ninja_gone(actor_t *a, gds *g) {             /* m_08bc0 */
    (void)g;
    if (a->unk_02a-- < 2) { a->state = 0; a->cmd = 2; a->timer = 0; a->dir = -1; }
    return 0;
}
static int ninja_flip(actor_t *a, gds *g) {             /* m_0861e / m_086dc */
    if (a->step == 4) draw_at_xy(a, g, a->unk_02a);
    else draw_plain(a, a->unk_02a);
    a->unk_02a++;
    a->step = (int2)(a->step == 4 ? 5 : 6);
    return 0;
}
static int ninja_fall(actor_t *a, gds *g) {             /* m_08782 / m_08872 / m_08962 */
    a->y += 0x10;
    draw_at_y(a, g, a->unk_02a);
    if (++a->unk_02c > 0) {
        a->unk_02c = 0;
        if (a->step == 8) { a->unk_02a -= 2; a->step = 6; }
        else { a->unk_02a++; a->step++; }
    }
    if (a->y > 0xe0) { a->step = 3; a->unk_02a = 2; }
    return 0;
}

int m_ninja(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, ninja_init);
        H(0, 2, ninja_dormant);
        H(1, 2, ninja_wait_door);
        H(2, 1, obj_erase);
        H(2, 2, ninja_emerge);
        H(3, 1, obj_erase);
        H(3, 2, ninja_moving);
        H(3, 3, ninja_hit);
        H(3, 4, ninja_destroy);
        H(4, 1, obj_erase);
        H(4, 2, ninja_dying);
        W(0, ninja_crouch);
        W(1, ninja_face);
        W(4, ninja_hop_start);
        W(5, ninja_hop);
        B(1, ninja_hit_1);
        B(2, ninja_hit_2);
        B(3, ninja_gone);
        B(4, ninja_flip);
        B(5, ninja_flip);
        B(6, ninja_fall);
        B(7, ninja_fall);
        B(8, ninja_fall);
    }
    return obj_dispatch(a, g);
}
