/* obj_sparks.c - "sparks" (Roy's hotel): an electric spark that flashes in front of a
 * random closed door (L2_s04_sub.o, functions m_003f2 .. m_0078a). It has no box, so it
 * can't hurt Mario; the door it picked (gds->unk_81c) is the one puzzle_2 watches.
 */
#include "obj_common.h"

static int sparks_init(actor_t *a, gds *g) {            /* m_003f2 */
    (void)g;
    a->unk_02a = 0; a->unk_02c = 2; a->step = 0;
    return 0;
}

static int sparks_place(actor_t *a, gds *g) {           /* m_004aa: pick a closed door */
    uint32_t r = (uint32_t)RND2(g);
    a->floor = (int2)smod((int)(r & 0xffff), 5);
    a->col = (int2)smod((int)((r >> 15) & 1), 7);
    while ((CELL(g, a->floor, a->col) & 0xf000) != 0x1000) {
        if (++a->col > 6) {
            a->col = 0;
            if (++a->floor > 4) a->floor = 0;
        }
    }
    hm_world->puzzle_cell = &CELL(g, a->floor, a->col);   /* g->unk_81c */
    a->y = a->drawn_y0 = a->drawn_y1 = (int2)(g->floor_y[a->floor] - 0x1d);
    a->x = a->drawn_x0 = a->drawn_x1 = (int2)(g->door_x[a->col] - 8);
    a->state = 3;
    return 0;
}

static int sparks_step(actor_t *a, gds *g) {            /* m_006c0 */
    a->frame = (int2)(a->unk_02a + 1);
    if (++a->unk_02a == 5) {
        a->unk_02a = 0;
        if (--a->unk_02c == 0) { a->state = 4; a->step = 3; a->unk_02c = 1; }
    } else if (a->unk_02a == 1) {
        g->unk_054 = 0xc;                                  /* the zap */
    }
    return 0;
}

static int sparks_on(actor_t *a, gds *g) {              /* m_005e4 */
    int r = CALL(a->walk_steps[a->step], a, g);
    draw_at_xy(a, g, a->frame);
    return r;
}

static int sparks_off(actor_t *a, gds *g) {             /* m_0078a */
    (void)g;
    if (a->unk_02c-- < 1) a->state = 0;
    return 0;
}

static int sparks_steps_b(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }   /* m_00750 */

int sparks(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, sparks_init);
        H(0, 2, sparks_init);           /* m_00478: the same reset */
        H(1, 2, sparks_place);
        H(3, 1, obj_erase);
        H(3, 2, sparks_on);
        H(4, 1, obj_erase);
        H(4, 2, sparks_steps_b);
        W(0, sparks_step);
        B(3, sparks_off);
    }
    return obj_dispatch(a, g);
}
