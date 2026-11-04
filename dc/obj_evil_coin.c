/* obj_evil_coin.c - "evil_coin" (hotel 7, stages 4 and 7; the same code in L7_s04_sub.o
 * and L7_s07_sub.o, m_08000 .. m_0916c in s04). Starts out as an ordinary coin (type 4)
 * that rolls out of its door when the door is open. After a short spin it turns into an
 * evil coin (type 0x2b, an enemy): it runs along the floor at 8 pixels a tick, turning
 * to chase Mario, and is stomped or burnt like any other enemy (100 points). While the
 * player's unk_10 flag is set (x_min = 1) it stays a plain rolling coin that can be
 * collected. start_mode 1 is an invisible coin spot like the coin object's.
 * Frames: 1-6 coin spin, 5/7/8 coin collected/falling, 9-13 about to turn, 14-23 turning
 * (right / left), 24-33 running (right / left), 34-39 turning round, 40-41 squashed,
 * 42-46 hit and falling.
 */
#include "obj_common.h"

#define EVIL 0x2b

static void draw_frame(actor_t *a, gds *g) { draw_at_xy(a, g, a->frame); }   /* m_0916c */

static void appear(actor_t *a, gds *g) {
    a->y = a->drawn_y0 = a->drawn_y1 = (int2)(g->floor_y[a->floor] - 0x18);
    a->cur_floor = a->floor;
    a->state = 1;
    a->box_y0 = a->y + 7; a->box_y1 = a->y + 0x18;
}

static int ec_init(actor_t *a, gds *g) {                /* m_08000 */
    if (g->item_hit < 10) {
        if (a->start_mode == 1) {                       /* an invisible coin spot */
            a->y = g->floor_y[a->floor];
            a->cur_floor = a->floor;
            a->box_y0 = a->y - 0x23; a->box_y1 = a->box_y0 + 1;
            a->x = (int2)((a->col + 1) * 0x2c);
            a->box_x0 = a->x + 0x16; a->box_x1 = a->box_x0 + 1;
            a->state = 3; a->step = 0;
            a->delay = 5;
        } else appear(a, g);
        if (a->appear_count == 0) a->appear_count = (RND(g) & 3) + 1;
        a->appear_count--;
        a->type = 4;
    } else {                                            /* made from an enemy by item 11/12 */
        a->floor = a->cur_floor;
        a->y = a->drawn_y0 = a->drawn_y1 = (int2)(g->floor_y[a->floor] - 0x18);
        a->box_y0 = a->y + 7; a->box_y1 = a->y + 0x18;
        a->box_x1 = a->x + 0x14; a->box_x0 = a->x + 4;
        a->dir = (0xb3 < a->x);
        a->appear_count = RND(g) & 3;
        a->type = 4;
        a->state = 3; a->step = 0;
    }
    return 0;
}

static int ec_dormant(actor_t *a, gds *g) {             /* m_08462: state 0, cmd 2 */
    if ((CELL(g, a->floor, a->col) & 0xf000) == 0x1000 || a->start_mode == 1) a->timer++;
    if (a->delay < a->timer) {
        a->timer = 0;
        if (a->appear_count-- > 0) {
            if (a->start_mode == 1) { a->state = 3; a->step = 0; }
            else appear(a, g);
            a->unk_04e = 0;
        }
        a->type = 4;
    }
    return 0;
}

static int ec_wait(actor_t *a, gds *g) {                /* m_08562: rolls out of an open door */
    if ((CELL(g, a->floor, a->col) & 0xf000) == 0x2000) {
        if (a->dir == -1) a->dir = (g->mario->state & 1) == 0 ? 1 : 0;
        a->x = (int2)(a->dir == 0 ? a->col * 0x2c + 0x5a : a->col * 0x2c + 10);
        a->box_x0 = a->x + 4; a->box_x1 = a->x + 0x14;
        a->state = 3; a->step = 0; a->unk_02a = 0;
    }
    return 0;
}

