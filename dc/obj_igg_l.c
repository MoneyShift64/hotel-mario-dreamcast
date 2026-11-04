/* obj_igg_l.c - "igg_l" (type 0x3f): one link of the boss's chain in the last stage
 * (hotel 8 stage 15), from L8_s15_sub.o (m_11162 .. m_115c6).
 * The boss puts the first link at the top of the screen above Mario (state 3); each
 * link then hangs the next one 0x24 lower, until the chain reaches the bottom floor.
 * Touching any link hurts Mario (cmd 3 returns 1, cmd 4 returns 2: it can't be killed).
 * When the boss pulls the chain back, the first link goes to state 4 and each link
 * passes that on to the next a tick later; then they vanish.
 * x_min = 1 once a link is taken; unk_25c+0 = the next link (actor index + 1).
 * Frames 1-3 (swinging).
 */
#include "obj_common.h"

static actor_t *next_link(actor_t *a, gds *g) {
    int32_t i = rd25c(a, 0);
    return (i > 0 && i <= 24) ? &g->room->actors[i - 1] : NULL;
}

static int il_init(actor_t *a, gds *g) {                /* m_11162: claim a free link */
    int i;
    a->x_min = 1;
    for (i = 0; i < 24 && g->room->actors[i].type != 0; i++) {
        actor_t *p = &g->room->actors[i];
        if (p->type == 0x3f && p->x_min == 0) {
            wr25c(a, 0, i + 1);
            p->x_min = 1;
            return 0;
        }
    }
    return 0;
}

static int il_moving(actor_t *a, gds *g) {              /* m_1128a */
    CALL(a->walk_steps[a->step], a, g);
    draw_at_xy(a, g, a->frame);
    return 0;
}

static int il_hurts(actor_t *a, gds *g) { (void)a; (void)g; return 1; }    /* m_11484 */
static int il_tough(actor_t *a, gds *g) { (void)a; (void)g; return 2; }    /* m_114a8 */

static int il_swing(actor_t *a) {
    a->frame = (int2)(a->unk_02a + 1);
    if (++a->unk_02a > 2) a->unk_02a = 0;
    return a->unk_02a;
}

static int il_hang(actor_t *a, gds *g) {                /* m_11366: hang the next link */
    int n = il_swing(a);
    a->cur_floor = g->mario->floor;
    if (n < 2) {
        a->box_x0 = a->x + 4; a->box_x1 = a->x + 10;
        a->box_y0 = a->y; a->box_y1 = a->y + 0x24;
    } else {
        actor_t *nx = next_link(a, g);
        if (nx && a->box_y1 < 0xde) {
            nx->x = a->x; nx->y = (int2)(a->y + 0x24);
            nx->state = 3; nx->step = 0; nx->unk_02a = 0;
        }
        a->step = 1;
    }
    return 0;
}

static int il_hanging(actor_t *a, gds *g) {             /* m_11440 */
    il_swing(a);
    a->cur_floor = g->mario->floor;
    return 0;
}

static int il_dying(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }   /* m_114cc */

static int il_pull(actor_t *a, gds *g) {                /* m_11506: pass it down the chain */
    if (a->unk_02a > 0) {
        actor_t *nx = next_link(a, g);
        if (nx && nx->state == 3) { nx->state = 4; nx->step = 0; nx->unk_02a = 0; }
    }
    if (++a->unk_02a > 1) { a->step = 3; a->unk_02a = 1; }
    return 0;
}

static int il_gone(actor_t *a, gds *g) {                /* m_11582 */
    (void)g;
    if (a->unk_02a-- < 2) {
        a->box_x0 = a->box_x1 = a->box_y0 = a->box_y1 = 0;
        a->state = 0; a->step = 0; a->unk_02a = 0;
    }
    return 0;
}

int igg_l(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, il_init);
        H(3, 1, obj_erase);
        H(3, 2, il_moving);
        H(3, 3, il_hurts);
        H(3, 4, il_tough);
        H(4, 1, obj_erase);
        H(4, 2, il_dying);
        W(0, il_hang);
        W(1, il_hanging);
        B(0, il_pull);
        B(3, il_gone);
    }
    return obj_dispatch(a, g);
}
