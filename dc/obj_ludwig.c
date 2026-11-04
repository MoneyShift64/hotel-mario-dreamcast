/* obj_ludwig.c - the hotel 5 boss (type 0x36?), from L5_s10_sub.o (m_02a16 .. m_05736).
 *
 * He doesn't use doors: he appears at the left or right edge of a random floor (not
 * Mario's floor... unless it is, see m_02eca) and runs along it, hopping, faster (6) on
 * Mario's floor than elsewhere (2). Off Mario's floor he picks a "mode" for his attack:
 * 0 straight ahead (Mario on his floor), 1 up a floor, 2 down a floor; when Mario is in
 * that direction ahead of him he stops, winds up and fires his beam (ludwig_l, a chain
 * of type 0x37 actors). If he runs into Mario hiding in a door or riding a lift on his
 * floor (within 10 pixels) he pulls Mario out: Mario is frozen (group 7 state 1),
 * unk_53a = 2 and unk_530 = 1 (no lifts), then after the animation Mario is finished
 * (group 6 state 5, fell = 1). Stomping him stuns him for 20 ticks (flashing); a second
 * stomp while stunned, or a fireball/star, knocks him away (squashed, then gone) and he
 * comes back a little later (6 ticks first, then the delay shrinks by 2 to 2) on another
 * floor. He leaves by himself after running for 60 ticks. Beating the stage is the
 * normal "close every door" - he gives no points.
 *
 * Fields: unk_25c int32s: +0 mode (0/1/2), +4 hop bit pattern, +8 floor-line y, +0xc door
 * column (catching), +0x10 link to the first ludwig_l (actor index+1, 0 = none).
 * x_min: running time (and turn bookkeeping); x_max: 1 = hidden (inside the door/lift).
 * Frames: 1-5 dust under him; 6/7 run (mode 0), 0x2c/0x31 (mode 2), 0x36/0x3b (mode 1);
 * 8-10 turn; 0xb-0x16 catching; 0xf/0x10 stunned; 0x18-0x21 appearing / squashed;
 * 0x22-0x3f firing (per mode).
 */
#include "obj_common.h"

static actor_t *link_of(gds *g, int32_t v) { return v ? &g->room->actors[v - 1] : NULL; }

static void lw_draw(actor_t *a, gds *g) { draw_at_xy(a, g, a->frame); }      /* m_05682 */

/* the hop: y = line - 1..4, two bits of the pattern per tick (0x00afeafe) */
static void lw_hop(actor_t *a) {
    if (rd25c(a, 4) == 0) wr25c(a, 4, 0x00afeafe);
    a->y = (int2)((int16_t)rd25c(a, 8) - ((((uint16_t)rd25c(a, 4)) & 3) + 1));
    wr25c(a, 4, rd25c(a, 4) >> 2);
}

static void lw_run_frame(actor_t *a) {
    int32_t mode = rd25c(a, 0);
    if (a->dir == 1) a->frame = mode == 0 ? 7 : mode == 2 ? 0x31 : 0x3b;
    else             a->frame = mode == 0 ? 6 : mode == 2 ? 0x2c : 0x36;
}

static void lw_boxes(actor_t *a) {
    a->box_x0 = a->x + 10; a->box_x1 = a->x + 0x1e;
    a->box_y0 = a->y + 10; a->box_y1 = a->y + 0x24;
}

static void lw_place(actor_t *a, gds *g) {
    a->y = a->drawn_y0 = a->drawn_y1 = (int2)(g->floor_y[a->floor] - 0x24);
    a->cur_floor = a->floor;
    a->box_y0 = a->y + 10; a->box_y1 = a->y + 0x24;
    a->x = g->door_x[a->col];
    a->box_x1 = a->x + 0x1e; a->box_x0 = a->x + 10;
}

/* stop the beam: the first link goes from moving (3) to fading (4) */
static void lw_beam_off(actor_t *a, gds *g) {
    actor_t *l = link_of(g, rd25c(a, 0x10));
    if (l && l->state == 3) { l->state = 4; l->unk_02a = 0; }
}

static int lw_init(actor_t *a, gds *g) {                /* m_02a16 */
    int i;
    lw_place(a, g);
    a->state = 1;
    a->delay = 6;
    for (i = 0; i < 24 && g->room->actors[i].type != 0; i++) {
        if (g->room->actors[i].type == 0x37) {
            wr25c(a, 0x10, i + 1);
            g->room->actors[i].x_min = 1;
            g->room->actors[i].start_mode = a->start_mode;
            break;
        }
    }
    g->unk_530 = 0;
    a->unk_04a = -1; a->unk_048 = -1;
    return 0;
}

