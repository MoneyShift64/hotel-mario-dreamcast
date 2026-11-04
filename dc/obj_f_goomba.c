/* obj_f_goomba.c - "f_goomba" (the hopping winged goomba, L4_s03_sub.o, m_0d126 .. m_0e1fa).
 * It doesn't use the door: when Mario is more than 88 px away (or on another floor) it
 * simply appears in front of it (frames 15-21) and walks along hopping. The hop is a bit
 * pattern in x_min (0x2a516a), 2 bits a tick: y moves by 2 * bits, up while the rest is
 * >= 0x2a5, down after. Only between hops (x_min == 0) does it follow the usual walker
 * rule, turn at its walls (unk_25c: int32 left 0x38 / 0x4c with a lift, +4 right 0x124 /
 * 0x110) and maybe open the closed door it is passing (1 in 4).
 * Stomped or hit: 300 points, squashes (10-11) and falls off the screen (12-14), then it
 * comes back at a random door (appear_count times; 2 when the room gives 0, 30 ticks wait).
 * With start_mode != 0 it claims a type-0x28 actor with start_mode > 1 (marks its unk_25c
 * int32 = 1) and, when it dies, wakes that actor (state 0 -> 1). The original keeps the
 * pointer in x_max; here x_max is its index + 1 (0 = none).
 * Frames: 1-3 / 4-6 walk, 7-9 turn, 10-14 dying, 15-21 appearing.
 */
#include "obj_common.h"

static void draw_frame(actor_t *a, gds *g) { draw_at_xy(a, g, a->frame); }   /* m_0e1fa */

static void wake_linked(actor_t *a, gds *g) {
    if (a->x_max > 0 && a->x_max <= 24) {
        actor_t *p = &g->room->actors[a->x_max - 1];
        if (p->state == 0) p->state = 1;
    }
}

static int fg_init(actor_t *a, gds *g) {                /* m_0d126 */
    int2 v;
    int i;
    a->speed = HOTEL(g) == 8 ? 4 : 2;
    v = (int2)(g->floor_y[a->floor] - 0x18);
    a->y = a->drawn_y0 = a->drawn_y1 = v;
    a->cur_floor = a->floor;
    a->x = (int2)g->door_x[a->col];
    if (a->appear_count == 0) a->appear_count = 2; else a->appear_count--;
    a->unk_04a = a->unk_048 = (int2)(a->appear_count + 1);
    a->timer = 0;
    if (a->delay != 0) a->delay = (int2)(6 * (*(int32_t *)(g->stage_info + 8) - a->delay));
    a->state = 1;
    a->x_max = 0;
    if (a->start_mode != 0) {
        for (i = 0; i < 24 && g->room->actors[i].type != 0; i++) {
            actor_t *p = &g->room->actors[i];
            if (p->type == 0x28 && p->start_mode > 1 && rd25c(p, 0) == 0) {
                a->x_max = i + 1;
                wr25c(p, 0, 1);
                break;
            }
        }
    }
    return 0;
}

static int fg_again(actor_t *a, gds *g) {               /* m_0d410: back at a random door */
    int2 v;
    if (a->appear_count-- < 1) { a->appear_count = 0; return 0; }
    a->floor = (int2)smod((int)(RND(g) & 0xffff), 5);
    a->col = (int2)smod((int)(RND(g) & 0xffff), 7);
    a->dir = (int2)((RND(g) & 1) == 0);
    v = (int2)(g->floor_y[a->floor] - 0x18);
    a->y = a->drawn_y0 = a->drawn_y1 = v;
    a->cur_floor = a->floor;
    a->state = 1;
    a->x = (int2)g->door_x[a->col];
    a->hit_x0 = a->hit_x1 = a->hit_y0 = a->hit_y1 = 0;
    a->box_x0 = a->box_x1 = a->box_y0 = a->box_y1 = 0;
    a->delay = 0x1e;
    a->timer = 0;
    a->unk_04e = 0;
    return 0;
}

static int fg_wait(actor_t *a, gds *g) {                /* m_0d53c */
    uint16_t l0, l6;
    if (g->unk_53a != 0 || a->timer++ <= a->delay) return 0;
    if (door_far_from_mario(a, g, a->x + 0x12, 0x10) > 0x58 || g->mario->floor != a->floor) {
        a->state = 2; a->unk_02a = 0;
        l0 = CELL(g, a->cur_floor, 0) & 0xf000;
        l6 = CELL(g, a->cur_floor, 6) & 0xf000;
        wr25c(a, 0, (l0 == 0xf000 || l0 == 0xe000) ? 0x4c : 0x38);
        wr25c(a, 4, (l6 == 0xf000 || l6 == 0xe000) ? 0x110 : 0x124);
    }
    return 0;
}

static int fg_erase(actor_t *a, gds *g) {               /* m_0e0d8: the HUD band needs a redraw */
    int y = (g->draw_field == 1) ? a->drawn_y0 : a->drawn_y1;
    if (y < 0x24 && 0x13 < y + 0x18) g->unk_532 = 1;
    return 0;
}

static int fg_appear(actor_t *a, gds *g) {              /* m_0d6cc */
    a->frame = (int2)(a->unk_02a + 0xf);
    if (++a->unk_02a > 6) {
        if (g->unk_53a == 0) { a->unk_02a = 1; a->state = 3; a->step = 0; a->x_min = 0x2a516a; }
        else { a->state = 4; a->step = 0; a->unk_02a = 0; }
        a->frame = (a->dir == 1) ? 9 : 7;
    }
    draw_frame(a, g);
    return 0;
}

