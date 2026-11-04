/* obj_power.c - the "power" object: the item that comes out of a door, ported from the
 * stage modules (source: L1_s06_sub.o, entry "power", functions m_0daa2 .. m_0e69c).
 *
 * After its delay it waits for its door to be open, then comes out as whatever Mario
 * needs next: a mushroom (type 7) when he is small, a fire flower (type 3) when he is
 * big, a 1-up (type 2) when he already has fire. It slides along the floor and Mario
 * collects it in mario_coll_det (cmd 3); off the edge it drops out or vanishes.
 * Frames: 1-6 mushroom/1-up right, 7-12 left, 14-19 fire flower (x_min = 13 is the
 * flower's frame base), +5 is the falling picture.
 */
#include <stdlib.h>
#include <string.h>
#include "world.h"

#define CELL(g, f, c) (((uint16_t *)((g)->stage_info + 0x12))[(f) * COLS + (c)])
#define CALL(fn, a, g) ((int (*)(actor_t *, gds *))(fn))((a), (g))

static int nop(actor_t *a, gds *g) { (void)a; (void)g; return 0; }

static int32_t rd25c(const actor_t *a) { int32_t v; memcpy(&v, a->unk_25c, 4); return v; }
static void wr25c(actor_t *a, int32_t v) { memcpy(a->unk_25c, &v, 4); }

static int drops_out(int hotel, int stage) {           /* m_0013c */
    switch (hotel) {
    case 0: return !(stage == 3 || stage == 6 || stage == 9);
    case 1: case 4: case 7: return 1;
    }
    return 0;
}

static void draw_at(actor_t *a, gds *g, int frame) {
    obj_draw(a, frame, a->x, a->y);
    if (g->draw_field == 1) a->drawn_x0 = a->x; else a->drawn_x1 = a->x;
}

/* m_0daa2: state 0, cmd 0 - set up */
static int power_init(actor_t *a, gds *g) {
    if (g->item_hit < 10) {
        if (a->appear_count == 0) a->appear_count = 1;
        a->timer = 0;
        if (a->delay != 0)                  /* delay: the time left (stage_info+8 units) when it comes */
            a->delay = (int2)(6 * (*(int32_t *)(g->stage_info + 8) - a->delay));
    } else {                                /* (an enemy turned into an item) */
        int2 v;
        a->floor = a->cur_floor;
        v = (int2)(g->floor_y[a->floor] - 0x1d);
        a->y = a->drawn_y0 = a->drawn_y1 = v;
        a->box_y0 = a->y + 6;
        a->box_y1 = a->y + 0x1d;
        a->x_min = 0;
        a->drawn_x1 = a->drawn_x0 = a->x;
        a->box_x0 = a->x + 4;
        a->box_x1 = a->x + 0x10;
        a->dir = (int2)(0xb5 < a->x);
        a->state = 3;
        a->step = 0;
    }
    return 0;
}

/* m_0dcc8: state 0, cmd 2 - wait out the delay */
static int power_wait(actor_t *a, gds *g) {
    (void)g;
    if (a->delay < ++a->timer) {
        a->timer = 0;
        if (a->appear_count-- > 0) {
            a->state = 1;
            a->unk_04e = 0;
        }
    }
    return 0;
}

/* m_0dd22: state 1, cmd 2 - come out when the door is open */
static int power_wait_door(actor_t *a, gds *g) {
    if ((CELL(g, a->floor, a->col) & 0xf000) != 0x2000) return 0;
    if (a->dir == -1) a->dir = (g->mario->state & 1) == 0 ? 1 : 0;
    a->x = (int2)(a->dir == 0 ? a->col * 0x2c + 0x5a : a->col * 0x2c + 10);
    switch (g->player[g->cur_player].power) {
    case 0:                                 /* mushroom */
        a->type = 7; a->unk_036 = 2;
        a->y = (int2)(g->floor_y[a->floor] - 0x1d);
        a->box_y0 = a->y + 6; a->box_y1 = a->y + 0x1d;
        a->box_x0 = a->x + 4; a->box_x1 = a->x + 0x10;
        a->x_min = 0;
        break;
    case 1:                                 /* fire flower */
        a->type = 3; a->unk_036 = 4;
        a->y = (int2)(g->floor_y[a->floor] - 0x1b);
        a->box_y0 = a->y + 2; a->box_y1 = a->y + 0x1b;
        a->box_x0 = a->x + 4; a->box_x1 = a->x + 0x10;
        a->x_min = 0xd;
        break;
    case 2:                                 /* 1-up */
        a->type = 2; a->unk_036 = 6;
        a->y = (int2)(g->floor_y[a->floor] - 0x1d);
        a->box_y0 = a->y + 6; a->box_y1 = a->y + 0x1d;
        a->box_x0 = a->x + 4; a->box_x1 = a->x + 0x10;
        a->x_min = 0;
        break;
    }
    a->drawn_x1 = a->drawn_x0 = a->x;
    a->drawn_y1 = a->drawn_y0 = a->y;
    a->cur_floor = a->floor;
    a->state = 3;
    a->step = 0;
    a->unk_02a = 0;
    return 0;
}

