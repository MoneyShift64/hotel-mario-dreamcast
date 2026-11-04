/* obj_igg_jet.c - "igg_jet" (type 0x3e): the boss's breath of flame in the last stage
 * (hotel 8 stage 15), from L8_s15_sub.o (m_106f0 .. m_10ac6).
 * Sits idle in state 0 until the boss puts it in front of its mouth (state 3); plays
 * frames 1-5 in place, then goes back to idle a couple of ticks later. It never sets
 * a hit box. cmd 3 returns 0, cmd 4 returns 2.
 */
#include "obj_common.h"

static int ij_moving(actor_t *a, gds *g) {              /* m_1078c */
    CALL(a->walk_steps[a->step], a, g);
    if (a->state == 3) draw_at_xy(a, g, a->frame);
    return 0;
}

static int ij_tough(actor_t *a, gds *g) { (void)a; (void)g; return 2; }     /* m_1093c */

static int ij_burn(actor_t *a, gds *g) {                /* m_1087a */
    (void)g;
    a->frame = (int2)(a->unk_02a + 1);
    if (++a->unk_02a > 4) { a->state = 4; a->unk_02a = 2; }
    if (a->x < 0 || a->x > 0x172 || a->y < 2 || a->y > 0xec) { a->state = 0; a->unk_02a = 0; }
    return 0;
}

static int ij_dying(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }   /* m_10960 */

static int ij_out(actor_t *a, gds *g) {                 /* m_1099a */
    (void)g;
    if (a->unk_02a-- < 2) { a->state = 0; a->unk_02a = 0; }
    return 0;
}

int igg_jet(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(3, 1, obj_erase);
        H(3, 2, ij_moving);
        H(3, 4, ij_tough);
        H(4, 1, obj_erase);
        H(4, 2, ij_dying);
        W(0, ij_burn);
        B(0, ij_out);
    }
    return obj_dispatch(a, g);
}
