/* obj_morton.c - "morton", the hotel 1 boss (stage 10), from L1_s10_sub.o (m_029da .. m_05608).
 *
 * The boss stage is an ordinary "close every door" stage; Morton just chases Mario:
 *  - He comes out of a door once Mario is more than 72 away or on another floor.
 *  - Walks fast (6 a tick, 8 while following Mario into a lift). Every 20 ticks he picks a
 *    target: an up lift if Mario is higher, a down lift if lower, else Mario himself.
 *    If Mario gets into a lift on Morton's floor, Morton heads for that lift.
 *  - At the lift he rides it (step 4, 23 ticks, lift-door tiles drawn from the door
 *    sheet, both lift cells marked busy 0xc000/0xd000 meanwhile; g->unk_530 = 1 stops the
 *    elevator switch). If Mario is still standing in that lift, he is caught: Mario goes
 *    to group 7 state 1, unk_53a = 2; after the ride Morton laughs (step 3) and then sets
 *    Mario to group 6 state 5 with fell = 1 and unk_53a = 0, so exit_check ends the stage
 *    as "Mario died".
 *  - A stomp knocks him dizzy for 20 ticks (step 2); stomping him while dizzy, or a
 *    fireball / star at any time but in a lift, sends him falling off the bottom of the
 *    screen. No points. He comes back out of a door near Mario after 'delay' ticks
 *    (35, 10 less each time, down to 15). He can't be beaten for good.
 * unk_25c (int32s): +0 target (0 none, 1 Mario, 0xe000/0xf000 lift type), +4 target x,
 *   +8 target lift column, +0xc target lift cell (index+1 here; a pointer in the original),
 *   +0x10 "hidden this tick", +0x14 extra speed.
 * x_max: retarget countdown (dizzy: flash toggle); x_min: 1 = caught Mario in the lift.
 * Frames: 1-10 walk, 11-16 turn, 22-23 out of the door, 24-25 dizzy, 17-21 falling,
 * 26-32 lift, 33-39 laughing, 43-45 lift.
 */
#include "obj_common.h"

static void draw_frame(actor_t *a, gds *g) { draw_at_xy(a, g, a->frame); }

static uint16_t *cell_ref(gds *g, int32_t idx) {
    return idx > 0 ? &((uint16_t *)(g->stage_info + 0x12))[idx - 1] : NULL;
}
static int32_t ref_of(gds *g, uint16_t *c) {
    return c ? (int32_t)(c - (uint16_t *)(g->stage_info + 0x12)) + 1 : 0;
}

/* m_054a0: a door-sheet tile over the lift Morton is in */
static void lift_tile(actor_t *a, int sx, int sy) {
    int col = rd25c(a, 8), f = a->cur_floor;
    if (f < 0 || f >= FLOORS || col < 0 || col >= COLS) return;
    hm_world->tile[f][col].on = 1;
    hm_world->tile[f][col].sx = (int16_t)sx;
    hm_world->tile[f][col].sy = (int16_t)sy;
}
/* the down lift (0xf000) and up lift (0xe000) versions of a tile */
static void lift_tile2(actor_t *a, int sx_down, int sy_down, int sx_up, int sy_up) {
    if (rd25c(a, 0) == 0xf000) lift_tile(a, sx_down, sy_down);
    else lift_tile(a, sx_up, sy_up);
}

static void unbusy(uint16_t *c) {
    if (!c) return;
    if ((*c & 0xf000) == 0xd000) *c = (*c & 0xfff) | 0xf000;
    else if ((*c & 0xf000) == 0xc000) *c = (*c & 0xfff) | 0xe000;
}
static int busy(uint16_t *c) {
    if ((*c & 0xf000) == 0xf000) { *c = (*c & 0xfff) | 0xd000; return 1; }
    if ((*c & 0xf000) == 0xe000) { *c = (*c & 0xfff) | 0xc000; return 1; }
    return 0;
}
/* both lift cells back to normal (after a ride, or knocked out of one) */
static void free_lifts(actor_t *a, gds *g) {
    unbusy(cell_ref(g, rd25c(a, 0xc)));
    if (a->cur_floor >= 0 && a->cur_floor < FLOORS && rd25c(a, 8) >= 0 && rd25c(a, 8) < COLS)
        unbusy(&CELL(g, a->cur_floor, rd25c(a, 8)));
    wr25c(a, 0, 0);
    a->unk_02a = 0;
    wr25c(a, 0x14, 0);
    g->unk_530 = 0;
}