static int lw_comeback(actor_t *a, gds *g) {            /* m_02eca: state 0 - pick a new spot */
    if (a->timer++ > a->delay) {
        uint16_t f;
        a->timer = 0;
        if (a->delay > 2) a->delay -= 2;
        f = (uint16_t)(RND(g) & 3);
        if (g->mario->floor <= (int2)f) f++;
        a->floor = (int2)f;
        if (((g->unk_826 >> 2) & 1) == 0) { a->col = 6; a->dir = 1; }
        else { a->col = 0; a->dir = 0; }
        lw_place(a, g);
        a->state = 1;
        a->unk_04e = 0;
    }
    g->unk_530 = 0;
    return 0;
}

static int lw_wait(actor_t *a, gds *g) {                /* m_02ff6 */
    if (g->unk_53a == 0) {
        int mx = g->mario->x + 0x10, dx = g->door_x[a->col] + 0x10, d;
        if (dx < mx) { if (a->dir == -1) a->dir = 0; d = mx - dx; }
        else         { if (a->dir == -1) a->dir = 1; d = dx - mx; }
        if (d > 0x48 || a->floor != g->mario->floor) { a->state = 2; a->step = 0; a->unk_02a = 0; }
        wr25c(a, 8, a->y);
    }
    return 0;
}

static int lw_appear(actor_t *a, gds *g) {              /* m_030e6 */
    a->frame = (int2)((a->dir == 1) ? a->unk_02a + 0x1d : a->unk_02a + 0x18);
    if (++a->unk_02a < 5) {
        if (a->unk_02a == 1) g->unk_054 = 0xc;
    } else {
        a->unk_036 = 0; a->unk_02a = 0; a->unk_02e = 0; a->x_min = 0;
        wr25c(a, 0, 0);
        a->step = 0; a->state = 3;
        if (g->unk_53a != 0) { a->state = 4; a->step = 2; a->unk_02a = 0; a->unk_02c = 0; }
    }
    lw_draw(a, g);
    return 0;
}

static int lw_moving(actor_t *a, gds *g) {              /* m_031c4 */
    int r = CALL(a->walk_steps[a->step], a, g);
    if (a->x_max == 0) {
        lw_draw(a, g);
        if (a->cmd != 3 && a->cmd != 4) {           /* the dust under his feet */
            if (++a->unk_02e > 4) a->unk_02e = 0;
            obj_draw(a, a->unk_02e + 1, a->x, (int16_t)rd25c(a, 8));
        }
    }
    return r;
}

static int lw_hit(actor_t *a, gds *g) {                 /* m_05328: stomped */
    if (g->unk_496 == 0) {
        if (a->step == 3) {                         /* stomped again while stunned */
            a->state = 4; a->step = 2;
            a->box_y0 = a->y + 10;
            a->unk_02a = 0; a->unk_02c = 0;
            a->timer = -0x28;
        } else { a->step = 2; a->unk_02a = 0; }
    } else a->unk_04e = a->cmd;
    return 0;
}

static int lw_destroy(actor_t *a, gds *g) {             /* m_053e6: fireball / star */
    if (g->unk_496 == 0) {
        if (a->unk_04c != 0 || g->unk_53a != 0) a->unk_04c = 0;
        a->state = 4; a->step = 2; a->unk_02a = 0; a->unk_02c = 0;
        a->timer = -10;
    } else a->unk_04e = a->cmd;
    return 0;
}

static int lw_leaving(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }   /* m_0547c */

static void lw_turn_start(actor_t *a) { a->step = 1; a->unk_02a = 0; }

/* Mario on this floor: is he ahead of us? */
static int ahead(actor_t *a, mario_t *m) { return (a->dir == 0 && a->x < m->x) || (a->dir == 1 && m->x < a->x); }

static void lw_try_open(actor_t *a, gds *g) {           /* passing a closed door: 1 in 4 */
    uint16_t *door = DOOR_CELL(a->cur_floor, a->x + 0x14);
    if (door && (*door & 0xf000) == 0x1000 && (RND(g) & 3) == 1) {
        *door = (*door & 0xfff) | 0x2009;
        g->unk_054 = 2;
    }
}

