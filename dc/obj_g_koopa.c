/* obj_g_koopa.c - "g_koopa" (Larry's hotel), from L3_s04_sub.o (m_004a8 .. m_01c90).
 * Type 0x2d is a koopa much like "koopa"; with start_mode >= 2 it is the koopa carrying a
 * bonus (drawn as frame + 0x23 on top): after its delay it comes out, wanders for 150
 * ticks and goes back in through a door (closing it); destroying it gives 5 lives.
 * Any other type is the puzzle_3 object: hitting it sets gds->unk_81a.
 * unk_25c: int32 left limit, int32 right limit. x_max: the door it opened (cell + 1).
 * Frames: 1-10 walk, 11-13 turn, 14-18 destroyed, 19-22 hit, 23-24 out of the door,
 * 25-27 / 28-31 the puzzle object, 32-33 its hit, 34-36 into a door, +0x23 the bonus.
 */
#include "obj_common.h"

#define GK_NORMAL 0x2d

static void at_door(actor_t *a, gds *g) {
    a->cur_floor = a->floor;
    a->x = a->drawn_x0 = a->drawn_x1 = g->door_x[a->col];
    a->y = a->drawn_y0 = a->drawn_y1 = (int2)(g->floor_y[a->floor] - 0x1c);
}

static void draw_frame(actor_t *a, gds *g) { draw_at_xy(a, g, a->frame); }   /* m_01c90 */

static int gk_init(actor_t *a, gds *g) {                /* m_004a8 */
    uint16_t l0, l6;
    a->speed = HOTEL(g) == 8 ? 6 : 4;
    at_door(a, g);
    if (a->start_mode > 1 && g->player[g->cur_player].unk_10 != 0) a->start_mode = 0;
    if (a->start_mode < 2) {
        a->delay = 0x1e; a->timer = 0x1e; a->appear_count = 1;
    } else {
        a->appear_count = 0;
        a->delay = (int2)(6 * (*(int32_t *)(g->stage_info + 8) - a->delay));
        a->x_min = 0;
        a->timer = 0;
    }
    a->unk_04a = a->unk_048 = a->appear_count + 1;
    if (a->start_mode == 1) { a->unk_02a = 1; a->state = 3; a->step = 0; }
    else a->state = 1;
    l0 = CELL(g, a->floor, 0) & 0xf000;
    l6 = CELL(g, a->floor, 6) & 0xf000;
    wr25c(a, 0, (l0 == 0xf000 || l0 == 0xe000) ? 0x4c : 0x22);
    wr25c(a, 4, (l6 == 0xf000 || l6 == 0xe000) ? 0x118 : 0x142);
    return 0;
}

static int gk_dormant(actor_t *a, gds *g) {             /* m_009aa */
    if (a->start_mode < 2 && a->appear_count-- > 0) {
        at_door(a, g);
        a->state = 1;
        a->hit_y1 = a->hit_y0 = a->hit_x1 = a->hit_x0 = 0;
        a->box_y1 = a->box_y0 = a->box_x1 = a->box_x0 = 0;
        a->unk_04e = 0;
    }
    return 0;
}

static int gk_wait_door(actor_t *a, gds *g) {           /* m_00a5c */
    uint16_t *c = &CELL(g, a->floor, a->col);
    int mx, ax, dist;
    if (g->unk_53a != 0 || ++a->timer < a->delay || (*c & 0xf000) == 0) return 0;
    mx = g->mario->x + 0xe;
    ax = a->x + 0xe;
    if (ax < mx) { if (a->dir == -1) a->dir = 0; dist = mx - ax; }
    else         { if (a->dir == -1) a->dir = 1; dist = ax - mx; }
    if (a->start_mode < 2) {
        if (dist > 0x48 && g->mario->floor == a->cur_floor && (*c & 0xf000) == 0x2000) {
            a->state = 2; a->step = 0;
        }
    } else if (dist > 0x48 || g->mario->floor != a->cur_floor) {
        a->state = 2; a->step = 0;
        if ((*c & 0xf000) == 0x1000) {
            *c = (*c & 0xfff) | 0x2009;
            a->x_max = a->floor * COLS + a->col + 1;      /* (the original keeps the pointer) */
            g->unk_054 = 2;
        }
    }
    return 0;
}