static int ec_moving(actor_t *a, gds *g) {              /* m_08648 */
    int r = CALL(a->walk_steps[a->step], a, g);
    if (a->start_mode != 1) draw_frame(a, g);
    return r;
}

static void off_screen(actor_t *a, gds *g) {
    a->state = 4;
    if (!drops_out(HOTEL(g), STAGE(g))) { a->step = 2; a->unk_02a = 2; }
    else { a->step = 3; a->unk_02a = 1; }
}

static int ec_roll(actor_t *a, gds *g) {                /* m_086a6: the coin */
    int d;
    if (a->start_mode == 1) return 0;
    if (a->x_min == 0) {
        a->frame = (int2)(a->unk_02a + 9);
        if (++a->unk_02a > 4) { a->step = 3; a->unk_02a = 0; }
    } else {
        a->frame = (int2)(a->unk_02a + 1);
        if (++a->unk_02a == 6) a->unk_02a = 0;
        else if (a->unk_02a > 6) { a->unk_02a = 0; a->frame = 1; }
    }
    a->x += (a->dir == 1) ? -2 : 2;
    if (a->x > 0x154) off_screen(a, g);
    if (a->x < 0x14) off_screen(a, g);
    a->box_y0 = abs(a->unk_02a * -4 + 0xc) + a->y + 1;
    d = 8 - abs(a->unk_02a * -2 + 6);
    a->box_y1 = a->y + 0x18 - d;
    a->box_x0 = d + a->x + 2;
    a->box_x1 = a->x + 0x16 - d;
    return 0;
}

static void back_to_coin(actor_t *a) {
    a->frame = 1; a->type = 4; a->step = 0; a->unk_02a = 1;
}

static int ec_run(actor_t *a, gds *g) {                 /* m_088f0: the evil coin */
    mario_t *m = g->mario;
    uint16_t *door;
    if (a->x_min != 0) back_to_coin(a);
    else {
        if (a->unk_02e == 0) {
            if (a->unk_02a < 4) a->unk_02a++; else { a->unk_02a--; a->unk_02e = 1; }
        } else if (a->unk_02a < 1) { a->unk_02a++; a->unk_02e = 0; }
        else a->unk_02a--;
        if (a->dir == 1) { a->x -= 8; a->frame = (int2)(a->unk_02a + 0x1d); }
        else { a->x += 8; a->frame = (int2)(a->unk_02a + 0x18); }
        if (m->floor == a->cur_floor && m->state_group == 0 &&
            ((m->x < a->x && a->dir == 0) || (a->x < m->x && a->dir == 1))) {
            a->step = 2; a->unk_02a = 0;
        }
        if (a->x >= 0x147) { a->x = 0x146; a->step = 2; a->unk_02a = 0; }
        else if (a->x < 0x22) { a->x = 0x22; a->step = 2; a->unk_02a = 0; }
        door = DOOR_CELL(a->cur_floor, a->x + 0xc);
        if (door && (*door & 0xf000) == 0x1000 && (RND(g) & 0x1f) == 1) {
            *door = (*door & 0xfff) | 0x2009;
            g->unk_054 = 2;
        }
    }
    a->box_x0 = a->x + 4; a->box_x1 = a->x + 0x14;
    return 0;
}

static int ec_turn(actor_t *a, gds *g) {                /* m_08c12 */
    (void)g;
    if (a->x_min != 0) { back_to_coin(a); return 0; }
    a->frame = (int2)(a->unk_02a + (a->dir == 1 ? 0x25 : 0x22));
    if (++a->unk_02a > 2) {
        a->step = 1; a->unk_02a = 0;
        a->dir = (a->dir == 0) ? 1 : 0;
    }
    return 0;
}