static void start_fall(actor_t *a) { a->state = 4; a->step = 4; a->unk_02a = 0x11; a->unk_02c = 0; }

static int is_door(uint16_t v) { v &= 0xf000; return v == 0x2000 || v == 0x1000; }

/* m_05080: pick the door (floor, col) to come back out of, near Mario */
static void pick_door(actor_t *a, gds *g) {
    mario_t *m = g->mario;
    int mf = m->floor, sel_c = 0, sel_f = 0, pass, f, c, found;
    for (pass = 0; pass < 2; pass++) {
        found = 0;
        for (f = mf - pass; f >= 0 && !found; f--)
            for (c = 0; c < COLS; c++)
                if (is_door(CELL(g, f, c))) { found = 1; sel_c = c; sel_f = f; break; }
        if (!found) sel_f = -10;
        found = 0;
        for (f = mf + 1; f <= 4 && !(mf - sel_f <= f - mf) && !found; f++)
            for (c = 0; c < COLS; c++)
                if (is_door(CELL(g, f, c))) { found = 1; sel_c = c; sel_f = f; break; }
        if (sel_f >= 0 && sel_f < FLOORS)
            for (c = sel_c + 1; c < COLS; c++) {
                if (!is_door(CELL(g, sel_f, c))) continue;
                {
                    int d1 = abs(m->x - g->door_x[sel_c]), d2 = abs(m->x - g->door_x[c]);
                    if ((d2 < d1 && (d2 > 0x54 || sel_f != mf)) || (d1 < 0x54 && sel_f == mf)) sel_c = c;
                }
            }
        if (abs(m->x - g->door_x[sel_c]) > 0x54 || sel_f != mf) break;
    }
    if (sel_f >= 0 && sel_f < 5 && sel_c >= 0 && sel_c < 7) { a->floor = (int2)sel_f; a->col = (int2)sel_c; }
}

static void place(actor_t *a, gds *g) {
    a->x = a->drawn_x0 = a->drawn_x1 = g->door_x[a->col];
    a->y = a->drawn_y0 = a->drawn_y1 = (int2)(g->floor_y[a->floor] - 0x20);
    a->cur_floor = a->floor;
}

static int mo_init(actor_t *a, gds *g) {               /* m_029da */
    place(a, g);
    a->state = 1;
    a->delay = 0x23;
    g->unk_530 = 0;
    a->x_max = 0x1e;
    a->unk_04a = -1; a->unk_048 = -1;
    return 0;
}

static int mo_respawn(actor_t *a, gds *g) {            /* m_02ce4 */
    if (a->timer++ > a->delay) {
        pick_door(a, g);
        place(a, g);
        a->hit_x0 = a->hit_x1 = a->hit_y0 = a->hit_y1 = 0;
        a->box_x0 = a->box_x1 = a->box_y0 = a->box_y1 = 0;
        a->timer = 0;
        if (a->delay > 0xf) a->delay -= 10;
        g->unk_530 = 0;
        a->unk_04e = 0;
        a->state = 1;
    }
    return 0;
}

static int mo_wait(actor_t *a, gds *g) {               /* m_02dba */
    if (g->unk_53a == 0 &&
        (door_far_from_mario(a, g, a->x + 0x10, 0x10) > 0x48 || a->floor != g->mario->floor)) {
        uint16_t *c = &CELL(g, a->floor, a->col);
        if ((*c & 0xf000) != 0x2000) { *c = (*c & 0xfff) | 0x2009; g->unk_054 = 2; }
        a->state = 2; a->step = 0;
        wr25c(a, 0x14, 0);
    }
    return 0;
}

