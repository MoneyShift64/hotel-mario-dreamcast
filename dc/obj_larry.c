/* obj_larry.c - "larry", the hotel 3 boss (stage 10), from L3_s10_sub.o (m_004fc .. m_02a82).
 *
 * He comes out of a door near Mario and runs along the floor at 6 px a tick (walls 0x22..0x138),
 * turning to face Mario. If Mario is on another floor he hops up or drops down a floor (3 in
 * 4 chance a tick). Facing Mario on his floor he throws one of his balls (larry_bb, up to 4,
 * linked at init). Passing closed doors he opens them (1 in 8).
 * Stomp: he staggers (steps_b 7) then is dazed for 20 ticks (step 2); a second stomp while
 * dazed (or while hopping between floors, steps 8 / 11), or any fireball / star, knocks him
 * off the screen. He is never beaten for good: 35 ticks later (10 fewer each time, down to
 * 15) he comes out of the open or closed door nearest Mario again.
 * If he reaches Mario (within 10 px) while Mario is hiding in a door (group 4) or standing in
 * a lift doorway (groups 1/2 start/end), he follows him in: Mario is frozen (group 7 state 1),
 * unk_53a = 2 and unk_530 = 1 (lift switching off), the door / lift tiles animate, and ~10
 * ticks later Mario is "dead" (group 6 state 5, fell = 1, unk_53a back to 0) so exit_check
 * ends the stage as a loss.
 * unk_25c: +0 int32 the door column he is entering, +4 int32 "hidden" (not drawn),
 *          +8.. 4 x int16 his balls (actor index + 1; the original keeps pointers there).
 * Frames: 1-10 run, 11-13 turn, 12 standing, 14-18 knocked off, 19-20 out of the door,
 * 23-24 dazed, 25-29 / 32-35 hops, 36-49 throwing.
 */
#include "obj_common.h"

static void draw_frame(actor_t *a, gds *g) { draw_at_xy(a, g, a->frame); }   /* m_024ac */

/* m_00070 updates the pointed seed itself, even without a caller assignment. */
static uint32_t rnd_step(gds *g) { return RND(g); }

static actor_t *ball(actor_t *a, gds *g, int i) {
    int16_t v;
    memcpy(&v, a->unk_25c + 8 + 2 * i, 2);
    return v > 0 ? &g->room->actors[v - 1] : NULL;
}

/* m_02a82: a door sheet tile (sx, sy) over the door he is going into */
static void door_tile(actor_t *a, int sx, int sy) {
    int c = rd25c(a, 0);
    if (a->cur_floor < 0 || a->cur_floor >= FLOORS || c < 0 || c >= COLS) return;
    hm_world->tile[a->cur_floor][c].on = 1;
    hm_world->tile[a->cur_floor][c].sx = (int16_t)sx;
    hm_world->tile[a->cur_floor][c].sy = (int16_t)sy;
}

static int col_of(int x) {                              /* m_02560 */
    x += 0x14;
    if (x < 0xb0) return x < 0x58 ? 0 : x < 0x84 ? 1 : 2;
    if (x < 0x108) return x < 0xdc ? 3 : 4;
    return x < 0x134 ? 5 : 6;
}

static int is_door(gds *g, int f, int c) {
    uint16_t v;
    if (f < 0 || f >= FLOORS) return 0;
    v = CELL(g, f, c) & 0xf000;
    return v == 0x2000 || v == 0x1000;
}

/* m_02668: choose the door to come out of - the nearest floor (below first, then above)
 * with a door, then the door on it nearest Mario but more than 84 px from him */