static int lw_run_out(actor_t *a) {                     /* ran for 60 ticks: off he goes */
    if (a->x_min++ > 0x3b) { a->state = 4; a->step = 2; a->unk_02a = 0; a->unk_02c = 0; a->timer = 0; }
    return 0;
}

static int lw_run(actor_t *a, gds *g) {                 /* m_033a8 */
    mario_t *m = g->mario;
    int same = m->floor == a->cur_floor;
    if (a->unk_036 < (same ? 6 : 2)) a->unk_036 += 2;
    if (a->dir == 1) a->x -= a->unk_036; else a->x += a->unk_036;
    lw_run_frame(a);
    lw_hop(a);
    lw_boxes(a);
    if (same) {
        wr25c(a, 0, 0);
        a->x_min = 0;
        if ((a->x < m->x && a->dir == 1) || (m->x < a->x && a->dir == 0)) lw_turn_start(a);
    } else if (a->x_min == 5 || a->x_min == 0x14) {
        int32_t mode;
        if (a->cur_floor == 0) mode = 2;
        else if (a->cur_floor == 4) mode = 1;
        else if (a->x_min == 5) { mode = (RND(g) & 1) ? 1 : 2; }
        else mode = rd25c(a, 0) == 1 ? 2 : 1;
        wr25c(a, 0, mode);
    }
    switch (rd25c(a, 0)) {                          /* Mario in the line of fire: shoot */
    case 0: if (same && (m->state_group == 0 || m->state_group == 3) && ahead(a, m)) { a->step = 8; a->unk_02a = 0; a->x_min = 0; } break;
    case 1: if (m->floor < a->cur_floor && ahead(a, m)) { a->step = 8; a->unk_02a = 0; a->x_min = 0; } break;
    case 2: if (a->cur_floor < m->floor && ahead(a, m)) { a->step = 8; a->unk_02a = 0; a->x_min = 0; } break;
    }
    if (a->x >= 0x137) { a->x = 0x136; if (a->dir == 0) lw_turn_start(a); }
    else if (a->x < 0x22) { a->x = 0x22; if (a->dir == 1) lw_turn_start(a); }
    if (same) {
        if (!world_invulnerable(g) && abs(m->x - a->x) < 10) {                /* caught him hiding? */
            if ((m->state_group == 1 || m->state_group == 2) &&
                ((m->state == 1 && m->sub == 3) || m->state == 0 || (m->state == 2 && (m->sub == 5 || m->sub == 6)))) {
                uint16_t *cell = DOOR_CELL(m->floor, m->x + 0x10);
                if (cell && ((*cell & 0xf000) == 0xf000 || (*cell & 0xf000) == 0xe000)) {
                    m->state_group = 7; m->state = 1;
                    g->unk_53a = 2;
                    a->step = (*cell & 0xf000) == 0xf000 ? 5 : 6;
                    a->unk_02a = 0;
                    g->unk_530 = 1;
                }
            } else if (m->state_group == 4) {
                m->state_group = 7; m->state = 1;
                g->unk_53a = 2;
                a->step = 7; a->unk_02a = 0;
                g->unk_530 = 1;
            }
        }
    } else lw_try_open(a, g);
    return lw_run_out(a);
}

static int lw_caught(actor_t *a, gds *g) {              /* m_05104: step 4 - Mario's done for */
    a->frame = 9;
    if (++a->unk_02a > 8) {
        g->mario->state_group = 6; g->mario->state = 5;
        g->unk_53a = 0;
        g->mario->fell = 1;
        g->unk_530 = 0;
    }
    return 0;
}

static int lw_turn(actor_t *a, gds *g) {                /* m_0515c */
    (void)g;
    lw_hop(a);
    a->box_y0 = a->y + 10; a->box_y1 = a->y + 0x24;
    a->frame = (int2)((a->dir == 1) ? 10 - a->unk_02a : a->unk_02a + 8);
    if (++a->unk_02a > 2) {
        a->unk_036 = 0; a->unk_02a = 0; a->step = 0;
        a->x_min += 3;
        a->dir = (a->dir == 0) ? 1 : 0;
    }
    return 0;
}

static int lw_stunned(actor_t *a, gds *g) {             /* m_05242: step 2 */
    lw_beam_off(a, g);
    a->frame = 0xf;
    lw_draw(a, g);
    a->step = 3;
    a->box_y0 += 8;
    a->x_min = 0;
    return 0;
}