static int mo_emerge(actor_t *a, gds *g) {             /* m_02ee0 */
    switch (a->step) {
    case 0: CELL(g, a->floor, a->col) |= 4; a->step = 1; break;
    case 1: a->frame = 0x16; a->step = 2; break;
    case 2: a->frame = 0x17; a->step = 3; break;
    case 3:
        CELL(g, a->floor, a->col) = (CELL(g, a->floor, a->col) & ~4) | 2;
        a->frame = (a->dir == 1) ? 0xd : 0x10;
        if (g->unk_53a == 0) { a->unk_02a = 1; a->step = 0; a->state = 3; wr25c(a, 0x14, 0); }
        else { a->unk_02a = 0x11; a->unk_02c = 0; a->state = 4; a->step = 4; }
        break;
    }
    if (a->step != 1) draw_frame(a, g);
    return 0;
}

static int mo_moving(actor_t *a, gds *g) {             /* m_030c8 */
    int r = CALL(a->walk_steps[a->step], a, g);
    if (rd25c(a, 0x10) == 0) draw_frame(a, g);
    else wr25c(a, 0x10, 0);
    return r;
}

static void turn_round(actor_t *a) { a->step = 1; a->unk_02a = 0xb; a->unk_02c = 1; }

static int mo_walk(actor_t *a, gds *g) {               /* m_031c2 */
    mario_t *m = g->mario;
    int spd, c;
    uint16_t *here;
    if (g->unk_53a != 0) { start_fall(a); return 0; }
    if (--a->unk_02c < 1) { a->unk_02c = 1; walk_cycle(a); }
    spd = (int16_t)rd25c(a, 0x14) + 6;
    if (a->dir == 1) { a->x -= spd; a->frame = (int2)(a->unk_02a + 6); }
    else             { a->x += spd; a->frame = (int2)(a->unk_02a + 1); }
    if (a->x_max-- < 2) {                               /* pick a target */
        a->x_max = 0x14;
        wr25c(a, 0, 0);
        if (a->cur_floor < m->floor || m->floor < a->cur_floor) {
            uint16_t want = a->cur_floor < m->floor ? 0xe000 : 0xf000;
            for (c = 0; c < COLS; c++)
                if ((CELL(g, a->cur_floor, c) & 0xf000) == want) {
                    wr25c(a, 0, want);
                    wr25c(a, 4, g->door_x[c]);
                    wr25c(a, 8, c);
                    wr25c(a, 0xc, ref_of(g, &CELL(g, a->cur_floor, c)));
                    break;
                }
        } else {
            wr25c(a, 0, 1);
            wr25c(a, 4, m->x);
        }
        if (rd25c(a, 0) != 0) {
            if (rd25c(a, 4) < a->x) { if (a->dir == 0) turn_round(a); }
            else if (a->x < rd25c(a, 4) && a->dir == 1) turn_round(a);
        }
    }
    if (rd25c(a, 0) == 1 && (m->state_group == 1 || m->state_group == 2) && a->cur_floor == m->floor) {
        uint16_t *mc = DOOR_CELL(m->floor, m->x + 0x10);  /* follow Mario into his lift */
        if (mc) {
            wr25c(a, 0, *mc & 0xf000);
            wr25c(a, 0xc, ref_of(g, mc));
            for (c = 0; c < COLS; c++)
                if (&CELL(g, a->cur_floor, c) == mc) {
                    wr25c(a, 8, c);
                    wr25c(a, 4, g->door_x[c]);
                    wr25c(a, 0x14, 2);
                    break;
                }
        }
    }
    if (rd25c(a, 0) == 0xf000 || rd25c(a, 0) == 0xe000) {
        here = DOOR_CELL(a->cur_floor, a->x + 0x10);
        if (here && here == cell_ref(g, rd25c(a, 0xc)) && (int32_t)(*here & 0xf000) == rd25c(a, 0)) {
            a->step = 4; g->unk_530 = 1; a->unk_02a = 0;    /* into the lift */
        }
    }
    if (a->x >= 0x13f) { a->x = 0x13e; turn_round(a); }
    else if (a->x < 0x22) { a->x = 0x22; turn_round(a); }
    a->box_y0 = a->y + 6; a->box_y1 = a->y + 0x14;
    a->hit_y0 = a->y + 0xf; a->hit_y1 = a->y + 0x1f;
    if (a->dir == 1) { a->box_x0 = a->x + 6; a->box_x1 = a->x + 0x13; a->hit_x0 = a->x + 0xd; a->hit_x1 = a->x + 0x18; }
    else { a->box_x0 = a->x + 0xd; a->box_x1 = a->x + 0x1a; a->hit_x0 = a->x + 8; a->hit_x1 = a->x + 0x13; }
    a->unk_03c = 1;
    maybe_open_door(a, g, a->x + 0x10, 7);
    return 0;
}