static void pick_door(actor_t *a, gds *g) {
    int mf = g->mario->floor, mx = g->mario->x, fl = 0, col = 0, pass, f, c, found;
    for (pass = 0; pass < 2; pass++) {
        found = 0;
        for (f = mf - pass; f >= 0 && !found; f--)
            for (c = 0; c < 7; c++) if (is_door(g, f, c)) { found = 1; col = c; fl = f; break; }
        if (!found) fl = -10;
        found = 0;
        f = mf;
        do {
            f++;
            if (f > 4 || mf - fl <= f - mf) break;
            for (c = 0; c < 7; c++) if (is_door(g, f, c)) { found = 1; col = c; fl = f; break; }
        } while (!found);
        for (c = col + 1; c < 7; c++) {
            if (!is_door(g, fl, c)) continue;
            if ((abs(mx - hm_door_x[col]) < abs(mx - hm_door_x[c]) && (abs(mx - hm_door_x[c]) > 0x54 || fl != mf)) ||
                (abs(mx - hm_door_x[col]) < 0x54 && fl == mf))
                col = c;
        }
        if (abs(mx - hm_door_x[col]) > 0x54 || fl != mf) break;
    }
    if (fl >= 0 && fl < 5 && col >= 0 && col < 7) { a->floor = (int2)fl; a->col = (int2)col; }
}

static void place(actor_t *a, gds *g) {
    a->y = a->drawn_y0 = a->drawn_y1 = (int2)(g->floor_y[a->floor] - 0x26);
    a->cur_floor = a->floor;
    a->state = 1;
    a->box_y0 = a->y + 0xc; a->box_y1 = a->y + 0x26;
    a->x = (int2)g->door_x[a->col];
    a->box_x1 = a->x + 0x1c; a->box_x0 = a->x + 10;
}

static int lr_init(actor_t *a, gds *g) {                /* m_004fc */
    int i, n = 0;
    place(a, g);
    a->delay = 0x23;
    memset(a->unk_25c + 8, 0, 8);
    for (i = 0; i < 0x18 && n < 4 && g->room->actors[i].type != 0; i++)
        if (g->room->actors[i].type == 0x2e) {
            int16_t v = (int16_t)(i + 1);
            memcpy(a->unk_25c + 8 + 2 * n++, &v, 2);
        }
    g->unk_530 = 0;
    a->unk_04a = -1; a->unk_048 = -1;
    return 0;
}

static int lr_respawn(actor_t *a, gds *g) {             /* m_008dc */
    if (a->delay < a->timer++) {
        a->timer = 0; a->unk_04e = 0;
        if (a->delay > 0xf) a->delay -= 10;
        pick_door(a, g);
        place(a, g);
    }
    g->unk_530 = 0;
    return 0;
}

static int lr_wait(actor_t *a, gds *g) {                /* m_009b2 */
    uint16_t *c;
    if (g->unk_53a != 0) return 0;
    if (door_far_from_mario(a, g, a->col * 0x2c + 0x3c, 0x10) > 0x48 || a->floor != g->mario->floor) {
        c = &CELL(g, a->floor, a->col);
        if ((*c & 0xf000) != 0x2000) { *c = (*c & 0xfff) | 0x2009; g->unk_054 = 2; }
        a->state = 2; a->step = 0;
    }
    return 0;
}

static int lr_emerge(actor_t *a, gds *g) {              /* m_00acc */
    switch (a->step) {
    case 0: CELL(g, a->floor, a->col) |= 4; a->step = 1; break;
    case 1: a->x = (int2)(a->col * 0x2c + 0x2c); a->step = 2; a->frame = 0x13; draw_frame(a, g); break;
    case 2: a->step = 3; a->frame = 0x14; draw_frame(a, g); break;
    case 3:
        CELL(g, a->floor, a->col) = (CELL(g, a->floor, a->col) & 0xfffb) | 2;
        a->unk_02a = 1; a->step = 0; a->state = 3;
        a->frame = (a->dir == 1) ? 0xd : 0xb;
        draw_frame(a, g);
        break;
    }
    return 0;
}

static int lr_moving(actor_t *a, gds *g) {              /* m_00c08 */
    int r = CALL(a->walk_steps[a->step], a, g);
    if (rd25c(a, 4) == 0) draw_frame(a, g);
    return r;
}

