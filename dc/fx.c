/* fx.c - Mario's fireballs and the kill stars, ported from the main program:
 *   sm_f_ball     a fireball (gds->fireballs[2]); sfb_coll_det its collisions
 *   kill_star     the star shown where an enemy was beaten (gds->kill_stars[5])
 * Both are driven by the_game like the room's objects: an erase call (phase 1) before
 * the actors' erase pass and an update call (phase 2) after check_doors. States
 * (obj+0x20): 0 set up, 1 ready, 3 in use (mario / mario_coll_det set it), 4 ending.
 * Pictures (fx.frm slots): 1-6 fireball (sm_f_ball_set_0..5), 8 kill star.
 */
#include "world.h"

static const int16_t bounce_offsets[6] = { 2, 4, 8, -8, -4, -2 };   /* bounce_offsets */

static void fx_draw(world *w, int slot, int x, int y) {
    if (w->n_fx >= 8) return;
    w->fx[w->n_fx].obj = -1;
    w->fx[w->n_fx].frame = slot;
    w->fx[w->n_fx].x = x;
    w->fx[w->n_fx].y = y;
    w->n_fx++;
}

/* sm_f_ball */
static void fireball(world *w, hm_fx *f, int phase) {
    gds *g = &w->g;
    switch (f->state) {
    case 0:
        if (phase != 0 && phase != 2) return;
        f->state = 1;
        f->hit = 0;
        f->anim = 0;
        f->count = 1;
        f->bounce = 0;
        break;
    case 3:
        if (phase == 1) return;             /* (the erase: nothing to do) */
        if (phase == 2) {
            if (f->hit == 0) {
                if (--f->count < 1) {
                    f->count = 1;
                    if (++f->anim == 3) f->anim = 0;
                }
                f->y += bounce_offsets[f->bounce];
                f->bounce++;
                if (f->bounce == 3 && f->y < g->floor_y[f->floor] - 0xe) f->bounce = 2;
                if (f->y > g->floor_y[f->floor] - 0xe) {
                    f->y = (int16_t)(g->floor_y[f->floor] - 0xe);
                    f->bounce = 3;
                }
                if (f->bounce == 6) f->bounce = 0;
                if (f->dir == 1) { f->x -= 0xe; f->frame = (int16_t)(f->anim + 4); }
                else             { f->x += 0xe; f->frame = (int16_t)(f->anim + 1); }
                if (f->x > 0x15a) { f->x = 0x15a; f->state = 4; f->hit = 1; f->count = 1; }
                if (f->x < 0x12)  { f->x = 0x12;  f->state = 4; f->hit = 1; f->count = 1; }
                f->bx0 = (int16_t)(f->x + 1);
                f->bx1 = (int16_t)(f->x + 0xd);
                f->by0 = (int16_t)(f->y + 3);
                f->by1 = (int16_t)(f->y + 9);
            }
            fx_draw(w, f->frame, f->x, f->y);
            return;
        }
        /* phase 3: it hit something */
        f->state = 4;
        f->hit = 1;
        f->count = 1;
        return;
    case 4:
        if (phase != 1 || f->hit != 1) return;
        if (f->count-- == 0) {
            f->state = 0;
            f->hit = 0;
            f->phase = 2;
            f->dir = -1;
        }
        return;
    }
}

/* kill_star */
static void kill_star(world *w, hm_fx *s, int phase) {
    switch (s->state) {
    case 0:
        if (phase != 0 && phase != 2) return;
        s->state = 1;
        s->anim = 0;
        s->count = 0;
        break;
    case 3:
        if (phase != 2) return;
        fx_draw(w, s->anim + 8, s->x, s->y - 0xc);
        if (s->count-- < 1) {
            s->state = 4;
            s->count = 1;
        }
        return;
    case 4:
        if (phase != 1) return;
        if (s->count-- == 0) {
            s->state = 0;
            s->phase = 2;
        }
        return;
    }
}

void fx_start(world *w) {
    int i;
    for (i = 0; i < 2; i++) { w->fireball[i].state = 0; fireball(w, &w->fireball[i], 0); }
    for (i = 0; i < 5; i++) { w->kill_star[i].state = 0; kill_star(w, &w->kill_star[i], 0); }
    w->next_star = 0;
}

void fx_erase(world *w) {           /* the_game: fireballs, kill stars with phase 1 */
    int i;
    w->n_fx = 0;
    for (i = 0; i < 2; i++) fireball(w, &w->fireball[i], 1);
    for (i = 0; i < 5; i++) kill_star(w, &w->kill_star[i], 1);
}

/* sfb_coll_det: a fireball against the room's enemies */
static void sfb_coll_det(world *w, hm_fx *f) {
    gds *g = &w->g;
    int i;
    if (f->state != 3 && f->state != 4) return;
    stage_secret_fireball(w, f);
    for (i = 0; i < w->room.n_actors; i++) {
        actor_t *a = &w->room.actors[i];
        int touch;
        if (a->state != 3 || f->floor != a->cur_floor || a->type <= 9) continue;
        touch = f->bx0 < a->box_x1 && a->box_x0 < f->bx1 && f->by0 < a->box_y1 && a->box_y0 < f->by1;
        if (!touch && a->unk_03c != 0)
            touch = f->bx0 < a->hit_x1 && a->hit_x0 < f->bx1 && f->by0 < a->hit_y1 && a->hit_y0 < f->by1;
        if (!touch) continue;
        fireball(w, f, 3);
        hm_afx(5);
        a->cmd = 4;
        ((int (*)(actor_t *, gds *))a->handler)(a, g);
        g->mario->hit = 1;
        fx_kill_star(w, (((a->box_x0 + a->box_x1) >> 2) * 2) - 6, ((a->box_y0 + f->by1) >> 1) + 6);
    }
}

void fx_update(world *w) {          /* after check_doors: fireballs (+ collisions), kill stars */
    int i;
    for (i = 0; i < 2; i++) fireball(w, &w->fireball[i], 2);
    for (i = 0; i < 2; i++) sfb_coll_det(w, &w->fireball[i]);
    for (i = 0; i < 5; i++) kill_star(w, &w->kill_star[i], 2);
}

/* a kill star at (x, y) if the next one is free (mario_coll_det, sfb_coll_det) */
void fx_kill_star(world *w, int x, int y) {
    hm_fx *s = &w->kill_star[w->next_star];
    if (s->state != 1) return;
    s->state = 3;
    s->x = (int16_t)x;
    s->y = (int16_t)y;
    if (++w->next_star > 4) w->next_star = 0;
}

/* mario: throw a fireball if one is ready (returns 1 if thrown) */
int fx_throw(world *w, int x, int y, int floor, int dir) {
    int i;
    for (i = 0; i < 2; i++) {
        hm_fx *f = &w->fireball[i];
        if (f->state != 1) continue;
        f->state = 3;
        f->dir = (int16_t)dir;
        f->x = (int16_t)x;
        f->y = (int16_t)y;
        f->floor = (int16_t)floor;
        hm_afx(9);
        return 1;
    }
    return 0;
}
