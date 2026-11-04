/* obj_bow_fb.c - "bow_fb" (type 0x3a): the fireball the boss throws in the last stage
 * (hotel 8 stage 15), from L8_s15_sub.o (m_0fa26 .. m_10194).
 * Idle in state 0 until the boss launches it (state 3, x_min = speed 8, unk_036 = -10):
 * it flies ahead in an arc (gravity +1 a tick, up to 16) and goes out at the screen
 * edges. Touching it hurts Mario (cmd 3 returns 1); a fireball or star puts it out
 * (10 points). Frames 1-4.
 * If the stage had type-0x3b actors (this one doesn't), x_max is the first one's index
 * and int32 unk_25c+0 is 1: the ball then sometimes lands on a floor and leaves one of
 * them burning there.
 */
#include "obj_common.h"

static int fb_init(actor_t *a, gds *g) {                /* m_0fa26 */
    int i;
    a->unk_02a = 0;
    for (i = 0; i < 24 && g->room->actors[i].type != 0; i++)
        if (g->room->actors[i].type == 0x3b) { a->x_max = i; wr25c(a, 0, 1); return 0; }
    return 0;
}

static int fb_idle(actor_t *a, gds *g) { (void)g; a->unk_02a = 0; return 0; }   /* m_0fb00 */

static int fb_moving(actor_t *a, gds *g) {              /* m_0fb28 */
    CALL(a->walk_steps[a->step], a, g);
    if (a->state == 3) draw_at_xy(a, g, a->frame);
    return 0;
}

static int fb_hurts(actor_t *a, gds *g) { (void)a; (void)g; return 1; }   /* m_0ff60 */

static void fb_out(actor_t *a) { a->state = 4; a->step = 3; a->unk_02a = 2; }

static int fb_destroy(actor_t *a, gds *g) {             /* m_0ff84 */
    if (g->unk_496 != 0) { a->unk_04e = a->cmd; return 0; }
    if (a->unk_04c == 0 && g->unk_53a == 0) SCORE(g, 10);
    else a->unk_04c = 0;
    fb_out(a);
    return 0;
}

/* a floor line just crossed this tick, or 0 */
static int floor_crossed(actor_t *a, int *line) {
    int y1 = a->y + a->unk_036, f = y1 + 10;
    if (f < 0x90) f = (f < 0x6c) ? 0x48 : 0x6c;
    else if (f < 0xb4) f = 0x90;
    else if (f < 0xd8) f = 0xb4;
    else f = 0xd8;
    *line = f;
    return a->y <= f && f < y1;
}

static void leave_flame(actor_t *a, gds *g, int line) {
    actor_t *acts = g->room->actors;
    int i = (int)a->x_max, pick = i;
    for (; i < 24 && acts[i].type == 0x3b; i++) {
        if (acts[i].state == 0) { pick = i; break; }
        if (acts[pick].unk_02c < acts[i].unk_02c) pick = i;
    }
    if (pick >= 24 || (acts[pick].state != 3 && acts[pick].state != 0)) return;
    acts[pick].state = 3;
    acts[pick].unk_02c = 0;
    acts[pick].x = (int2)(a->x + (a->dir == 1 ? -(int2)a->x_min : (int2)a->x_min));
    acts[pick].y = (int2)(line - 0x10);
    fb_out(a);
}

static int fb_fly(actor_t *a, gds *g) {                 /* m_0fc16 */
    a->frame = (int2)(a->unk_02a + 1);
    if (++a->unk_02a > 3) a->unk_02a = 0;
    if (a->dir == 1) a->x -= (int2)a->x_min; else a->x += (int2)a->x_min;
    a->y += a->unk_036;
    if (a->unk_036 < 0x10) a->unk_036++;
    if (a->x < 0xe || a->x > 0x166 || a->y < 2 || a->y > 0xe8) fb_out(a);
    else if (rd25c(a, 0) != 0 && a->unk_036 > 0 && a->x > 0x22 && a->x < 0x152 && (RND(g) & 3) == 0) {
        int line;
        if (floor_crossed(a, &line)) leave_flame(a, g, line);
    }
    a->box_x0 = a->x + 1; a->box_x1 = a->x + 0xb;
    a->box_y0 = a->y; a->box_y1 = a->y + 10;
    a->cur_floor = g->mario->floor;
    return 0;
}

static int fb_dying(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }   /* m_10018 */

static int fb_gone(actor_t *a, gds *g) {                /* m_10052 */
    (void)g;
    if (a->unk_02a-- < 2) {
        a->state = 0; a->step = 0; a->unk_04e = 0;
        a->box_x0 = a->box_x1 = a->box_y0 = a->box_y1 = 0;
    }
    return 0;
}

int bow_fb(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, fb_init);
        H(0, 2, fb_idle);
        H(3, 1, obj_erase);
        H(3, 2, fb_moving);
        H(3, 3, fb_hurts);
        H(3, 4, fb_destroy);
        H(4, 1, obj_erase);
        H(4, 2, fb_dying);
        W(0, fb_fly);
        B(3, fb_gone);
    }
    return obj_dispatch(a, g);
}
