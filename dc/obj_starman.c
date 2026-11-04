/* obj_starman.c - "starman": the bouncing star that comes out of a door and runs along
 * the floor (L1_s08_sub.o m_0043a .. m_00c5e; L5_s07_sub's copy is identical).
 * It waits for its door to be shut for `delay` ticks, then comes out of it when it is
 * opened, heading away from the way Mario faces, at 4 pixels a tick. At either wall it
 * either drops off the screen (drops_out) or just vanishes, and starts again
 * (appear_count times). cmd 3 (Mario takes it) is worth 500 - the invincibility itself
 * is Mario's side (item type 1 in mario_coll_det); cmd 4 makes it fall.
 * Frames: 1-7 spin, 5 also while falling.
 */
#include "obj_common.h"

static void place(actor_t *a, gds *g) {
    a->y = a->drawn_y0 = a->drawn_y1 = (int2)(g->floor_y[a->floor] - 0x18);
    a->cur_floor = a->floor;
    a->state = 1;
    a->box_y0 = a->y + 2; a->box_y1 = a->y + 0x18;
}

static int st_init(actor_t *a, gds *g) {                /* m_0043a */
    place(a, g);
    a->appear_count--;
    return 0;
}

static int st_again(actor_t *a, gds *g) {               /* m_00514: state 0 cmd 2 */
    if ((CELL(g, a->floor, a->col) & 0xf000) == 0x1000) a->timer++;
    if (a->delay < a->timer) {
        a->unk_04e = 0; a->timer = 0;
        if (a->appear_count-- > 0) place(a, g);
    }
    return 0;
}

static int st_wait(actor_t *a, gds *g) {                /* m_005d6: out when the door opens */
    if ((CELL(g, a->floor, a->col) & 0xf000) == 0x2000) {
        if (a->dir == -1) a->dir = (g->mario->state & 1) ? 0 : 1;
        a->x = (int2)(a->dir == 0 ? a->col * 0x2c + 0x5a : a->col * 0x2c + 10);
        a->box_x0 = a->x + 4; a->box_x1 = a->x + 0x10;
        a->state = 3; a->step = 0;
    }
    return 0;
}

static int st_moving(actor_t *a, gds *g) {              /* m_006b8 */
    int r = CALL(a->walk_steps[a->step], a, g);
    draw_at(a, g, a->unk_02a + 1);
    return r;
}

static void off_edge(actor_t *a, gds *g) {
    a->state = 4;
    if (!drops_out(HOTEL(g), STAGE(g))) { a->step = 0; a->unk_02a = 3; }
    else { a->step = 3; a->unk_02c = 0; a->unk_02a = 0; }
}

static int st_run(actor_t *a, gds *g) {                 /* m_0078c */
    if (--a->unk_02c < 1) {
        a->unk_02c = 1;
        if (++a->unk_02a > 6) a->unk_02a = 0;
    }
    a->x += (a->dir == 1) ? -4 : 4;
    if (a->x > 0x14a) off_edge(a, g);
    if (a->x < 0x22) off_edge(a, g);
    a->box_x0 = a->x + 4; a->box_x1 = a->x + 0x10;
    return 0;
}

static int st_taken(actor_t *a, gds *g) {               /* m_008f2: cmd 3 */
    if (g->unk_496 == 0) { SCORE(g, 500); a->state = 4; a->step = 0; a->unk_02a = 3; }
    else a->unk_04e = a->cmd;
    return 0;
}

static int st_destroy(actor_t *a, gds *g) {             /* m_00958: cmd 4 */
    if (g->unk_496 == 0) { a->state = 4; a->step = 3; a->unk_02c = 0; a->unk_02a = 0; }
    else a->unk_04e = a->cmd;
    return 0;
}

static int st_dying(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }   /* m_009b0 */

static int st_gone(actor_t *a, gds *g) {                /* m_009ea */
    (void)g;
    if (a->unk_02a-- < 2) { a->state = 0; a->cmd = 2; a->timer = 0; a->dir = -1; }
    return 0;
}

static int st_hop(actor_t *a, gds *g) {                 /* m_00a24 */
    draw_at(a, g, 5);
    if (++a->unk_02c == 2) a->step = 4;
    return 0;
}

static int st_fall(actor_t *a, gds *g) {                /* m_00ad8 */
    a->y += 0x10;
    if (a->y < 0xda) draw_at_y(a, g, 5);
    else a->step = 5;
    return 0;
}

static int st_reset(actor_t *a, gds *g) {               /* m_00b98 */
    (void)g;
    a->unk_02c = 0; a->unk_02a = 0; a->state = 0; a->timer = 0; a->dir = -1;
    return 0;
}

int starman(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, st_init);
        H(0, 2, st_again);
        H(1, 2, st_wait);
        H(3, 1, obj_erase);
        H(3, 2, st_moving);
        H(3, 3, st_taken);
        H(3, 4, st_destroy);
        H(4, 1, obj_erase);
        H(4, 2, st_dying);
        W(0, st_run);
        B(0, st_gone);
        B(3, st_hop);
        B(4, st_fall);
        B(5, st_reset);
    }
    return obj_dispatch(a, g);
}
