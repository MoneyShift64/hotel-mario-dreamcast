/* obj_larry_bb.c - "larry_bb", the balls Larry throws (hotel 3 boss), from L3_s10_sub.o
 * (m_0f89a .. m_0fbfa). It waits in state 0 until larry's throw (m_01248) puts it in state 3
 * at his hand with a speed of 10 or 12 in x_min; it rolls straight along until it leaves
 * 0x22..0x154. Stomping it hurts Mario (cmd 3 returns 1); a fireball or star breaks it for
 * 10 points. Either way it waits 40 ticks (state 4) before Larry can throw it again.
 * Frames 1-3: rolling. */
#include "obj_common.h"

static int bb_reset(actor_t *a, gds *g) { (void)g; a->unk_02a = 0; return 0; }   /* m_0f89a / m_0f8fa */

static int bb_moving(actor_t *a, gds *g) {              /* m_0f922 */
    int r = CALL(a->walk_steps[a->step], a, g);
    draw_at_xy(a, g, a->frame);
    return r;
}

static int bb_roll(actor_t *a, gds *g) {                /* m_0f9fe */
    (void)g;
    a->frame = (int2)(a->unk_02a + 1);
    if (++a->unk_02a > 2) a->unk_02a = 0;
    if (a->dir == 1) a->x = (int2)(a->x - a->x_min);
    else             a->x = (int2)(a->x + a->x_min);
    a->box_x0 = a->x; a->box_x1 = a->x + 9;
    a->box_y0 = a->y; a->box_y1 = a->y + 9;
    if (a->x < 0x22 || a->x > 0x154) { a->state = 4; a->step = 3; a->unk_02a = 0x28; }
    return 0;
}

static int bb_hurts(actor_t *a, gds *g) { (void)a; (void)g; return 1; }   /* m_0fac8 */

static int bb_destroy(actor_t *a, gds *g) {             /* m_0faec */
    if (g->unk_496 == 0) {
        if (a->unk_04c == 0 && g->unk_53a == 0) SCORE(g, 10);
        else a->unk_04c = 0;
        a->state = 4; a->step = 3; a->unk_02a = 0x28;
    } else a->unk_04e = a->cmd;
    return 0;
}

static int bb_dying(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }

static int bb_wait(actor_t *a, gds *g) {                /* m_0fbba */
    (void)g;
    if (a->unk_02a-- < 2) {
        a->state = 0; a->unk_04e = 0;
        a->box_x0 = a->box_x1 = a->box_y0 = a->box_y1 = 0;
    }
    return 0;
}

int larry_bb(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, bb_reset);
        H(0, 2, bb_reset);
        H(3, 1, obj_erase);
        H(3, 2, bb_moving);
        H(3, 3, bb_hurts);
        H(3, 4, bb_destroy);
        H(4, 1, obj_erase);
        H(4, 2, bb_dying);
        W(0, bb_roll);
        B(3, bb_wait);
    }
    return obj_dispatch(a, g);
}