static int ec_change(actor_t *a, gds *g) {              /* m_08b3c: coin -> evil coin */
    (void)g;
    if (a->x_min != 0) { back_to_coin(a); return 0; }
    a->frame = (int2)(a->unk_02a + (a->dir == 1 ? 0x13 : 0xe));
    if (a->unk_02a == 0) {
        a->box_y0 = a->y + 7; a->box_y1 = a->y + 0x18;
        a->box_x0 = a->x + 4; a->box_x1 = a->x + 0x14;
    }
    if (++a->unk_02a > 4) { a->type = EVIL; a->step = 1; a->unk_02a = 1; }
    return 0;
}

static int ec_hit(actor_t *a, gds *g) {                 /* m_08ccc: collected / stomped */
    if (g->unk_496 == 0) {
        SCORE(g, 100);
        if (a->type == 4) g->player[g->cur_player].coins++;
        a->state = 4;
        a->step = (a->start_mode == 1) ? 2 : 0;
        a->unk_02a = 0;
    } else a->unk_04e = a->cmd;
    return 0;
}

static int ec_destroy(actor_t *a, gds *g) {             /* m_08d70 */
    if (g->unk_496 == 0) {
        if (a->start_mode == 1) {
            if (g->unk_53a != 0) { a->state = 0; a->step = 0; a->unk_02a = 0; a->appear_count = 0; }
        } else {
            if (a->unk_04c == 0 && g->unk_53a == 0) { if (a->type != 4) SCORE(g, 100); }
            else a->unk_04c = 0;
            a->state = 4; a->step = 3; a->unk_02a = 0;
        }
    } else a->unk_04e = a->cmd;
    return 0;
}

static int ec_dying(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }   /* m_08e50 */

static int ec_squash1(actor_t *a, gds *g) {             /* m_08e8a */
    a->frame = a->type == 4 ? 7 : 0x28; draw_frame(a, g);
    if (++a->unk_02a > 0) a->step = 1;
    return 0;
}
static int ec_squash2(actor_t *a, gds *g) {             /* m_08ef2 */
    a->frame = a->type == 4 ? 8 : 0x29; draw_frame(a, g);
    if (++a->unk_02a > 1) { a->step = 2; a->unk_02a = 2; }
    return 0;
}
static int ec_gone(actor_t *a, gds *g) {                /* m_08f60 */
    (void)g;
    if (a->unk_02a-- < 2) { a->state = 0; a->timer = 0; a->dir = -1; }
    return 0;
}
static int ec_flip(actor_t *a, gds *g) {                /* m_08f94 */
    a->frame = (int2)(a->type == 4 ? 5 : a->unk_02a + 0x2a); draw_frame(a, g);
    if (++a->unk_02a == 2) { a->step = 4; a->unk_02a = 0; }
    return 0;
}
static int ec_fall(actor_t *a, gds *g) {                /* m_09004 */
    a->y += 0x10;
    if (a->y < 0xda) {
        if (a->type == 4) a->frame = 5;
        else {
            a->frame = (int2)(a->unk_02a + 0x2c);
            if (++a->unk_02a > 2) a->unk_02a = 0;
        }
        draw_frame(a, g);
    } else { a->step = 2; a->unk_02a = 2; }
    return 0;
}

static int ec_erase(actor_t *a, gds *g) { (void)a; (void)g; return 0; }   /* m_0909c */

int evil_coin(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, ec_init);
        H(0, 2, ec_dormant);
        H(1, 2, ec_wait);
        H(3, 1, ec_erase);
        H(3, 2, ec_moving);
        H(3, 3, ec_hit);
        H(3, 4, ec_destroy);
        H(4, 1, ec_erase);
        H(4, 2, ec_dying);
        W(0, ec_roll);
        W(1, ec_run);
        W(2, ec_turn);
        W(3, ec_change);
        B(0, ec_squash1);
        B(1, ec_squash2);
        B(2, ec_gone);
        B(3, ec_flip);
        B(4, ec_fall);
    }
    if (g->player[g->cur_player].unk_10 != 0) a->x_min = 1;
    return obj_dispatch(a, g);
}
