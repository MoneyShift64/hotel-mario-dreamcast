/* obj_lg_f_ball.c - "lg_f_ball": Bowser's big fireball (L8_s04_sub.o m_0f4c6 .. m_0fc3a).
 * It sits in state 1 doing nothing until something else (the Bowser fight) sets it to
 * state 2. Then it starts at the screen edge away from Mario (x 0x22 or 0x14c) on its
 * floor, plays sound 0xf, flares up harmlessly (step 1, 5 frames, no box) and flies
 * across at 14 pixels a tick. Touching Mario hurts him (cmd 3 returns 1). A fireball
 * or star (cmd 4) puts it out, setting the bonus flag (unk_81a) unless the level is
 * over or Mario has just picked up a special item (item_hit >= 10).
 * Frames: 1-3 flying right, 4-6 flying left, 7-11 flare right, 12-16 flare left,
 * 10-11 going out.
 */
#include "obj_common.h"

static int lf_init(actor_t *a, gds *g) { (void)g; a->state = 1; return 0; }          /* m_0f4c6 */

static int lf_again(actor_t *a, gds *g) {               /* m_0f5de: state 0 cmd 2 */
    if (g->unk_53a == 0) a->state = 1;
    return 0;
}

static int lf_launch(actor_t *a, gds *g) {              /* m_0f648: state 2 */
    a->dir = (g->mario->x < 0xc1) ? 1 : 0;
    a->x = (a->dir == 0) ? 0x22 : 0x14c;
    a->y = a->drawn_y0 = a->drawn_y1 = (int2)(g->floor_y[a->floor] - 0x21);
    a->cur_floor = a->floor;
    a->box_y0 = a->y + 3; a->box_y1 = a->y + 10;
    a->state = 3; a->step = 1;
    a->unk_02a = 0; a->unk_02c = 2;
    a->box_x0 = 0; a->box_x1 = 0;      /* no box while it flares up */
    g->unk_054 = 0xf;
    return 0;
}

static int lf_moving(actor_t *a, gds *g) {              /* m_0f730 */
    int r = CALL(a->walk_steps[a->step], a, g);
    draw_at(a, g, a->frame);
    return r;
}

static int lf_fly(actor_t *a, gds *g) {                 /* m_0f884 */
    (void)g;
    if (--a->unk_02c < 1) {
        a->unk_02c = 1;
        if (++a->unk_02a > 2) a->unk_02a = 0;
    }
    if (a->dir == 1) { a->x -= 0xe; a->frame = (int2)(a->unk_02a + 4); }
    else             { a->x += 0xe; a->frame = (int2)(a->unk_02a + 1); }
    if (a->x > 0x14c) { a->state = 4; a->step = 0; a->unk_02a = 3; }
    if (a->x < 0x22)  { a->state = 4; a->step = 0; a->unk_02a = 3; }
    a->box_x0 = a->x + 3; a->box_x1 = a->x + 0xf;
    return 0;
}

static int lf_flare(actor_t *a, gds *g) {               /* m_0f800 */
    (void)g;
    if (--a->unk_02c < 1) {
        a->unk_02c = 2;
        if (++a->unk_02a > 4) { a->unk_02a = 0; a->step = 0; a->unk_02c = 1; }
    }
    a->frame = (int2)((a->dir == 1) ? a->unk_02a + 0xc : a->unk_02a + 7);
    return 0;
}

static int lf_touch(actor_t *a, gds *g) { (void)a; (void)g; return 1; }     /* m_0f962: hurts */

static int lf_destroy(actor_t *a, gds *g) {             /* m_0f986: cmd 4 */
    if (g->unk_496 == 0) {
        if (g->unk_53a == 0 && g->item_hit < 10) g->unk_81a = 1;
        a->state = 4; a->step = 3; a->unk_02c = 0; a->unk_02a = 0;
    } else a->unk_04e = a->cmd;
    return 0;
}

static int lf_dying(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }   /* m_0fa06 */

static int lf_gone(actor_t *a, gds *g) {                /* m_0fa40 */
    (void)g;
    if (a->unk_02a-- < 2) { a->state = 0; a->timer = 0; a->dir = -1; a->unk_04e = 0; }
    return 0;
}

static int lf_out(actor_t *a, gds *g) {                 /* m_0fa78 */
    draw_at(a, g, a->unk_02a + 10);
    if (++a->unk_02c > 1 && ++a->unk_02a > 1) { a->step = 5; a->unk_02a = 2; }
    return 0;
}

static int lf_reset(actor_t *a, gds *g) {               /* m_0fb60 */
    (void)g;
    if (a->unk_02a-- < 2) {
        a->unk_02c = 0; a->unk_02a = 0; a->state = 0; a->timer = 0; a->dir = -1; a->unk_04e = 0;
    }
    return 0;
}

int lg_f_ball(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, lf_init);
        H(0, 2, lf_again);
        H(1, 2, obj_nop);              /* m_0f624: waits to be launched */
        H(2, 2, lf_launch);
        H(3, 1, obj_erase);
        H(3, 2, lf_moving);
        H(3, 3, lf_touch);
        H(3, 4, lf_destroy);
        H(4, 1, obj_erase);
        H(4, 2, lf_dying);
        W(0, lf_fly);
        W(1, lf_flare);
        B(0, lf_gone);
        B(3, lf_out);
        B(5, lf_reset);
    }
    return obj_dispatch(a, g);
}