static void lift_boxes(actor_t *a) {
    a->box_x0 = a->x + 0xb; a->box_x1 = a->x + 0x16;
    a->box_y0 = a->y + 5;   a->box_y1 = a->y + 0x1f;
    a->hit_x0 = a->x + 7;   a->hit_x1 = a->x + 0x1a;
    a->hit_y0 = a->y + 0xc; a->hit_y1 = a->y + 0x17;
    a->unk_03c = 1;
}
static void hide(actor_t *a) { wr25c(a, 0x10, 1); }

static int mo_lift(actor_t *a, gds *g) {               /* m_037ca: the lift ride, 23 ticks */
    int caught = a->x_min != 0;
    mario_t *m = g->mario;
    switch (a->unk_02a++) {
    case 0:
        a->x = (int2)rd25c(a, 4);
        lift_boxes(a);
        a->frame = (a->dir == 1) ? 0x1b : 0x1a;
        break;
    case 1: {
        uint16_t *src = cell_ref(g, rd25c(a, 0xc)), *mc = DOOR_CELL(m->floor, m->x + 0x10), *dst;
        a->frame = 0x1c;
        if (!src) break;
        if (!world_invulnerable(g) && mc == src && (m->state_group == 1 || m->state_group == 2)) {
            if ((m->state == 1 && m->sub == 3) || m->state == 0 || (m->state == 2 && (m->sub == 5 || m->sub == 6))) {
                m->state_group = 7; m->state = 1;            /* caught */
                g->unk_53a = 2;
                a->x_min = 1;
            } else a->unk_02a--;                             /* wait for him */
            break;
        }
        dst = &CELL(g, (*src >> 8) & 0xf, (*src >> 4) & 0xf);
        if (mc == dst && (m->state_group == 1 || m->state_group == 2)) { a->unk_02a--; break; }
        a->x_min = 0;
        if (!busy(dst)) break;
        busy(src);
        break;
    }
    case 2:
        if (caught) lift_tile2(a, 0x66, 0x84, 0x132, 0x84);
        else lift_tile2(a, 0x22, 0x21, 0xee, 0x21);
        break;
    case 3: if (!caught) lift_tile2(a, 0x44, 0x21, 0x110, 0x21); break;
    case 4: if (!caught) lift_tile2(a, 0x66, 0x21, 0x132, 0x21); break;
    case 5: case 6: a->frame++; break;
    case 7:
        a->hit_x0 = a->hit_x1 = a->hit_y0 = a->hit_y1 = 0;
        a->box_x0 = a->box_x1 = a->box_y0 = a->box_y1 = 0;
        a->frame++;
        if (caught) lift_tile2(a, 0x44, 0x84, 0x110, 0x84);
        else lift_tile2(a, 0x44, 0x21, 0x110, 0x21);
        break;
    case 8:
        a->frame++;
        if (caught) lift_tile2(a, 0x22, 0x84, 0xee, 0x84);
        else lift_tile2(a, 0x22, 0x21, 0xee, 0x21);
        break;
    case 9:  hide(a); lift_tile2(a, 0, 0x84, 0xcc, 0x84); break;
    case 10: hide(a); lift_tile2(a, 0xaa, 0x21, 0xaa, 0x63); g->unk_054 = 3; break;
    case 11: hide(a); lift_tile2(a, 0xaa, 0x42, 0xaa, 0x84); break;
    case 12: hide(a); lift_tile2(a, 0, 0x21, 0xcc, 0x21); break;
    case 13: {                                          /* arrive at the other floor */
        uint16_t *src = cell_ref(g, rd25c(a, 0xc));
        int nf = src ? (*src >> 8) & 0xf : a->cur_floor, nc = src ? (*src >> 4) & 0xf : rd25c(a, 8);
        hide(a);
        if (nf >= FLOORS || nc >= COLS) break;
        a->cur_floor = (int2)nf;
        a->y = (int2)(g->floor_y[nf] - 0x20);
        a->x = g->door_x[nc];
        wr25c(a, 8, nc);
        break;
    }
    case 14: hide(a); lift_tile2(a, 0xaa, 0x84, 0xaa, 0x42); break;
    case 15: hide(a); lift_tile2(a, 0xaa, 0x63, 0xaa, 0x21); break;
    case 16: a->frame = 0x2d; lift_tile2(a, 0xcc, 0x84, 0, 0x84); break;
    case 17: a->frame = 0x2b; lift_tile2(a, 0xee, 0x21, 0x22, 0x21); break;
    case 18: a->frame = 0x16; lift_tile2(a, 0x110, 0x21, 0x44, 0x21); break;
    case 19: a->frame++; lift_boxes(a); lift_tile2(a, 0x132, 0x21, 0x66, 0x21); break;
    case 20: lift_tile2(a, 0x110, 0x21, 0x44, 0x21); break;
    case 21: a->frame = (a->dir == 1) ? 0xd : 0xb; lift_tile2(a, 0xee, 0x21, 0x22, 0x21); break;
    case 22:
        a->frame = (a->dir == 1) ? 8 : 3;
        lift_tile2(a, 0xcc, 0x21, 0, 0x21);
        free_lifts(a, g);
        a->step = (a->x_min == 1) ? 3 : 0;
        break;
    }
    return 0;
}