/* he follows Mario into a door or lift: Mario is caught */
static void caught(actor_t *a, gds *g, int step) {
    if(world_invulnerable(g))return;
    g->mario->state_group = 7; g->mario->state = 1;
    g->unk_53a = 2;
    a->step = (int2)step; a->unk_02a = 0;
    g->unk_530 = 1;
}

static int lr_run(actor_t *a, gds *g) {                 /* m_00cf4 */
    mario_t *m = g->mario;
    int sp, i;
    walk_cycle(a);
    sp = (a->step == 0xd) ? 2 : 6;
    if (a->dir == 1) { a->x = (int2)(a->x - sp); a->frame = (int2)(a->unk_02a + 6); }
    else             { a->x = (int2)(a->x + sp); a->frame = (int2)(a->unk_02a + 1); }
    if (m->floor < a->cur_floor) {
        if ((rnd_step(g) & 3) != 1) { a->step = 7; a->unk_02a = 0; }
    } else if (a->cur_floor < m->floor) {
        if ((rnd_step(g) & 3) != 1) { a->step = 10; a->unk_02a = 0; }
    } else if ((a->x < m->x && a->dir == 1) || (m->x < a->x && a->dir == 0)) {
        a->step = 1; a->unk_02a = 0;
    } else if (m->state_group == 0 || m->state_group == 3) {
        for (i = 0; i < 4; i++) {
            actor_t *b = ball(a, g, i);
            if (b && b->state == 0) { a->step = 0xd; a->unk_02a = 0; a->unk_02c = 0; break; }
        }
    }
    if (a->x > 0x138) { a->x = 0x138; a->step = 1; a->unk_02a = 0; }
    else if (a->x < 0x22) { a->x = 0x22; a->step = 1; a->unk_02a = 0; }
    if (abs(m->x - a->x) < 10) {
        if (m->floor == a->cur_floor) {
            if ((m->state_group == 1 || m->state_group == 2) &&
                ((m->state == 1 && m->sub == 3) || m->state == 0 ||
                 (m->state == 2 && (m->sub == 5 || m->sub == 6)))) {
                uint16_t *c = DOOR_CELL(m->floor, m->x + 0x10);
                if (c) {
                    if ((*c & 0xf000) == 0xf000) caught(a, g, 4);
                    else if ((*c & 0xf000) == 0xe000) caught(a, g, 5);
                }
            } else if (m->state_group == 4) caught(a, g, 6);
        }
    } else {
        uint16_t *c = DOOR_CELL(a->cur_floor, a->x + 0x14);
        if (c && (*c & 0xf000) == 0x1000 && (RND(g) & 7) == 1) {
            *c = (*c & 0xfff) | 0x2009;
            g->unk_054 = 2;
        }
    }
    a->box_x0 = a->x + 10; a->box_x1 = a->x + 0x1c;
    a->box_y0 = a->y + 0xc; a->box_y1 = a->y + 0x26;
    return 0;
}

static int lr_done(actor_t *a, gds *g) {                /* m_01ed0: Mario is gone */
    a->frame = 0xc;
    if (++a->unk_02a > 10) {
        g->mario->state_group = 6; g->mario->state = 5;
        g->unk_53a = 0;
        g->mario->fell = 1;
        g->unk_530 = 0;
    }
    return 0;
}

static int lr_turn(actor_t *a, gds *g) {                /* m_01f28 */
    (void)g;
    a->frame = (int2)((a->dir == 1) ? 0xd - a->unk_02a : a->unk_02a + 0xb);
    if (++a->unk_02a > 2) {
        a->unk_02a = 0; a->step = 0;
        a->dir = (a->dir == 0) ? 1 : 0;
    }
    return 0;
}

