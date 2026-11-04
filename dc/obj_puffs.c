/* obj_puffs.c - "puffs" (L4_s03_sub.o, m_00412 .. m_00d96): three puffs of smoke that
 * rise from three different random doors (open or closed, never the one Mario is hiding
 * in). It sits idle in state 0 until something outside sets state 1 (the hotel's puzzle);
 * the third door it picks is stored in gds->unk_81c (hm_world->puzzle_cell).
 * Puff 1 at (x_min, x_max), puff 2 at unk_25c+8/+0xc (int32), puff 3 at (x, y).
 * Puffs 1 and 2 use frames 1..10 (unk_02a), puff 3 frames 11..20 (frame = unk_02a+10);
 * all rise 1 pixel a tick, the picture changes every 3rd tick, gone after 10 pictures.
 * No box: it can't touch Mario.
 */
#include "obj_common.h"

static int pf_init(actor_t *a, gds *g) {                /* m_00412 / m_0056a */
    (void)g;
    a->unk_02a = 1; a->unk_02c = 0; a->step = 0;
    return 0;
}

/* floor = |rnd| % 5, column = |rnd >> 15| % 7 (both rolls from the same unk_82a step) */
static void roll_door(actor_t *a, gds *g) {
    int32_t r = (int32_t)RND2(g);
    a->floor = (int2)smod(abs(r) & 0xffff, 5);
    a->col = (int2)smod(abs((int32_t)g->unk_82a >> 15) & 0xffff, 7);
}
static void next_door(actor_t *a) {
    if (++a->col > 6) { a->col = 0; if (++a->floor > 4) a->floor = 0; }
}
static int is_door(uint16_t v) { v &= 0xf000; return v == 0x1000 || v == 0x2000; }

static int pf_place(actor_t *a, gds *g) {               /* m_0059c */
    mario_t *m = g->mario;
    uint16_t *c1, *c2, *c3;
    int hiding = m->state_group == 4;
    roll_door(a, g);
    c1 = &CELL(g, a->floor, a->col);
    while ((*c1 & 0xf000) != 0x1000 && ((*c1 & 0xf000) != 0x2000 || (hiding && c1 == m->door))) {
        next_door(a); c1 = &CELL(g, a->floor, a->col);
    }
    a->x_min = g->door_x[a->col] + 0x18;
    a->x_max = g->floor_y[a->floor] - 10;
    roll_door(a, g);
    c2 = &CELL(g, a->floor, a->col);
    while (!is_door(*c2) || c2 == c1 || (hiding && c2 == m->door)) {
        next_door(a); c2 = &CELL(g, a->floor, a->col);
    }
    wr25c(a, 8, g->door_x[a->col] + 0x18);
    wr25c(a, 0xc, g->floor_y[a->floor] - 10);
    roll_door(a, g);
    c3 = &CELL(g, a->floor, a->col);
    while (!is_door(*c3) || c3 == c1 || c3 == c2 || (hiding && c3 == m->door)) {
        next_door(a); c3 = &CELL(g, a->floor, a->col);
    }
    hm_world->puzzle_cell = c3;                         /* g->unk_81c */
    a->x = (int2)(g->door_x[a->col] + 0x18);
    a->y = (int2)(g->floor_y[a->floor] - 10);
    a->state = 3;
    return 0;
}

static int pf_moving(actor_t *a, gds *g) {              /* m_009fe */
    int r = CALL(a->walk_steps[a->step], a, g);
    obj_draw(a, a->frame, a->x, a->y);
    obj_draw(a, a->unk_02a, (int)a->x_min, (int)a->x_max);
    obj_draw(a, a->unk_02a, rd25c(a, 8), rd25c(a, 0xc));
    if (g->draw_field == 1) a->drawn_y0 = a->y; else a->drawn_y1 = a->y;
    return r;
}

static int pf_rise(actor_t *a, gds *g) {                /* m_00b8e */
    (void)g;
    if (++a->unk_02c == 3) { a->unk_02c = 0; a->unk_02a++; }
    a->frame = (int2)(a->unk_02a + 10);
    a->y--;
    a->x_max--;
    wr25c(a, 0xc, rd25c(a, 0xc) - 1);
    if (a->unk_02a == 10) { a->state = 4; a->step = 3; a->unk_02c = 1; }
    return 0;
}

static int pf_dying(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }   /* m_00c0a */

static int pf_gone(actor_t *a, gds *g) {                /* m_00c44 */
    (void)g;
    if (a->unk_02c-- < 1) a->state = 0;
    return 0;
}

int puffs(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, pf_init);
        H(0, 2, pf_init);
        H(1, 2, pf_place);
        H(3, 1, obj_erase);
        H(3, 2, pf_moving);
        H(4, 1, obj_erase);
        H(4, 2, pf_dying);
        W(0, pf_rise);
        B(3, pf_gone);
    }
    return obj_dispatch(a, g);
}