static int mo_laugh(actor_t *a, gds *g) {              /* m_04572: caught Mario */
    if (a->unk_02a < 10) {
        a->frame = (int2)(a->unk_02a + 0x21);
        if (a->frame == 0x24) g->unk_054 = 0xb;
    } else if (a->unk_02a == 10) a->frame = 0x27;
    else if (a->unk_02a == 0xb) a->frame = 0x26;
    else if (a->unk_02a > 0x14) {
        g->mario->state_group = 6; g->mario->state = 5;
        g->unk_53a = 0;
        g->mario->fell = 1;
    }
    a->unk_02a++;
    return 0;
}

static int mo_turn(actor_t *a, gds *g) {               /* m_04642 */
    (void)g;
    lift_boxes(a);
    if (--a->unk_02c == 0) { a->unk_02c = 1; a->unk_02a++; }
    a->frame = (int2)((a->dir == 1) ? a->unk_02a + 3 : a->unk_02a);
    if (a->unk_02a > 0xd) {
        a->unk_02a = 0;
        if (a->dir == 0) { a->frame = 6; a->dir = 1; }
        else { a->frame = 1; a->dir = 0; }
        a->step = 0;
    }
    return 0;
}

static int mo_dizzy(actor_t *a, gds *g) {              /* m_0474a */
    a->unk_03c = 0;
    a->box_x0 = a->x + 0xb; a->box_x1 = a->x + 0x16;
    a->box_y0 = a->y + 0xf; a->box_y1 = a->y + 0x1f;
    if (g->unk_53a != 0) { start_fall(a); return 0; }
    if (++a->unk_02a % 3 == 0) a->x_max = (a->x_max == 0);
    a->frame = (int2)(a->x_max + 0x18);
    if (a->unk_02a > 0x13) { a->step = 0; a->unk_02a = 0; a->unk_02c = 0; wr25c(a, 0x14, 0); }
    return 0;
}