static int lr_dazed(actor_t *a, gds *g) {               /* m_01fbe */
    if (g->unk_53a == 0) {
        if (++a->unk_02a % 3 == 0) a->x_min = (a->x_min == 0);
        a->frame = (int2)(a->x_min + 0x17);
        if (a->unk_02a > 0x13) {
            a->step = 0;
            a->box_y0 = a->y + 0xc;
            a->unk_02a = 0; a->unk_02c = 0;
        }
    } else {
        a->state = 4; a->step = 4; a->unk_02a = 0xe; a->unk_02c = 0;
    }
    return 0;
}

/* m_0174e / m_019f4 / m_01c9e: into the lift (up / down) or the door after Mario.
 * Each entry: tick, frame (0 = keep, -1 = +1), tile sx, tile sy (-1 = none), hidden (-1 keep), sound */
typedef struct { int8_t t; int8_t frame; int16_t sx, sy; int8_t hide; int8_t snd; } follow_t;
static const follow_t follow_up[] = {
    { 0, 0, 0x66, 0x84, -1, 0 }, { 1, 0x1b, 0, -1, -1, 0 }, { 2, 0x1c, 0, -1, -1, 0 }, { 3, -1, 0, -1, -1, 0 },
    { 4, -1, 0x44, 0x84, -1, 0 }, { 5, -1, 0x22, 0x84, -1, 0 }, { 6, 0, 0, 0x84, 1, 0 },
    { 0x11, 0x15, 0, -1, 0, 0 }, { 0x12, 0x16, 0x22, 0x21, -1, 0 }, { 0x13, 0x13, 0x44, 0x21, -1, 0 },
    { 0x14, -1, 0x66, 0x21, -1, 0 }, { 0x15, 0xc, 0x44, 0x21, -1, 0 }, { 0x16, 0, 0x22, 0x21, -1, 0 },
    { 0x17, 0, 0, 0x21, -1, 0 }, { -1, 0, 0, 0, 0, 0 } };
static const follow_t follow_down[] = {
    { 0, 0, 0x132, 0x84, -1, 0 }, { 1, 0x1b, 0, -1, -1, 0 }, { 2, 0x1c, 0, -1, -1, 0 }, { 3, -1, 0, -1, -1, 0 },
    { 4, -1, 0x110, 0x84, -1, 0 }, { 5, -1, 0xee, 0x84, -1, 0 }, { 6, 0, 0xcc, 0x84, 1, 0 },
    { 0x11, 0x15, 0, -1, 0, 0 }, { 0x12, 0x16, 0xee, 0x21, -1, 0 }, { 0x13, 0x13, 0x110, 0x21, -1, 0 },
    { 0x14, -1, 0x132, 0x21, -1, 0 }, { 0x15, 0xc, 0x110, 0x21, -1, 0 }, { 0x16, 0, 0xee, 0x21, -1, 0 },
    { 0x17, 0, 0xcc, 0x21, -1, 0 }, { -1, 0, 0, 0, 0, 0 } };
static const follow_t follow_door[] = {
    { 0, 0, 0x110, 0, -1, 0 }, { 1, 0x1b, 0, -1, -1, 0 }, { 2, 0x1c, 0xee, 0, -1, 0 }, { 3, -1, 0, -1, -1, 0 },
    { 4, 0, 0x110, 0, 1, 0 }, { 5, 0, 0, 0, -1, 1 },
    { 0x12, 0x13, 0x66, 0, 0, 2 }, { 0x13, 0x14, 0x88, 0, -1, 0 }, { 0x14, 0xc, 0x88, 0, -1, 0 },
    { 0x15, 0, 0x66, 0, -1, 0 }, { -1, 0, 0, 0, 0, 0 } };

