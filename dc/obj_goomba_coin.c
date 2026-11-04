/* obj_goomba_coin.c - the goomba and coin objects, ported from the stage modules.
 *
 * Source: L1_s01_sub.o (Morton's hotel, stage 1), entry points "goomba" and "coin",
 * decompiled with tools/decompile.py --module. The original function names are given
 * as m_<offset> (offsets in the module). Drawing a compiled sprite routine,
 * frames[n](draw_a0/a1 + x + y*384), becomes obj_draw(a, n, x, y); frames[0] of every
 * object is its eraser and is not drawn. The double-buffer bookkeeping (drawn_x0/x1...)
 * is kept as in the original.
 */
#include "world.h"

#define CELL(g, f, c) (((uint16_t *)((g)->stage_info + 0x12))[(f) * COLS + (c)])
#define RND(g) ((g)->unk_826 = (int4)hm_rnd((uint32_t *)&(g)->unk_826))

static int nop(actor_t *a, gds *g) { (void)a; (void)g; return 0; }

/* draw into "the current plane-A buffer" (draw_field 1 or 2 on the CD-i) */
static void draw_at(actor_t *a, gds *g, int frame, int x, int y) {
    obj_draw(a, frame, x, y);
    if (g->draw_field == 1) a->drawn_x0 = x; else a->drawn_x1 = x;
}

/* =========================================================================== goomba */

static void goomba_place_at_door(actor_t *a, gds *g) {
    int2 v = g->door_x[a->col];
    a->x = a->drawn_x0 = a->drawn_x1 = v;
    v = g->floor_y[a->floor] - 0x18;
    a->y = a->drawn_y0 = a->drawn_y1 = v;
    a->cur_floor = a->floor;
    a->box_x0 = a->x + 9;  a->box_x1 = a->x + 0xf;
    a->box_y0 = a->y + 4;  a->box_y1 = a->y + 0x17;
    a->hit_x0 = a->x + 5;  a->hit_x1 = a->x + 0x13;
    a->hit_y0 = a->y + 9;  a->hit_y1 = a->y + 0x17;
}

/* m_00444: state 0, cmd 0 - set up */
static int goomba_init(actor_t *a, gds *g) {
    uint16_t lift0, lift6;
    a->speed = (g->player[g->cur_player].hotel == 8) ? 4 : 2;
    goomba_place_at_door(a, g);
    a->unk_03c = 1;
    if (a->start_mode == 1) {                 /* starts out walking */
        a->unk_02a = 1; a->state = 3; a->step = 0;
    } else if (a->start_mode == 2) {          /* starts standing in its doorway */
        CELL(g, a->floor, a->col) |= 4;
        a->unk_02a = 1; a->state = 2; a->step = 0;
    } else {
        a->state = 1;                         /* behind its door */
    }
    if (a->dir == -1) a->dir = (g->mario->x < a->x) ? 1 : 0;
    if (a->delay == 0) a->delay = 0x28;
    if (a->appear_count == 0) a->appear_count = 1;
    else a->appear_count--;
    a->unk_048 = a->unk_04a = a->appear_count + 1;
    /* walking limits: further in if there's a lift at either end of the floor */
    lift0 = CELL(g, a->floor, 0) & 0xf000;
    lift6 = CELL(g, a->floor, 6) & 0xf000;
    a->x_min = (lift0 == 0xf000 || lift0 == 0xe000) ? 0x4c : 0x22;
    a->x_max = (lift6 == 0xf000 || lift6 == 0xe000) ? 0x11c : 0x146;
    return 0;
}

/* m_0082c: state 0, cmd 2 - gone: wait, then come back to its door */
static int goomba_dormant(actor_t *a, gds *g) {
    if (a->timer++ > a->delay) {
        a->timer = 0;
        if (a->appear_count-- > 0) {
            goomba_place_at_door(a, g);
            a->delay = 0x28;
            a->state = 1;
            a->unk_04e = 0;
        }
    }
    return 0;
}

/* m_0091c: state 1, cmd 2 - behind its door: come out when the door is open and
 * Mario is on this floor but more than 72 pixels away */