static int mo_hit(actor_t *a, gds *g) {                /* m_04822: stomped */
    if (g->unk_496 != 0) { a->unk_04e = a->cmd; return 0; }
    if (a->step == 2) start_fall(a);
    else {
        if (a->step == 4) free_lifts(a, g);
        a->state = 4; a->step = 9; a->unk_02a = 0;
    }
    return 0;
}

static int mo_destroy(actor_t *a, gds *g) {            /* m_049ba: fireball or star */
    if (g->unk_496 != 0) { a->unk_04e = a->cmd; return 0; }
    if (a->step != 4) {
        if (a->unk_04c != 0 || g->unk_53a != 0) a->unk_04c = 0;
        start_fall(a);
    }
    return 0;
}

static int mo_dying(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }

static int mo_squash(actor_t *a, gds *g) {             /* m_04a98: squashed, then dizzy */
    a->frame = 0x2c;
    draw_frame(a, g);
    if (++a->unk_02a > 1) {
        a->state = 3; a->step = 2; a->x_max = 0;
        wr25c(a, 0x14, 0);
    }
    return 0;
}

static int mo_gone(actor_t *a, gds *g) {               /* m_04f88 */
    (void)g;
    if (a->unk_02a-- < 2) { a->state = 0; a->cmd = 2; a->timer = 0; a->dir = -1; }
    return 0;
}

static int mo_fall0(actor_t *a, gds *g) {              /* m_04b60 */
    a->frame = a->unk_02a; draw_frame(a, g);
    a->unk_02a++; a->step = 5;
    return 0;
}
static int mo_fall1(actor_t *a, gds *g) {              /* m_04c12 */
    a->frame = a->unk_02a; draw_frame(a, g);
    a->unk_02a++; a->step = 6;
    return 0;
}
/* m_04cb8 / m_04da8 / m_04e98: drop 16 a tick, cycling frames 19-21 */
static void fall_step(actor_t *a, gds *g, int next, int adv) {
    a->y += 0x10;
    a->frame = a->unk_02a; draw_frame(a, g);
    if (++a->unk_02c > 0) { a->unk_02c = 0; a->unk_02a += adv; a->step = next; }
    if (a->y > 0xd1) { a->step = 3; a->unk_02a = 2; }
}
static int mo_fall2(actor_t *a, gds *g) { fall_step(a, g, 7, 1); return 0; }
static int mo_fall3(actor_t *a, gds *g) { fall_step(a, g, 8, 1); return 0; }
static int mo_fall4(actor_t *a, gds *g) { fall_step(a, g, 6, -2); return 0; }

int morton(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, mo_init);
        H(0, 2, mo_respawn);
        H(1, 2, mo_wait);
        H(2, 1, obj_erase);
        H(2, 2, mo_emerge);
        H(3, 1, obj_erase);
        H(3, 2, mo_moving);
        H(3, 3, mo_hit);
        H(3, 4, mo_destroy);
        H(4, 1, obj_erase);
        H(4, 2, mo_dying);
        W(0, mo_walk);
        W(1, mo_turn);
        W(2, mo_dizzy);
        W(3, mo_laugh);
        W(4, mo_lift);
        B(3, mo_gone);
        B(4, mo_fall0);
        B(5, mo_fall1);
        B(6, mo_fall2);
        B(7, mo_fall3);
        B(8, mo_fall4);
        B(9, mo_squash);
    }
    return obj_dispatch(a, g);
}