static void follow(actor_t *a, gds *g, const follow_t *tab, int last) {
    int t = a->unk_02a++;
    const follow_t *e;
    if (t == 0) {
        wr25c(a, 0, col_of(a->x));
        a->x = (int2)(hm_door_x[rd25c(a, 0)] - 4);
        a->frame = (a->dir == 1) ? 0x1a : 0x19;
    }
    for (e = tab; e->t >= 0; e++) if (e->t == t) break;
    if (e->t < 0) return;
    if (e->frame > 0) a->frame = e->frame;
    else if (e->frame < 0) a->frame++;
    if (e->hide >= 0) wr25c(a, 4, e->hide);
    if (e->sy >= 0) door_tile(a, e->sx, e->sy);
    if (e->snd) g->unk_054 = e->snd;
    if (t == last) { a->unk_02a = 0; a->step = 3; }
}
static int lr_follow_up(actor_t *a, gds *g)   { follow(a, g, follow_up, 0x17);   return 0; }
static int lr_follow_down(actor_t *a, gds *g) { follow(a, g, follow_down, 0x17); return 0; }
static int lr_follow_door(actor_t *a, gds *g) { follow(a, g, follow_door, 0x15); return 0; }

/* hopping up a floor: m_013ce, m_01444, m_014c8 */
static int lr_hop_start(actor_t *a, gds *g) {
    (void)g;
    if (a->unk_02a++ == 0) a->frame = (a->dir == 1) ? 0x1a : 0x19;
    else { a->unk_02a = 0; a->frame = 0x1b; a->step = 8; a->x_max = a->cur_floor; }
    return 0;
}
static int lr_hop_rise(actor_t *a, gds *g) {
    a->cur_floor = g->mario->floor;
    a->frame = (int2)(a->unk_02a + 0x20);
    if (++a->unk_02a < 4) { if (a->unk_02a == 3) a->y -= 4; }
    else { a->unk_02a = 0; a->step = 9; a->y -= 0x19; }
    a->box_y0 = a->y + 0xc; a->box_y1 = a->y + 0x26;
    return 0;
}
static int lr_hop_land(actor_t *a, gds *g) {
    if (a->unk_02a++ == 0) {
        a->frame = 0x1b;
        a->cur_floor = (int2)(a->x_max - 1);
        a->y = (int2)(g->floor_y[a->cur_floor] - 0x26);
        a->box_y0 = a->y + 0xc; a->box_y1 = a->y + 0x26;
    } else {
        a->frame = (a->dir == 1) ? 0x1a : 0x19;
        a->unk_02a = 1; a->step = 0;
    }
    return 0;
}

/* dropping down a floor: m_0156a, m_015e0, m_016e0 */
static int lr_drop_start(actor_t *a, gds *g) {
    (void)g;
    if (a->unk_02a++ == 0) a->frame = (a->dir == 1) ? 0x1a : 0x19;
    else { a->unk_02a = 0; a->frame = 0x1b; a->step = 0xb; a->x_max = a->cur_floor; }
    return 0;
}
static int lr_drop_fall(actor_t *a, gds *g) {
    a->frame = (int2)(0x23 - a->unk_02a);
    switch (++a->unk_02a) {
    case 1: a->cur_floor = g->mario->floor; a->y += 8; break;
    case 2: a->cur_floor = g->mario->floor; a->y += 0x19; break;
    case 3: a->cur_floor = (int2)(a->x_max + 1); a->y = (int2)(g->floor_y[a->cur_floor] - 0x26); break;
    default: a->unk_02a = 0; a->step = 0xc; break;
    }
    a->box_y0 = a->y + 0xc; a->box_y1 = a->y + 0x26;
    return 0;
}
static int lr_drop_land(actor_t *a, gds *g) {
    (void)g;
    if (a->unk_02a++ == 0) a->frame = 0x1b;
    else { a->frame = (a->dir == 1) ? 0x1a : 0x19; a->unk_02a = 1; a->step = 0; }
    return 0;
}

