/* obj_w_bon_bon.c - "w_bon_bon" (type 0x24), Wendy's bon-bon in the hotel 7 boss stage,
 * from L7_s10_sub.o (m_106ae .. m_1092a). It does nothing by itself: Wendy (obj_wendy.c)
 * puts it at her x - 26 and sets state 2 when she climbs on, and moves it from state 3
 * to 4 when she's knocked off. It has no collision box (cur_floor 5, box all 0).
 * Frames: 1-2.
 */
#include "obj_common.h"

static void draw_frame(actor_t *a, gds *g) { draw_at_xy(a, g, a->frame); }   /* m_1092a */

static int bb_appear(actor_t *a, gds *g) {              /* m_10720 */
    a->y = a->drawn_y0 = a->drawn_y1 = 0x31;
    a->cur_floor = 5;
    a->box_x0 = a->box_x1 = a->box_y0 = a->box_y1 = 0;
    a->frame = 1;
    draw_frame(a, g);
    a->state = 3; a->step = 0;
    return 0;
}

static int bb_show(actor_t *a, gds *g) {                /* m_10788 */
    a->frame = 2;
    draw_frame(a, g);
    return 0;
}

static int bb_going(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }

static int bb_last(actor_t *a, gds *g) {                /* m_107fa */
    a->frame = 1;
    draw_frame(a, g);
    a->step = 1; a->unk_02a = 2;
    return 0;
}

static int bb_gone(actor_t *a, gds *g) {                /* m_1083e */
    (void)g;
    if (a->unk_02a-- < 2) { a->state = 0; a->step = 0; }
    return 0;
}

int w_bon_bon(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(2, 1, obj_erase);
        H(2, 2, bb_appear);
        H(3, 1, obj_erase);
        H(3, 2, bb_show);
        H(4, 1, obj_erase);
        H(4, 2, bb_going);
        B(0, bb_last);
        B(1, bb_gone);
    }
    return obj_dispatch(a, g);
}