static int lw_dizzy(actor_t *a, gds *g) {               /* m_052a8: step 3, 20 ticks */
    (void)g;
    if (++a->unk_02a % 3 == 0) a->x_min = (a->x_min == 0);
    a->frame = (int2)(a->x_min + 0xf);
    if (a->unk_02a > 0x13) { a->step = 0; a->box_y0 = a->y + 10; a->unk_02a = 0; a->unk_02c = 0; }
    return 0;
}

/* steps 5, 6, 7: pulling Mario out of an up lift / a down lift / a door. Each tick n of
 * the table; the door cell is redrawn (m_0583e) on the marked ticks - the port draws the
 * door tiles every frame anyway. x_max = 1 while he's out of sight. */
static void lw_catch_start(actor_t *a, gds *g) {        /* tick 0 of all three */
    int c = a->x + 0x14;
    c = c < 0x58 ? 0 : c < 0x84 ? 1 : c < 0xb0 ? 2 : c < 0xdc ? 3 : c < 0x108 ? 4 : c < 0x134 ? 5 : 6;   /* m_05736 */
    wr25c(a, 0xc, c);
    a->x = (int2)(g->door_x[c] - 4);
    a->frame = (a->dir == 1) ? 0x12 : 0x11;
}

static int lw_catch_lift(actor_t *a, gds *g) {          /* m_04982 / m_04c28 (the same) */
    int n = a->unk_02a++;
    switch (n) {
    case 0: lw_catch_start(a, g); break;
    case 1: a->frame = 0x13; break;
    case 2: a->frame = 0x14; break;
    case 3: case 4: case 5: a->frame++; break;
    case 6: a->x_max = 1; break;
    case 0x11: a->x_max = 0; a->frame = 0xd; break;
    case 0x12: a->frame = 0xe; break;
    case 0x13: a->frame = 0xb; break;
    case 0x14: a->frame++; break;
    case 0x15: a->frame = 9; break;
    case 0x17: a->unk_02a = 0; a->step = 4; break;
    }
    return 0;
}

static int lw_catch_door(actor_t *a, gds *g) {          /* m_04ed2 */
    int n = a->unk_02a++;
    switch (n) {
    case 0: lw_catch_start(a, g); break;
    case 1: a->frame = 0x13; break;
    case 2: a->frame = 0x14; break;
    case 3: a->frame++; break;
    case 4: a->x_max = 1; break;
    case 5: g->unk_054 = 1; break;
    case 0x12: a->x_max = 0; a->frame = 0xb; g->unk_054 = 2; break;
    case 0x13: a->frame = 0xc; break;
    case 0x14: a->frame = 9; break;
    case 0x15: a->unk_02a = 0; a->step = 4; break;
    }
    return 0;
}

static int lw_skid(actor_t *a, gds *g) {                /* m_03b70: step 8 - braking to shoot */
    mario_t *m = g->mario;
    int32_t mode;
    if (a->dir == 1) a->x -= a->unk_036; else a->x += a->unk_036;
    lw_run_frame(a);
    lw_hop(a);
    lw_boxes(a);
    mode = rd25c(a, 0);
    if (mode == 0) {
        a->x_min = 0; a->unk_036 = 0;
        if ((m->state_group == 0 || m->state_group == 3) && m->floor == a->cur_floor &&
            (a->dir != 0 || a->x <= m->x) && (a->dir != 1 || m->x <= a->x)) { a->step = 9; a->unk_02a = 0; }
        else { a->step = 0; a->unk_02a = 0; }
    } else if (mode == 1 || mode == 2) {
        /* where the beam would cross Mario's floor (40 per floor, plus a lead if he walks) */
        int fl = (mode == 1) ? a->cur_floor - m->floor : m->floor - a->cur_floor, d = 0x28 * fl;
        if (m->state_group == 0 && (m->state == 0 || m->state == 1)) {
            int lead = 0xc * ((a->dir == 0) ? (m->floor + 5) - a->cur_floor : (m->floor - 5) - a->cur_floor);
            d = (m->state == 0) ? d - lead : d + lead;
        }
        d = (int16_t)d;
        if ((mode == 1) ? (m->floor < a->cur_floor) : (a->cur_floor < m->floor)) {
            if ((a->dir == 0 && d + a->x + 0x10 < m->x) || (a->dir == 1 && m->x < (a->x - d) - 0x10))
                a->unk_036 = 4;
            else if ((a->dir == 0 && m->x < d + a->x - 0x10) || (a->dir == 1 && (a->x - d) + 0x10 < m->x)) {
                if ((a->dir == 0 && m->x < a->x && (m->state & 1) != 0) || (a->dir == 1 && a->x < m->x && (m->state & 1) == 0))
                    lw_turn_start(a);
                else a->unk_036 = -4;
            } else { a->step = 9; a->unk_02a = 0; }
        } else { a->step = 0; a->unk_02a = 0; }
    }
    if (a->x >= 0x137) { a->x = 0x136; a->x_min += 2; }
    else if (a->x < 0x22) { a->x = 0x22; a->x_min += 2; }
    lw_try_open(a, g);
    return lw_run_out(a);
}

