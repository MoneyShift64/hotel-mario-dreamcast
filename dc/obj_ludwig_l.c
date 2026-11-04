/* obj_ludwig_l.c - the hotel 5 boss's beam (type 0x37), from L5_s10_sub.o (m_175fa .. m_17e86).
 *
 * The beam is a chain of these actors. At init each one claims the next unclaimed one
 * (x_min == 0; the boss claims the first). Idle in state 0 until the boss (or the
 * previous link) puts it at a spot and sets state 3: it animates in place, 3 frames per
 * direction, and on each tick (start_mode 0) or every third tick (start_mode != 0) it
 * starts the next link 40 pixels further on (straight, a floor up or a floor down,
 * x_max = 0/1/2), until the chain leaves the screen. When the boss stops firing, the
 * first link is set to state 4: each link passes that on to the next and disappears.
 * Touching it hurts Mario (cmd 3 returns 1, cmd 4 returns 2: it can't be destroyed).
 * unk_25c int32 +0: the next link (actor index+1, 0 = none). It always counts as being on
 * Mario's floor. Frames 1-3 / 4-6 straight (dir 0 / 1), 7-9 / 10-12 up, 13-15 / 16-18 down.
 */
#include "obj_common.h"

static actor_t *link_of(gds *g, int32_t v) { return v ? &g->room->actors[v - 1] : NULL; }

static void no_boxes(actor_t *a) {
    a->hit_y1 = a->hit_y0 = a->hit_x1 = a->hit_x0 = 0;
    a->box_y1 = a->box_y0 = a->box_x1 = a->box_x0 = 0;
}

static int ll_init(actor_t *a, gds *g) {                /* m_175fa */
    int i;
    a->unk_02a = 0;
    a->x_min = 1;
    for (i = 0; i < 24 && g->room->actors[i].type != 0; i++) {
        actor_t *p = &g->room->actors[i];
        if (p->type == 0x37 && p->x_min == 0) {
            wr25c(a, 0, i + 1);
            p->x_min = 1;
            p->start_mode = a->start_mode;
            break;
        }
    }
    return 0;
}

static int ll_reset(actor_t *a, gds *g) { (void)g; a->unk_02a = 0; a->step = 0; return 0; }   /* m_177e4 */

static int ll_erase(actor_t *a, gds *g) {               /* m_17e86 (the score bar redraw flag) */
    int y = (g->draw_field == 1) ? a->drawn_y0 : a->drawn_y1;
    if (y < 0x24 && 0x13 < y + 0x24) g->unk_532 = 1;
    return 0;
}

static int ll_moving(actor_t *a, gds *g) {              /* m_17810 */
    int r = CALL(a->walk_steps[a->step], a, g);
    if (a->state == 3) draw_at_xy(a, g, a->frame);
    return r;
}

static int ll_hurts(actor_t *a, gds *g) { (void)a; (void)g; return 1; }      /* m_17d1c */
static int ll_tough(actor_t *a, gds *g) { (void)a; (void)g; return 2; }      /* m_17d40 */
static int ll_fading(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }   /* m_17d64 */

static int ll_shine(actor_t *a, gds *g) {               /* m_178fe */
    actor_t *n;
    int base = a->x_max == 1 ? 7 : a->x_max == 2 ? 0xd : 1;
    if (a->x_max >= 0 && a->x_max <= 2) a->frame = (int2)(base + a->unk_02a + (a->dir == 1 ? 3 : 0));
    if (++a->unk_02a > 2) a->unk_02a = 0;
    if (a->x_max == 0) {
        a->unk_03c = 0;
        a->box_x0 = a->x; a->box_x1 = a->x + 0x28;
        a->box_y0 = a->y + 0xf; a->box_y1 = a->y + 0x15;
    } else {
        a->unk_03c = 1;
        a->box_y0 = a->y + 2; a->box_y1 = a->y + 0xc;
        a->hit_y0 = a->y + 0x16; a->hit_y1 = a->y + 0x20;
        if ((a->dir == 0 && a->x_max == 2) || (a->dir == 1 && a->x_max == 1)) {
            a->box_x0 = a->x + 6; a->box_x1 = a->x + 0xe;
            a->hit_x0 = a->x + 0x1a; a->hit_x1 = a->x + 0x22;
        } else {
            a->box_x0 = a->x + 0x1a; a->box_x1 = a->x + 0x22;
            a->hit_x0 = a->x + 6; a->hit_x1 = a->x + 0xe;
        }
    }
    a->cur_floor = g->mario->floor;
    if (a->x < 0 || 0x156 < a->x || a->y < 2 || 0xde < a->y) {     /* off the screen: done */
        a->state = 0; a->step = 0;
        no_boxes(a);
    } else if ((a->start_mode == 0 || a->unk_02a == 2) &&
               (n = link_of(g, rd25c(a, 0))) != NULL && n->state == 0) {
        n->x = (int2)((a->dir == 1) ? a->x - 0x28 : a->x + 0x28);
        if (a->x_max == 0) n->y = a->y;
        else if (a->x_max == 1) n->y = a->y - 0x24;
        else if (a->x_max == 2) n->y = a->y + 0x24;
        n->dir = a->dir;
        n->x_max = a->x_max;
        n->state = 3;
    }
    return 0;
}

static int ll_pass_on(actor_t *a, gds *g) {             /* m_17d9e: steps_b[0] */
    actor_t *n;
    if ((0 < a->unk_02a || a->start_mode == 0) && (n = link_of(g, rd25c(a, 0))) != NULL && n->state == 3) {
        n->state = 4; n->unk_02a = 0;
    }
    if (++a->unk_02a > 1 || a->start_mode == 0) { a->step = 3; a->unk_02a = 1; }
    return 0;
}

static int ll_vanish(actor_t *a, gds *g) {              /* m_17e36: steps_b[3] */
    (void)g;
    if (a->unk_02a-- < 2) { no_boxes(a); a->state = 0; a->step = 0; }
    return 0;
}

int ludwig_l(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, ll_init);
        H(0, 2, ll_reset);
        H(3, 1, ll_erase);
        H(3, 2, ll_moving);
        H(3, 3, ll_hurts);
        H(3, 4, ll_tough);
        H(4, 1, ll_erase);
        H(4, 2, ll_fading);
        W(0, ll_shine);
        B(0, ll_pass_on);
        B(3, ll_vanish);
    }
    return obj_dispatch(a, g);
}
