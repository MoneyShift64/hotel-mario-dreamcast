/* obj_common.h - shared helpers for the objects ported from the stage modules.
 *
 * Each stage module starts with the same little routines; the objects call them:
 *   rnd (m_0008c...)        RND(g) on gds->unk_826, RND2(g) on unk_82a
 *   door cell (m_000a2...)  DOOR_CELL(floor, x): the stage_info cell at x, or NULL
 *   m_0013c / m_00110       drops_out(hotel, stage): leave the screen by falling?
 *   compiler helpers        abs, 32-bit multiply, divide, modulo (plain C here)
 * Drawing a compiled sprite, frames[n](draw_a0/a1 + x + y*384), is draw_at(); frames[0]
 * is the object's eraser and is never drawn.
 */
#ifndef HM_OBJ_COMMON_H
#define HM_OBJ_COMMON_H
#include <stdlib.h>
#include <string.h>
#include "world.h"

#define CELL(g, f, c) (((uint16_t *)((g)->stage_info + 0x12))[(f) * COLS + (c)])
#define RND(g)  ((g)->unk_826 = (int4)hm_rnd((uint32_t *)&(g)->unk_826))
#define RND2(g) ((g)->unk_82a = (int4)hm_rnd((uint32_t *)&(g)->unk_82a))
#define CALL(fn, a, g) ((int (*)(actor_t *, gds *))(fn))((a), (g))
#define DOOR_CELL(floor, x) hm_door_cell_at(&hm_world->doors, (floor), (x))
#define HOTEL(g) ((g)->player[(g)->cur_player].hotel)
#define STAGE(g) ((g)->player[(g)->cur_player].stage)
#define SCORE(g, n) ((g)->player[(g)->cur_player].score += (n))
#define MARIO(g) ((g)->mario)

static inline int obj_nop(actor_t *a, gds *g) { (void)a; (void)g; return 0; }
static inline int obj_erase(actor_t *a, gds *g) { (void)a; (void)g; return 0; }

/* the handler tables, all "do nothing" to start with (the entry points' loops) */
static inline void obj_tables(actor_t *a) {
    int i;
    for (i = 0; i < 25; i++) a->handlers[i] = (code *)obj_nop;
    for (i = 0; i < 24; i++) a->walk_steps[i] = (code *)obj_nop;
    for (i = 0; i < 16; i++) a->steps_b[i] = (code *)obj_nop;
}
#define H(st, cmd, fn) (a->handlers[(st) * 5 + (cmd)] = (code *)(fn))
#define W(n, fn) (a->walk_steps[n] = (code *)(fn))
#define B(n, fn) (a->steps_b[n] = (code *)(fn))

static inline int obj_dispatch(actor_t *a, gds *g) {
    if (a->state < 0 || a->state > 4 || a->cmd < 0 || a->cmd > 4) return 0;
    return CALL(a->handlers[a->state * 5 + a->cmd], a, g);
}

/* draw frames[frame] at (x, y) and remember where (drawn_x0 / x1 by draw_field) */
static inline void draw_at(actor_t *a, gds *g, int frame) {
    obj_draw(a, frame, a->x, a->y);
    if (g->draw_field == 1) a->drawn_x0 = a->x; else a->drawn_x1 = a->x;
}
static inline void draw_at_xy(actor_t *a, gds *g, int frame) {   /* (both x and y kept) */
    obj_draw(a, frame, a->x, a->y);
    if (g->draw_field == 1) { a->drawn_x0 = a->x; a->drawn_y0 = a->y; }
    else { a->drawn_x1 = a->x; a->drawn_y1 = a->y; }
}
static inline void draw_at_y(actor_t *a, gds *g, int frame) {    /* (y kept) */
    obj_draw(a, frame, a->x, a->y);
    if (g->draw_field == 1) a->drawn_y0 = a->y; else a->drawn_y1 = a->y;
}
static inline void draw_plain(actor_t *a, int frame) { obj_draw(a, frame, a->x, a->y); }