static int lr_throw(actor_t *a, gds *g) {               /* m_01248 */
    int i;
    if (a->dir == 1) { a->frame = (int2)(a->unk_02a + 0x2b); if (a->unk_02a != 0) a->x -= 2; }
    else             { a->frame = (int2)(a->unk_02a + 0x24); if (a->unk_02a != 0) a->x += 2; }
    a->box_x0 = a->x + 10; a->box_x1 = a->x + 0x1c;
    if (++a->unk_02c > 2 && ++a->unk_02a > 6) {
        a->step = 0; a->unk_02a = 1;
        for (i = 0; i < 4; i++) {
            actor_t *b = ball(a, g, i);
            if (b && b->state == 0) {
                b->state = 3; b->step = 0;
                b->dir = a->dir;
                b->y = (int2)(a->y + 0x1d);
                b->cur_floor = a->cur_floor;
                b->x = (int2)(a->x + 0x14);
                b->x_min = (int4)((rnd_step(g) & 2) + 10);
                g->unk_054 = 0xc;
                return 10;
            }
        }
    }
    return 0;
}

static int lr_hit(actor_t *a, gds *g) {                 /* m_02070: stomped */
    if (g->unk_496 == 0) {
        if (a->step == 2 || a->step == 8 || a->step == 0xb) {
            a->state = 4; a->step = 4; a->box_y0 = a->y + 0xc; a->unk_02a = 0xe; a->unk_02c = 0;
        } else { a->state = 4; a->step = 7; a->unk_02a = 0; }
    } else a->unk_04e = a->cmd;
    return 0;
}

static int lr_destroy(actor_t *a, gds *g) {             /* m_02160: fireball / star (no score) */
    if (g->unk_496 == 0) {
        if (a->unk_04c != 0 || g->unk_53a != 0) a->unk_04c = 0;
        a->state = 4; a->step = 4; a->unk_02a = 0xe; a->unk_02c = 0;
    } else a->unk_04e = a->cmd;
    return 0;
}

static int lr_dying(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }

static int lr_stagger(actor_t *a, gds *g) {             /* m_0222c */
    a->frame = 0x17;
    draw_frame(a, g);
    if (++a->unk_02a > 1) { a->state = 3; a->step = 2; a->box_y0 += 8; a->x_min = 0; }
    return 0;
}
static int lr_gone(actor_t *a, gds *g) {                /* m_02350 */
    (void)g;
    if (a->unk_02a-- < 2) { a->state = 0; a->cmd = 2; a->timer = 0; a->dir = -1; }
    return 0;
}
static int lr_knocked(actor_t *a, gds *g) {             /* m_02280 */
    a->frame = a->unk_02a; draw_frame(a, g); a->unk_02a++; a->step = 5; return 0;
}
static int lr_knocked2(actor_t *a, gds *g) {            /* m_022b4 */
    a->frame = a->unk_02a; draw_frame(a, g); a->unk_02a++; a->step = 6; return 0;
}
static int lr_fall(actor_t *a, gds *g) {                /* m_022e8 */
    a->y += 0x10;
    a->frame = a->unk_02a;
    draw_frame(a, g);
    if (++a->unk_02a > 0x12) a->unk_02a -= 3;
    if (a->y > 0xcb) { a->step = 3; a->unk_02a = 2; }
    return 0;
}

int larry(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, lr_init);
        H(0, 2, lr_respawn);
        H(1, 2, lr_wait);
        H(2, 1, obj_erase);
        H(2, 2, lr_emerge);
        H(3, 1, obj_erase);
        H(3, 2, lr_moving);
        H(3, 3, lr_hit);
        H(3, 4, lr_destroy);
        H(4, 1, obj_erase);
        H(4, 2, lr_dying);
        W(0, lr_run);
        W(1, lr_turn);
        W(2, lr_dazed);
        W(3, lr_done);
        W(4, lr_follow_up);
        W(5, lr_follow_down);
        W(6, lr_follow_door);
        W(7, lr_hop_start);
        W(8, lr_hop_rise);
        W(9, lr_hop_land);
        W(10, lr_drop_start);
        W(11, lr_drop_fall);
        W(12, lr_drop_land);
        W(13, lr_throw);
        B(3, lr_gone);
        B(4, lr_knocked);
        B(5, lr_knocked2);
        B(6, lr_fall);
        B(7, lr_stagger);
    }
    return obj_dispatch(a, g);
}