static int goomba_wait_door(actor_t *a, gds *g) {
    if ((CELL(g, a->floor, a->col) & 0xf000) == 0x2000 && g->mario->floor == a->floor) {
        int mx = g->mario->x + 0xe, ax = a->x + 0xc, dist;
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

/* m_0168a: cmd 1 - erase at the last drawn place (nothing to do here) */
static int erase(actor_t *a, gds *g) { (void)a; (void)g; return 0; }

/* m_00a1e: state 2, cmd 2 - stepping out of the doorway */
static int goomba_emerge(actor_t *a, gds *g) {
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
    draw_at(a, g, a->frame, a->x, a->y);
    return 0;
}

/* m_00c7a: walk step */
static int goomba_walk(actor_t *a, gds *g) {
    int2 x;
    uint16_t *door;
    if (a->unk_02e == 0) {                     /* walk cycle 0..4..0 */
        if (a->unk_02a == 4) { a->unk_02a--; a->unk_02e = 1; } else a->unk_02a++;
    } else if (a->unk_02a == 0) { a->unk_02a++; a->unk_02e = 0; }
    else a->unk_02a--;
    if (a->dir == 1) { x = a->x - a->speed; a->frame = a->unk_02a + 6; }
    else             { x = a->x + a->speed; a->frame = a->unk_02a + 1; }
    a->x = x;

    /* at the start of each cycle, on Mario's floor: turn towards him unless another
     * goomba on the floor above is already heading his way (the original's logic) */
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
    a->box_x0 = x + 9;  a->box_x1 = x + 0xf;
    a->hit_x0 = x + 5;  a->hit_x1 = x + 0x13;

    /* passing a closed door: 1 in 64 chance of opening it */
    door = hm_door_cell_at(&hm_world->doors, a->cur_floor, a->x + 0xc);
    if (door && (*door & 0xf000) == 0x1000) {
        if ((RND(g) & 0x3f) == 1) {
            *door = (*door & 0xfff) | 0x2009;     /* open, worth 50 when closed, changed */
            g->unk_054 = 2;                       /* door sound */
        }
    }
    return 0;
}

/* m_0114a: turning round */
static int goomba_turn(actor_t *a, gds *g) {
    (void)g;
    a->frame = (a->dir == 1) ? 0xd - a->unk_02a : a->unk_02a + 0xb;
    if (++a->unk_02a > 2) {
        a->unk_02a = 1;
        a->unk_02e = 0;
        a->step = 0;
        a->dir = (a->dir == 1) ? 0 : 1;
    }
    return 0;
}

/* m_00baa: state 3, cmd 2 - walk_steps[step], then draw */
static int goomba_walking(actor_t *a, gds *g) {
    int r = ((int (*)(actor_t *, gds *))a->walk_steps[a->step])(a, g);
    draw_at(a, g, a->frame, a->x, a->y);
    return r;
}

/* m_011e8: state 3, cmd 3 - hit (by a fireball or a kill star) */
static int goomba_hit(actor_t *a, gds *g) {
    if (g->unk_496 == 0) {
        g->player[g->cur_player].score += 50;
        a->unk_048--; a->unk_04a--;
        a->state = 4; a->step = 1; a->unk_02a = 0;
    } else a->unk_04e = a->cmd;                 /* (handled after the collision pass) */
    return 0;
}

/* m_01254: state 3, cmd 4 - destroyed (Mario invincible / stomp) */
static int goomba_destroy(actor_t *a, gds *g) {
    if (g->unk_496 == 0) {
        if (a->unk_04c == 0 && g->unk_53a == 0) {
            g->player[g->cur_player].score += 50;
            a->unk_04a--;
        } else a->unk_04c = 0;
        a->state = 4; a->step = 4; a->unk_02a = 0;
    } else a->unk_04e = a->cmd;
    return 0;
}

/* m_012ea: state 4, cmd 2 - steps_b[step] */
static int goomba_dying(actor_t *a, gds *g) {
    ((int (*)(actor_t *, gds *))a->steps_b[a->step])(a, g);
    return 0;
}

static int goomba_hit_1(actor_t *a, gds *g) {          /* m_014e2 */
    draw_at(a, g, 0x13, a->x, a->y);
    if (++a->unk_02a > 0) a->step = 2;
    return 0;
}
static int goomba_hit_2(actor_t *a, gds *g) {          /* m_01596 */
    draw_at(a, g, 0x14, a->x, a->y);
    if (++a->unk_02a > 3) { a->step = 3; a->unk_02a = 2; }
    return 0;
}
static int goomba_gone(actor_t *a, gds *g) {           /* m_01650: back to dormant */
    (void)g;
    if (a->unk_02a-- < 2) { a->state = 0; a->cmd = 2; a->timer = 0; a->dir = -1; }
    return 0;
}
static int goomba_squash(actor_t *a, gds *g) {         /* m_01324 */
    draw_at(a, g, a->unk_02a + 0xe, a->x, a->y);
    if (++a->unk_02a > 1) { a->step = 5; a->unk_02a = 0; }
    return 0;
}
static int goomba_fall(actor_t *a, gds *g) {           /* m_013f4: drops off the screen */
    a->y += 0x10;
    obj_draw(a, a->unk_02a + 0x10, a->x, a->y);
    if (g->draw_field == 1) a->drawn_y0 = a->y; else a->drawn_y1 = a->y;
    if (++a->unk_02a > 2) a->unk_02a = 0;
    if (a->y > 0xd9) { a->step = 3; a->unk_02a = 2; }
    return 0;
}

/* entry point (module offset 0x222) */
int goomba(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        int i;
        for (i = 0; i < 25; i++) a->handlers[i] = (code *)nop;
        for (i = 0; i < 24; i++) a->walk_steps[i] = (code *)nop;
        for (i = 0; i < 16; i++) a->steps_b[i] = (code *)nop;
        a->handlers[0 * 5 + 0] = (code *)goomba_init;
        a->handlers[0 * 5 + 2] = (code *)goomba_dormant;
        a->handlers[1 * 5 + 2] = (code *)goomba_wait_door;
        a->handlers[2 * 5 + 1] = (code *)erase;
        a->handlers[2 * 5 + 2] = (code *)goomba_emerge;
        a->handlers[3 * 5 + 1] = (code *)erase;
        a->handlers[3 * 5 + 2] = (code *)goomba_walking;
        a->handlers[3 * 5 + 3] = (code *)goomba_hit;
        a->handlers[3 * 5 + 4] = (code *)goomba_destroy;
        a->handlers[4 * 5 + 1] = (code *)erase;
        a->handlers[4 * 5 + 2] = (code *)goomba_dying;
        a->walk_steps[0] = (code *)goomba_walk;
        a->walk_steps[1] = (code *)goomba_turn;
        a->steps_b[1] = (code *)goomba_hit_1;
        a->steps_b[2] = (code *)goomba_hit_2;
        a->steps_b[3] = (code *)goomba_gone;
        a->steps_b[4] = (code *)goomba_squash;
        a->steps_b[5] = (code *)goomba_fall;
    }
    if (a->state < 0 || a->state > 4 || a->cmd < 0 || a->cmd > 4) return 0;
    return ((int (*)(actor_t *, gds *))a->handlers[a->state * 5 + a->cmd])(a, g);
}

/* ============================================================================= coin */

/* m_00110: does a coin that rolls off the edge drop out (1) or just vanish (0)? */
static int coin_drops_out(int hotel, int stage) {
    switch (hotel) {
    case 0: return !(stage == 3 || stage == 6 || stage == 9);
    case 1: case 4: case 7: return 1;
    }
    return 0;
}

/* m_058ce: state 0, cmd 0 - set up */
static int coin_init(actor_t *a, gds *g) {
    if (g->item_hit < 10) {
        if (a->start_mode == 1) {              /* an invisible coin spot */
            a->y = g->floor_y[a->floor];
            a->cur_floor = a->floor;
            a->box_y0 = a->y - 0x23;
            a->box_y1 = a->box_y0 + 1;
            a->x = (a->col + 1) * 0x2c;
            a->box_x0 = a->x + 0x16;
            a->box_x1 = a->box_x0 + 1;
            a->state = 3; a->step = 0;
            a->delay = 5;
            a->hit_x0 = 0;
        } else {
            a->timer = 0;
            if (a->delay == 0) {
                if (a->start_mode == 2) a->unk_036 = -0x1e - (RND(g) & 0x1f);
                else if (RND(g) & 1)    a->unk_036 = -(RND(g) & 0xf);
                else                    a->unk_036 = -(RND(g) & 0x3f);
                a->hit_x0 = 0;
            } else {                           /* appears after a set time into the stage */
                int4 limit = *(int4 *)(g->stage_info + 8);
                a->delay = (int2)(6 * (limit - a->delay));
                a->hit_x0 = 1;
            }
        }
        if (a->appear_count == 0) a->appear_count = (RND(g) & 3) + 3;
    } else {                                   /* made from an enemy by item 11/12 */
        a->floor = a->cur_floor;
        a->y = a->drawn_y0 = a->drawn_y1 = g->floor_y[a->floor] - 0x18;
        a->box_y0 = a->y + 2;  a->box_y1 = a->y + 0x18;
        a->box_x1 = a->x + 0x10; a->box_x0 = a->x + 4;
        a->dir = (0xb5 < a->x);
        a->state = 3; a->step = 0;
    }
    return 0;
}

/* m_05bee: state 0, cmd 2 - waiting to come out again */
static int coin_dormant(actor_t *a, gds *g) {
    uint16_t door = CELL(g, a->floor, a->col) & 0xf000;
    if (++a->unk_036 > 0x13) a->hit_x0 = 1;
    if (door == 0x1000 || a->start_mode == 1 || a->hit_x0 != 0) a->timer++;
    if (a->timer > a->delay) {
        if (door == 0x1000) a->hit_x0 = 0;
        a->timer = 0;
        if (a->appear_count-- > 0) {
            if (a->start_mode == 1) { a->state = 3; a->step = 0; }
            else {
                a->state = 1;
                a->cur_floor = a->floor;
                a->y = a->drawn_y0 = a->drawn_y1 = g->floor_y[a->floor] - 0x18;
                a->box_y0 = a->y + 2;  a->box_y1 = a->y + 0x18;
            }
            a->unk_04e = 0;
        }
    }
    return 0;
}

/* m_05d40: state 1, cmd 2 - behind its door: roll out when it opens */
static int coin_wait_door(actor_t *a, gds *g) {
    if ((a->start_mode != 2 || g->player[g->cur_player].hotel != 7 || g->player[g->cur_player].unk_10 != 0) &&
        g->unk_53a == 0 && (CELL(g, a->floor, a->col) & 0xf000) == 0x2000) {
        if (a->dir == -1) a->dir = (g->mario->state & 1) ? 0 : 1;
        if (a->hit_x0 == 0) a->x = (a->dir == 0) ? a->col * 0x2c + 0x5a : a->col * 0x2c + 10;
        else a->x = (a->col + 1) * 0x2c;
        a->box_x0 = a->x + 4;
        a->box_x1 = a->x + 0x10;
        a->state = 3; a->step = 0;
    }
    return 0;
}

/* m_06052: rolling */
static int coin_roll(actor_t *a, gds *g) {
    int2 d;
    if (a->start_mode == 1) return 0;
    if (--a->unk_02c < 1) {
        a->unk_02c = 1;
        if (++a->unk_02a > 5) a->unk_02a = 0;
    }
    d = (a->start_mode == 2) ? 4 : 2;
    a->x += (a->dir == 1) ? -d : d;
    if (a->x > 0x158 || a->x < 0x14) {         /* off the edge */
        a->state = 4;
        if (!coin_drops_out(g->player[g->cur_player].hotel, g->player[g->cur_player].stage)) {
            a->step = 2; a->unk_02a = 2;
        } else {
            a->step = 3; a->unk_02c = 1;
        }
    }
    d = a->unk_02a * -4 + 0xc; if (d < 0) d = -d;
    a->box_y0 = d + a->y + 1;
    d = a->unk_02a * -2 + 6;   if (d < 0) d = -d;
    d = 8 - d;
    a->box_y1 = (a->y + 0x18) - d;
    a->box_x0 = d + a->x;
    a->box_x1 = (a->x + 0x14) - d;
    a->frame = a->unk_02a + ((a->start_mode == 2) ? 7 : 1);
    return 0;
}

/* m_05f70: state 3, cmd 2 */
static int coin_rolling(actor_t *a, gds *g) {
    int r = ((int (*)(actor_t *, gds *))a->walk_steps[a->step])(a, g);
    if (a->start_mode != 1) draw_at(a, g, a->frame, a->x, a->y);
    return r;
}

/* m_0629c: state 3, cmd 3 - Mario picked it up */
static int coin_collect(actor_t *a, gds *g) {
    if (g->unk_496 == 0) {
        if (a->start_mode == 2) {
            g->player[g->cur_player].score += 1000;
            g->player[g->cur_player].coins += 10;
            if (g->item_hit < 10) g->unk_054 = 6;
        } else {
            g->player[g->cur_player].score += 100;
            g->player[g->cur_player].coins += 1;
        }
        a->state = 4;
        a->step = (a->start_mode == 1) ? 5 : 0;
        a->unk_02a = 0;
    } else a->unk_04e = a->cmd;
    return 0;
}

/* m_06384: state 3, cmd 4 */
static int coin_cmd4(actor_t *a, gds *g) {
    if (g->unk_496 == 0) {
        if (a->start_mode == 1) {
            if (g->unk_53a != 0) { a->state = 0; a->step = 0; a->unk_02c = 0; a->unk_02a = 0; a->appear_count = 0; }
        } else { a->state = 4; a->step = 3; a->unk_02c = 0; a->unk_02a = 0; }
    } else a->unk_04e = a->cmd;
    return 0;
}

/* m_0641c: state 4, cmd 2 */
static int coin_gone(actor_t *a, gds *g) {
    ((int (*)(actor_t *, gds *))a->steps_b[a->step])(a, g);
    return 0;
}

static void coin_reset(actor_t *a, gds *g) {
    a->state = 0;
    a->timer = 0;
    a->delay = (a->start_mode == 1) ? 5 : 0;
    a->dir = -1;
    a->unk_036 = -(int2)(RND(g) & 0x3f);
    a->hit_x0 = 0;
}
static int coin_sparkle_1(actor_t *a, gds *g) {        /* m_06456 */
    draw_at(a, g, 0xd, a->x, a->y);
    if (++a->unk_02a > 0) a->step = 1;
    return 0;
}
static int coin_sparkle_2(actor_t *a, gds *g) {        /* m_0650a */
    draw_at(a, g, 0xe, a->x, a->y);
    if (++a->unk_02a > 1) { a->step = 2; a->unk_02a = 2; }
    return 0;
}
static int coin_vanish(actor_t *a, gds *g) {           /* m_065c4 */
    if (--a->unk_02a == 0) coin_reset(a, g);
    return 0;
}
static int coin_drop_1(actor_t *a, gds *g) {           /* m_0663e */
    draw_at(a, g, (a->start_mode == 2) ? 0xb : 5, a->x, a->y);
    if (++a->unk_02c == 2) a->step = 4;
    return 0;
}
static int coin_drop_2(actor_t *a, gds *g) {           /* m_0672e: falls off the screen */
    a->y += 0x10;
    if (a->y < 0xda) {
        obj_draw(a, (a->start_mode == 2) ? 0xb : 5, a->x, a->y);
        if (g->draw_field == 1) a->drawn_y0 = a->y; else a->drawn_y1 = a->y;
    } else a->step = 5;
    return 0;
}
static int coin_reset_step(actor_t *a, gds *g) {       /* m_0682a */
    a->unk_02c = 0;
    a->unk_02a = 0;
    coin_reset(a, g);
    return 0;
}

/* entry point (module offset 0x56c8) */
int coin(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        int i;
        for (i = 0; i < 25; i++) a->handlers[i] = (code *)nop;
        for (i = 0; i < 24; i++) a->walk_steps[i] = (code *)nop;
        for (i = 0; i < 16; i++) a->steps_b[i] = (code *)nop;
        a->handlers[0 * 5 + 0] = (code *)coin_init;
        a->handlers[0 * 5 + 2] = (code *)coin_dormant;
        a->handlers[1 * 5 + 2] = (code *)coin_wait_door;
        a->handlers[3 * 5 + 1] = (code *)erase;
        a->handlers[3 * 5 + 2] = (code *)coin_rolling;
        a->handlers[3 * 5 + 3] = (code *)coin_collect;
        a->handlers[3 * 5 + 4] = (code *)coin_cmd4;
        a->handlers[4 * 5 + 1] = (code *)erase;
        a->handlers[4 * 5 + 2] = (code *)coin_gone;
        a->walk_steps[0] = (code *)coin_roll;
        a->steps_b[0] = (code *)coin_sparkle_1;
        a->steps_b[1] = (code *)coin_sparkle_2;
        a->steps_b[2] = (code *)coin_vanish;
        a->steps_b[3] = (code *)coin_drop_1;
        a->steps_b[4] = (code *)coin_drop_2;
        a->steps_b[5] = (code *)coin_reset_step;
    }
    if (a->state < 0 || a->state > 4 || a->cmd < 0 || a->cmd > 4) return 0;
    return ((int (*)(actor_t *, gds *))a->handlers[a->state * 5 + a->cmd])(a, g);
}