static int fire_base(actor_t *a) {                      /* first firing frame for the mode */
    int32_t mode = rd25c(a, 0);
    if (mode == 0) return a->dir == 1 ? 0x27 : 0x22;
    if (mode == 1) return a->dir == 1 ? 0x3b : 0x36;
    return a->dir == 1 ? 0x31 : 0x2c;
}

static int lw_windup(actor_t *a, gds *g) {              /* m_04430: step 9 */
    a->frame = (int2)(fire_base(a) + a->unk_02a);
    if (++a->unk_02a < 5) { if (a->unk_02a == 4) g->unk_054 = 0xf; }
    else { a->step = 10; a->unk_02a = 0; }
    return 0;
}

static int lw_fire(actor_t *a, gds *g) {                /* m_04584: step 10 - the beam starts */
    actor_t *l;
    a->unk_02a++;
    a->step = 0xb; a->unk_02a = 0;
    l = link_of(g, rd25c(a, 0x10));
    if (l && l->state == 0) {
        int32_t mode = rd25c(a, 0);
        if (mode == 0) { l->y = a->y + 8; l->x = (a->dir == 1) ? a->x - 0x20 : a->x + 0x20; }
        else if (mode == 1) { l->y = a->y - 0x15; l->x = (a->dir == 1) ? a->x - 0x1c : a->x + 0x1c; }
        else if (mode == 2) { l->y = a->y + 0x21; l->x = (a->dir == 1) ? a->x - 0x1c : a->x + 0x1c; }
        l->dir = a->dir;
        l->x_max = mode;
        l->state = 3;
    }
    return 0;
}

static int lw_hold(actor_t *a, gds *g) {                /* m_04702: step 11, 9 ticks */
    a->frame = (int2)(fire_base(a) + 4);
    if (++a->unk_02a > 8) { a->step = 0xc; a->unk_02a = 0; lw_beam_off(a, g); }
    return 0;
}

static int lw_wind_down(actor_t *a, gds *g) {           /* m_0484a: step 12 */
    (void)g;
    a->frame = (int2)(fire_base(a) + 4 - a->unk_02a);
    if (++a->unk_02a > 4) { a->step = 8; a->unk_02a = 0; a->x_min += 0x10; }
    return 0;
}

static int lw_gone(actor_t *a, gds *g) {                /* m_05582: steps_b[1] */
    (void)g;
    if (a->unk_02a-- < 2) a->state = 0;
    return 0;
}

static int lw_squash(actor_t *a, gds *g) {              /* m_054b6: steps_b[2] */
    lw_beam_off(a, g);
    a->y = (int16_t)rd25c(a, 8);
    a->frame = (int2)((a->dir == 1) ? 0x21 - a->unk_02a : 0x1c - a->unk_02a);
    lw_draw(a, g);
    if (++a->unk_02a < 5) { if (a->unk_02a == 1) g->unk_054 = 0xd; }
    else { a->step = 1; a->unk_02a = 2; }
    return 0;
}

int ludwig(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, lw_init);
        H(0, 2, lw_comeback);
        H(1, 2, lw_wait);
        H(2, 1, obj_erase);
        H(2, 2, lw_appear);
        H(3, 1, obj_erase);
        H(3, 2, lw_moving);
        H(3, 3, lw_hit);
        H(3, 4, lw_destroy);
        H(4, 1, obj_erase);
        H(4, 2, lw_leaving);
        W(0, lw_run);
        W(1, lw_turn);
        W(2, lw_stunned);
        W(3, lw_dizzy);
        W(4, lw_caught);
        W(5, lw_catch_lift);
        W(6, lw_catch_lift);
        W(7, lw_catch_door);
        W(8, lw_skid);
        W(9, lw_windup);
        W(10, lw_fire);
        W(11, lw_hold);
        W(12, lw_wind_down);
        B(1, lw_gone);
        B(2, lw_squash);
    }
    return obj_dispatch(a, g);
}