static int gk_emerge(actor_t *a, gds *g) {              /* m_00c14 */
    uint16_t *c;
    switch (a->step) {
    case 0: a->frame = 0x17; a->step = 1; CELL(g, a->floor, a->col) |= 4; break;
    case 1: a->frame = 0x18; a->step = 2; break;
    case 2:
        if (a->start_mode < 2) { a->step = 0; a->state = 3; a->unk_02a = 1; }
        else a->step = 3;
        CELL(g, a->floor, a->col) = (CELL(g, a->floor, a->col) & ~4) | 2;
        a->frame = (a->dir == 1) ? 0xd : 0xb;
        break;
    case 3:                                 /* the bonus koopa shuts the door it opened */
        c = a->x_max > 0 ? &hm_world->doors.cell[a->x_max - 1] : NULL;
        if (c && (*c & 0xf000) == 0x2000) {
            *c = (uint16_t)(((*c & 0xfff) | 0x1000) & ~4);
            *c |= 1 | 8;
            g->unk_054 = 1;
        }
        a->frame = (a->dir == 1) ? 6 : 1;
        a->step = 0; a->state = 3; a->unk_02a = 1;
        break;
    }
    draw_frame(a, g);
    return 0;
}

static int gk_moving(actor_t *a, gds *g) {              /* m_00dcc */
    int r = CALL(a->walk_steps[a->step], a, g);
    draw_frame(a, g);
    if (a->start_mode > 1 && a->type == GK_NORMAL) {   /* what it carries */
        a->frame += 0x23;
        draw_frame(a, g);
    }
    return r;
}

static int gk_walk(actor_t *a, gds *g) {                /* m_00e50 */
    int2 x;
    uint16_t *door;
    if (a->type != GK_NORMAL) {                         /* the puzzle object */
        if (++a->unk_02a > 2) a->unk_02a = 0;
        a->frame = (int2)(a->unk_02a + 0x19);
        if (++a->x_min > 0x1e) { a->state = 4; a->step = 9; a->unk_02a = 0; }
        return 0;
    }
    if (--a->unk_02c < 1) {
        a->unk_02c = 1;
        if (a->unk_02e == 0) {
            if (a->unk_02a < 4) a->unk_02a++; else { a->unk_02a--; a->unk_02e = 1; }
        } else if (a->unk_02a < 1) { a->unk_02a++; a->unk_02e = 0; }
        else a->unk_02a--;
    }
    if (a->dir == 1) { x = (int2)(a->x - a->speed); a->frame = (int2)(a->unk_02a + 6); }
    else             { x = (int2)(a->x + a->speed); a->frame = (int2)(a->unk_02a + 1); }
    a->x = x;
    if (a->unk_02a == 0 && a->cur_floor > 0 && g->mario->floor == a->cur_floor) {
        mario_t *m = g->mario;
        int above = a->cur_floor - 1, i, same_dir = 0, other_dir = 0;
        actor_t *p;
        for (i = 0, p = g->room->actors; i < 24 && p->type != 0; i++, p++) {
            int near;
            if (p->state != 3 || p->cur_floor != above || p->speed != a->speed) continue;
            near = (m->x < x) ? (x - 0x3c < p->x && p->x < x + 8) : (p->x < x + 0x3c && x - 8 < p->x);
            if (!near) continue;
            if (p->dir == a->dir) same_dir = 1; else other_dir = 1;
        }
        if (same_dir) {
            if (!other_dir) { a->step = 1; a->unk_02a = 0; }
            else if ((a->dir == 1 && m->x < x) || (a->dir == 0 && x < m->x)) { a->step = 1; a->unk_02a = 0; }
        }
    }
    if (x > rd25c(a, 4))      { x = a->x = (int2)rd25c(a, 4); a->step = 1; a->unk_02a = 0; }
    else if (x < rd25c(a, 0)) { x = a->x = (int2)rd25c(a, 0); a->step = 1; a->unk_02a = 0; }
    a->box_y0 = a->y + 4;   a->box_y1 = a->y + 0xd;
    a->hit_y0 = a->y + 0xb; a->hit_y1 = a->y + 0x1b;
    if (a->dir == 1) { a->box_x0 = x + 6; a->box_x1 = x + 0x10; a->hit_x0 = x + 0xb; a->hit_x1 = x + 0x17; }
    else { a->box_x0 = x + 0xc; a->box_x1 = x + 0x16; a->hit_x0 = x + 5; a->hit_x1 = x + 0x11; }
    a->unk_03c = 1;
    if (a->start_mode > 1 && ++a->x_min > 0x96) {       /* the bonus koopa leaves */
        mario_t *m = g->mario;
        door = DOOR_CELL(a->cur_floor, x + 0xe);
        if (door && ((*door & 0xf000) == 0x2000 || (*door & 0xf000) == 0x1000) &&
            (m->floor != a->cur_floor || m->state_group != 4 || abs(x - m->x) > 0x22)) {
            if ((*door & 0xf000) == 0x1000) {
                *door = (*door & 0xfff) | 0x2009;
                g->unk_054 = 2;
            }
            a->state = 4; a->step = 7; a->unk_02a = 0;
            a->x = g->door_x[(x - 0x1e) / 0x2c];
        }
    }
    if (a->start_mode < 2) {
        door = DOOR_CELL(a->cur_floor, x + 0xe);
        if (door && (*door & 0xf000) == 0x1000 && (RND(g) & 0x1f) == 1) {
            *door = (*door & 0xfff) | 0x2009;
            g->unk_054 = 2;
        }
    }
    return 0;
}