/* m_0df72: state 3, cmd 2 - walk_steps[step], then draw */
static int power_moving(actor_t *a, gds *g) {
    int r = CALL(a->walk_steps[a->step], a, g);
    draw_at(a, g, (int)a->x_min + a->frame);
    return r;
}

static void off_edge(actor_t *a, gds *g) {
    a->state = 4;
    if (!drops_out(g->player[g->cur_player].hotel, g->player[g->cur_player].stage)) {
        a->step = 0;
        a->unk_02a = 3;
    } else {
        a->step = 3;
        a->unk_02c = 1;
    }
}

/* m_0e04a: slide along the floor */
static int power_slide(actor_t *a, gds *g) {
    if (a->dir == 1) {
        a->x -= a->unk_036;
        a->frame = (int2)(a->type == 3 ? a->unk_02a + 1 : a->unk_02a + 7);
    } else {
        a->x += a->unk_036;
        a->frame = (int2)(a->unk_02a + 1);
    }
    if (a->unk_02a == 0) {                  /* (start of a 6-picture cycle: the hop) */
        a->x_max = 0x1c;
        wr25c(a, 0x10);
    }
    if (++a->unk_02a > 5) a->unk_02a = 0;
    if (a->x > 0x15a) off_edge(a, g);
    if (a->x < 0x12) off_edge(a, g);
    a->box_x0 = a->x + 4;
    a->box_x1 = a->x + 0x10;
    if (a->type != 3) {                     /* the mushroom and 1-up hop: the box follows */
        a->x_max -= 7;
        a->box_y0 = (int2)(labs((long)a->x_max) + a->y);
        wr25c(a, rd25c(a) - 4);
        a->box_y1 = (int2)(labs((long)rd25c(a)) + a->y + 0x11);
    }
    return 0;
}

/* m_0e27a: state 3, cmd 3 - collected */
static int power_collected(actor_t *a, gds *g) {
    if (g->unk_496 == 0) {
        switch (a->type) {
        case 2: g->player[g->cur_player].score += 750; break;
        case 3: g->player[g->cur_player].score += 500; break;
        case 7: g->player[g->cur_player].score += 250; break;
        }
        a->state = 4;
        a->step = 0;
        a->unk_02a = 3;
    } else a->unk_04e = a->cmd;
    return 0;
}

/* m_0e370: state 3, cmd 4 - destroyed */
static int power_destroy(actor_t *a, gds *g) {
    if (g->unk_496 == 0) {
        a->state = 4;
        a->step = 3;
        a->unk_02c = 0;
        a->unk_02a = 0;
    } else a->unk_04e = a->cmd;
    return 0;
}

/* m_0e3c8: state 4, cmd 2 - steps_b[step] */
static int power_gone(actor_t *a, gds *g) {
    CALL(a->steps_b[a->step], a, g);
    return 0;
}

static int power_reset_after(actor_t *a, gds *g) {     /* m_0e402 */
    (void)g;
    if (a->unk_02a-- < 2) { a->state = 0; a->timer = 0; a->dir = -1; }
    return 0;
}

static int power_tip(actor_t *a, gds *g) {             /* m_0e436 */
    draw_at(a, g, (int)a->x_min + 5);
    if (++a->unk_02c == 2) a->step = 4;
    return 0;
}

static int power_fall(actor_t *a, gds *g) {            /* m_0e4fa: drops off the screen */
    a->y += 0x10;
    if (a->y < 0xd5) {
        obj_draw(a, (int)a->x_min + 5, a->x, a->y);
        if (g->draw_field == 1) a->drawn_y0 = a->y; else a->drawn_y1 = a->y;
    } else a->step = 5;
    return 0;
}

static int power_reset(actor_t *a, gds *g) {           /* m_0e5ca */
    (void)g;
    a->unk_02c = 0;
    a->unk_02a = 0;
    a->state = 0;
    a->timer = 0;
    a->dir = -1;
    return 0;
}

static int erase(actor_t *a, gds *g) { (void)a; (void)g; return 0; }   /* m_0e5f6 */

int power(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        int i;
        for (i = 0; i < 25; i++) a->handlers[i] = (code *)nop;
        for (i = 0; i < 24; i++) a->walk_steps[i] = (code *)nop;
        for (i = 0; i < 16; i++) a->steps_b[i] = (code *)nop;
        a->handlers[0] = (code *)power_init;
        a->handlers[2] = (code *)power_wait;
        a->handlers[7] = (code *)power_wait_door;
        a->handlers[0x10] = (code *)erase;
        a->handlers[0x11] = (code *)power_moving;
        a->handlers[0x12] = (code *)power_collected;
        a->handlers[0x13] = (code *)power_destroy;
        a->handlers[0x15] = (code *)erase;
        a->handlers[0x16] = (code *)power_gone;
        a->walk_steps[0] = (code *)power_slide;
        a->steps_b[0] = (code *)power_reset_after;
        a->steps_b[3] = (code *)power_tip;
        a->steps_b[4] = (code *)power_fall;
        a->steps_b[5] = (code *)power_reset;
    }
    if (a->state < 0 || a->state > 4 || a->cmd < 0 || a->cmd > 4) return 0;
    return CALL(a->handlers[a->state * 5 + a->cmd], a, g);
}
