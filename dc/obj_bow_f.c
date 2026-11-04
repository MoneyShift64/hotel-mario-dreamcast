/* Bowser's residual flame (L8_s15_subT.o, bow_f/m_00402..m_007c6). */
#include "obj_common.h"
static int init(actor_t *a, gds *g) {
    (void)a;
    (void)g;
    return 0;
}
static int reset(actor_t *a, gds *g) {
    (void)g;
    a->unk_02a = a->unk_02c = 0;
    return 0;
}
static int update(actor_t *a, gds *g) {
    a->frame = a->unk_02a + 1;
    if (++a->unk_02a > 2)
        a->unk_02a = 0;
    a->cur_floor = g->mario->floor;
    a->box_x0 = a->x + 2;
    a->box_x1 = a->x + 14;
    a->box_y0 = a->y + 4;
    a->box_y1 = a->y + 16;
    if (++a->unk_02c > 20) {
        a->state = 4;
        a->unk_02a = 2;
    }
    draw_at_xy(a, g, a->frame);
    return 0;
}
static int hit(actor_t *a, gds *g) {
    (void)a;
    (void)g;
    return 1;
}
static int kill(actor_t *a, gds *g) {
    (void)a;
    (void)g;
    return 2;
}
static int fade(actor_t *a, gds *g) {
    (void)g;
    if (a->unk_02a-- < 2) {
        a->box_x0 = a->box_x1 = a->box_y0 = a->box_y1 = 0;
        a->state = 0;
        a->unk_02a = a->unk_02c = 0;
    }
    return 0;
}
int bow_f(actor_t *a, gds *g) {
    if (!a->state && !a->cmd) {
        obj_tables(a);
        H(0, 0, init);
        H(0, 2, reset);
        H(3, 2, update);
        H(3, 3, hit);
        H(3, 4, kill);
        H(4, 2, fade);
    }
    return obj_dispatch(a, g);
}