static int fg_moving(actor_t *a, gds *g) {              /* m_0d786 */
    int r = CALL(a->walk_steps[a->step], a, g);
    draw_frame(a, g);
    return r;
}

static int fg_walk(actor_t *a, gds *g) {                /* m_0d7d2 */
    uint32_t bits;
    int dy;
    if (a->unk_02e == 0) {
        if (a->unk_02a == 2) { a->unk_02a--; a->unk_02e = 1; } else a->unk_02a++;
    } else if (a->unk_02a == 0) { a->unk_02a++; a->unk_02e = 0; }
    else a->unk_02a--;
    if (a->dir == 1) { a->x -= a->speed; a->frame = (int2)(a->unk_02a + 4); }
    else             { a->x += a->speed; a->frame = (int2)(a->unk_02a + 1); }
    bits = (uint32_t)a->x_min;
    a->x_min = (int4)(bits >> 2);
    dy = (int)((bits & 3) << 1);
    if (a->x_min < 0x2a5) a->y += dy; else a->y -= dy;
    if (a->x_min == 0) {
        if (walker_follow(a, g, a->x, NULL)) { a->step = 1; a->unk_02a = 0; }
        a->x_min = 0x2a516a;
        if ((rd25c(a, 4) < a->x && a->dir == 0) || (a->x < rd25c(a, 0) && a->dir == 1)) {
            a->step = 1; a->unk_02a = 0;
        }
        {   /* m_000c2(floor, x + 0x12) */
            uint16_t *door = DOOR_CELL(a->floor, a->x + 0x12);
            if (door && (*door & 0xf000) == 0x1000 && (RND(g) & 3) == 1) {
                *door = (*door & 0xfff) | 0x2009;
                g->unk_054 = 2;
            }
        }
    }
    a->box_y0 = a->y + 4;   a->box_y1 = a->y + 0x10;
    a->hit_y0 = a->y + 0xb; a->hit_y1 = a->y + 0x17;
    if (a->dir == 1) { a->box_x0 = a->x + 8; a->box_x1 = a->x + 0x13; a->hit_x0 = a->x + 0x15; a->hit_x1 = a->x + 0x1b; }
    else { a->box_x0 = a->x + 0x11; a->box_x1 = a->x + 0x1c; a->hit_x0 = a->x + 9; a->hit_x1 = a->x + 0xf; }
    a->unk_03c = 1;
    return 0;
}

static int fg_turn(actor_t *a, gds *g) {                /* m_0dd8a */
    (void)g;
    a->unk_03c = 0;
    a->box_x0 = a->x + 0xb; a->box_x1 = a->x + 0x1a;
    a->box_y0 = a->y + 4;   a->box_y1 = a->y + 0x17;
    a->frame = (int2)((a->dir == 1) ? 9 - a->unk_02a : a->unk_02a + 7);
    if (++a->unk_02a > 2) {
        a->unk_02a = 1; a->step = 0;
        a->dir = (a->dir == 0) ? 1 : 0;
    }
    return 0;
}

static int fg_hit(actor_t *a, gds *g) {                 /* m_0de4e: stomped */
    if (g->unk_496 == 0) {
        SCORE(g, 300);
        a->unk_048--; a->unk_04a--;
        a->state = 4; a->step = 0; a->unk_02a = 0;
        wake_linked(a, g);
    } else a->unk_04e = a->cmd;
    return 0;
}

static int fg_destroy(actor_t *a, gds *g) {             /* m_0dee2 */
    if (g->unk_496 == 0) {
        if (a->unk_04c == 0 && g->unk_53a == 0) { SCORE(g, 300); a->unk_04a--; }
        else a->unk_04c = 0;
        a->state = 4; a->step = 0; a->unk_02a = 0;
        if (g->unk_53a == 0) wake_linked(a, g);
    } else a->unk_04e = a->cmd;
    return 0;
}

static int fg_dying(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }   /* m_0dfb0 */

static int fg_squash(actor_t *a, gds *g) {              /* m_0dfea */
    a->frame = (int2)(a->unk_02a + 10);
    draw_frame(a, g);
    if (++a->unk_02a > 1) { a->step = 1; a->unk_02a = 0; }
    return 0;
}

static int fg_fall(actor_t *a, gds *g) {                /* m_0e038 */
    a->y += 0x10;
    a->frame = (int2)(a->unk_02a + 0xc);
    draw_frame(a, g);
    if (++a->unk_02a > 2) a->unk_02a = 0;
    if (a->y > 0xd9) { a->step = 2; a->unk_02a = 2; }
    return 0;
}

static int fg_gone(actor_t *a, gds *g) {                /* m_0e0a4 */
    (void)g;
    if (a->unk_02a-- < 2) { a->state = 0; a->timer = 0; a->dir = -1; }
    return 0;
}

int f_goomba(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, fg_init);
        H(0, 2, fg_again);
        H(1, 2, fg_wait);
        H(2, 1, fg_erase);
        H(2, 2, fg_appear);
        H(3, 1, fg_erase);
        H(3, 2, fg_moving);
        H(3, 3, fg_hit);
        H(3, 4, fg_destroy);
        H(4, 1, fg_erase);
        H(4, 2, fg_dying);
        W(0, fg_walk);
        W(1, fg_turn);
        B(0, fg_squash);
        B(1, fg_fall);
        B(2, fg_gone);
    }
    return obj_dispatch(a, g);
}