static int gk_turn(actor_t *a, gds *g) {                /* m_0155e */
    (void)g;
    a->unk_03c = 0;
    a->box_x0 = a->x + 8; a->box_x1 = a->x + 0x14;
    a->box_y0 = a->y + 3; a->box_y1 = a->y + 0x1b;
    a->frame = (int2)((a->dir == 1) ? 0xd - a->unk_02a : a->unk_02a + 0xb);
    if (++a->unk_02a > 2) {
        a->unk_02a = 0;
        a->step = 0;
        a->dir = (a->dir == 0) ? 1 : 0;
    }
    return 0;
}

static int gk_hit(actor_t *a, gds *g) {                 /* m_01620 */
    if (g->unk_496 == 0) {
        if (a->type == GK_NORMAL) {
            SCORE(g, 100);
            a->unk_048--; a->unk_04a--;
            a->state = 4; a->step = 1; a->unk_02a = 0;
        } else {
            a->state = 4; a->step = 0; a->unk_02a = 0;
            g->unk_81a = 1;
        }
    } else a->unk_04e = a->cmd;
    return 0;
}

static int gk_destroy(actor_t *a, gds *g) {             /* m_016bc */
    if (g->unk_496 == 0) {
        if (a->type == GK_NORMAL) {
            if (a->unk_04c == 0 && g->unk_53a == 0) { SCORE(g, 100); a->unk_04a--; }
            else a->unk_04c = 0;
            a->state = 4; a->step = 4; a->unk_02a = 0xe; a->unk_02c = 0;
            if (a->start_mode > 1) {                    /* the bonus: 5 lives */
                int2 *l = &g->player[g->cur_player].lives;
                *l += 5;
                if (*l > 99) *l = 99;
                g->unk_054 = 8;
            }
        } else {
            a->state = 4; a->step = 9; a->unk_02a = 0; a->unk_02c = 0;
        }
    } else a->unk_04e = a->cmd;
    return 0;
}

static int gk_dying(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }

static int gk_hit_anim(actor_t *a, gds *g) {            /* m_018d6 */
    a->frame = (int2)(a->unk_02a + 0x13);
    draw_frame(a, g);
    if (a->start_mode < 2) a->unk_02a++;
    else { a->frame = (int2)(a->unk_02a + 0x31); draw_frame(a, g); }
    if (++a->unk_02a > 3) {
        if (a->start_mode < 2) { a->step = 3; a->unk_02a = 2; }
        else { a->type = 6; a->step = 8; a->unk_02a = 0; }
    }
    return 0;
}
static int gk_puzzle_hit_1(actor_t *a, gds *g) {        /* m_0198a */
    a->frame = 0x20; draw_frame(a, g);
    if (++a->unk_02a > 0) a->step = 2;
    return 0;
}
static int gk_puzzle_hit_2(actor_t *a, gds *g) {        /* m_019d0 */
    a->frame = 0x21; draw_frame(a, g);
    if (++a->unk_02a > 1) { a->step = 3; a->unk_02a = 2; }
    return 0;
}
static int gk_gone(actor_t *a, gds *g) {                /* m_01a1c */
    (void)g;
    if (a->unk_02a-- < 2) { a->state = 0; a->cmd = 2; a->timer = 0; a->dir = -1; }
    return 0;
}
static int gk_flip(actor_t *a, gds *g) {                /* m_01804 / m_01838 */
    a->frame = a->unk_02a; draw_frame(a, g);
    a->unk_02a++;
    a->step = (int2)(a->step == 4 ? 5 : 6);
    return 0;
}
static int gk_fall(actor_t *a, gds *g) {                /* m_0186c */
    a->y += 0x10;
    a->frame = a->unk_02a; draw_frame(a, g);
    if (++a->unk_02a > 0x12) a->unk_02a = 0x10;
    if (a->y > 0xd5) { a->step = 3; a->unk_02a = 2; }
    return 0;
}
static int gk_into_door(actor_t *a, gds *g) {           /* m_01a56 */
    uint16_t *c = DOOR_CELL(a->cur_floor, a->x + 0xe);
    if (c == NULL) { a->step = 3; a->unk_02a = 2; }
    else *c |= 4;
    a->frame = (int2)(a->unk_02a + 0x22);
    a->unk_02a++;
    if (a->frame > 0x24) {
        a->step = 3; a->unk_02a = 2;
        if (c && (*c & 0xf000) == 0x2000) {             /* and shuts it behind it */
            *c = (uint16_t)(((*c & 0xfff) & ~4) | 0x1009);
            g->unk_054 = 1;
        }
    }
    if (a->frame < 0x24) draw_frame(a, g);
    return 0;
}
static int gk_recover(actor_t *a, gds *g) {             /* m_01b2c */
    a->frame = (int2)(0x1f - a->unk_02a);
    if (++a->unk_02a > 3) { a->x_min = 0; a->state = 3; a->step = 0; a->unk_02a = 0; }
    draw_frame(a, g);
    return 0;
}
static int gk_puzzle_gone(actor_t *a, gds *g) {         /* m_01b82 */
    a->frame = (int2)(a->unk_02a + 0x1c);
    if (++a->unk_02a > 3) { a->step = 3; a->unk_02a = 2; }
    draw_frame(a, g);
    return 0;
}

int g_koopa(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, gk_init);
        H(0, 2, gk_dormant);
        H(1, 2, gk_wait_door);
        H(2, 1, obj_erase);
        H(2, 2, gk_emerge);
        H(3, 1, obj_erase);
        H(3, 2, gk_moving);
        H(3, 3, gk_hit);
        H(3, 4, gk_destroy);
        H(4, 1, obj_erase);
        H(4, 2, gk_dying);
        W(0, gk_walk);
        W(1, gk_turn);
        B(0, gk_puzzle_hit_1);
        B(1, gk_hit_anim);
        B(2, gk_puzzle_hit_2);
        B(3, gk_gone);
        B(4, gk_flip);
        B(5, gk_flip);
        B(6, gk_fall);
        B(7, gk_into_door);
        B(8, gk_recover);
        B(9, gk_puzzle_gone);
    }
    return obj_dispatch(a, g);
}