static inline int drops_out(int hotel, int stage) {
    switch (hotel) {
    case 0: return !(stage == 3 || stage == 6 || stage == 9);
    case 1: case 4: case 7: return 1;
    }
    return 0;
}

/* the 68000 runtime's signed modulo (result has the dividend's sign) */
static inline int smod(int a, int b) { return b ? a % b : 0; }

static inline int32_t rd25c(const actor_t *a, int off) { int32_t v; memcpy(&v, a->unk_25c + off, 4); return v; }
static inline void wr25c(actor_t *a, int off, int32_t v) { memcpy(a->unk_25c + off, &v, 4); }

/* The walkers' "follow Mario" rule (goomba, koopa, beetles ...): at the start of a walk
 * cycle, on Mario's floor (not the top one), look for an object of the same speed on
 * the floor above, near this one on Mario's side. If one walks the same way and none
 * the other way, turn round; if both, turn only if Mario is behind. Returns 1 to turn;
 * *same is set when a same-way walker was found. */
static inline int walker_follow(actor_t *a, gds *g, int x, int *same) {
    mario_t *m = g->mario;
    int above = a->cur_floor - 1, i, same_dir = 0, other_dir = 0;
    actor_t *p;
    if (same) *same = 0;
    if (a->cur_floor < 1 || m->floor != a->cur_floor) return 0;
    for (i = 0, p = g->room->actors; i < 24 && p->type != 0; i++, p++) {
        int near;
        if (p->state != 3 || p->cur_floor != above || p->speed != a->speed) continue;
        near = (m->x < x) ? (x - 0x3c < p->x && p->x < x + 8) : (p->x < x + 0x3c && x - 8 < p->x);
        if (!near) continue;
        if (p->dir == a->dir) same_dir = 1; else other_dir = 1;
    }
    if (same) *same = same_dir;
    if (!same_dir) return 0;
    if (!other_dir) return 1;
    return (a->dir == 1 && m->x < x) || (a->dir == 0 && x < m->x);
}

/* the 0..4..0 walk cycle in unk_02a (unk_02e: going down) */
static inline void walk_cycle(actor_t *a) {
    if (a->unk_02e == 0) {
        if (a->unk_02a == 4) { a->unk_02a--; a->unk_02e = 1; } else a->unk_02a++;
    } else if (a->unk_02a == 0) { a->unk_02a++; a->unk_02e = 0; }
    else a->unk_02a--;
}

/* passing a closed door: a 1 in (mask+1) chance of opening it (worth 50 when shut) */
static inline void maybe_open_door(actor_t *a, gds *g, int x, uint32_t mask) {
    uint16_t *door = DOOR_CELL(a->cur_floor, x);
    if (door && (*door & 0xf000) == 0x1000 && (RND(g) & mask) == 1) {
        *door = (*door & 0xfff) | 0x2009;
        g->unk_054 = 2;
    }
}

/* the usual "come out of the door" check: open door, Mario on this floor > 72 away */
static inline int door_far_from_mario(actor_t *a, gds *g, int ax, int mx_ofs) {
    int mx = g->mario->x + mx_ofs, dist;
    if (ax < mx) { if (a->dir == -1) a->dir = 0; dist = mx - ax; }
    else         { if (a->dir == -1) a->dir = 1; dist = ax - mx; }
    return dist;
}

/* walls of a floor: further in where a lift stands at either end */
static inline void floor_limits(actor_t *a, gds *g, int lo_lift, int lo, int hi_lift, int hi) {
    uint16_t l0 = CELL(g, a->floor, 0) & 0xf000, l6 = CELL(g, a->floor, 6) & 0xf000;
    a->x_min = (l0 == 0xf000 || l0 == 0xe000) ? lo_lift : lo;
    a->x_max = (l6 == 0xf000 || l6 == 0xe000) ? hi_lift : hi;
}

#endif
