/* Hidden item/warp rules from eegg, sfb_eegg_det, jump_coll_det, room_coll_det
 * and eegg_coll_det. Native cell indices replace CD-i pointers. */
#include "hm_rules.h"
#include "world.h"
#include <string.h>
static void message(actor_t *a, gds *g, int cmd) {
    int old = a->cmd;
    a->cmd = cmd;
    ((int (*)(actor_t *, gds *))a->handler)(a, g);
    a->cmd = old;
}
static void items(world *w) {
    gds *g = &w->g;
    mario_t *m = &w->mario;
    player_t *p = &g->player[g->cur_player];
    int item = g->item_hit;
    if (!item)
        return;
    switch (item) {
    case 1:
    case 5:
        m->star_timer = item == 1 ? 1000 : 7200;
        m->item_flash = 36;
        m->invincible = 1;
        m->unk_146 = 0;
        hm_afx(11);
        break;
    case 2:
        if (p->lives < 99)
            p->lives++;
        hm_afx(8);
        break;
    case 3:
    case 7:
        p->power = item == 7 ? 1 : 2;
        m->item_flash = 36;
        p->unk_0c = 1;
        hm_afx(11);
        break;
    case 4:
        p->coins++;
        hm_afx(6);
        break;
    case 10:
        for (int i = 0; i < w->room.n_actors; i++)
            if (w->room.actors[i].type > 9)
                message(&w->room.actors[i], g, 4);
        hm_afx(8);
        break;
    case 11:
    case 12:
        for (int i = 0; i < w->room.n_actors; i++) {
            actor_t *a = &w->room.actors[i];
            if (a->type <= 9 || a->type == 0x2f || a->type == 0x2a || a->type == 0x32 ||
                a->type == 0x1d)
                continue;
            int protected = (a->unk_018 && a->state == 3) || a->type == 0x29 || a->type == 0x15 ||
                            (a->type == 0x20 && a->state == 3 &&
                             (a->step == 3 || a->step == 4 || a->step == 5));
            if (protected) {
                a->appear_count = 0;
                message(a, g, 4);
                continue;
            }
            if (a->state != 3)
                continue;
            int x = a->x, floor = a->cur_floor;
            memset(a, 0, sizeof *a);
            a->x = x;
            a->cur_floor = floor;
            int obj = item == 12 ? OBJ_power : (p->hotel == 7 ? OBJ_elvis_y : OBJ_coin);
            a->type = item == 12 ? 2 : 4;
            w->obj_of[i] = obj;
            a->handler = (code *)(item == 12 ? power : (p->hotel == 7 ? elvis_y : coin));
            ((int (*)(actor_t *, gds *))a->handler)(a, g);
        }
        hm_afx(8);
        break;
    }
    g->item_hit = 0;
    g->unk_532 = 1;
}
static void collect(world *w, int cooldown) {
    player_t *p = &w->g.player[w->g.cur_player];
    if (w->secret.remaining <= 0 || w->secret.cooldown >= 0)
        return;
    w->secret.cooldown = cooldown;
    if (w->secret.kind != 4 || (w->secret.toggle ^= 1))
        w->secret.remaining--;
    if (w->secret.kind == 3 && p->power == 2)
        w->secret.kind = 2;
    int kind = w->secret.kind;
    if (kind == 7)
        kind = p->power == 0 ? 7 : p->power == 1 ? 3 : 2;
    w->g.item_hit = kind;
}
void stage_secrets_start(world *w) {
    player_t *p = &w->g.player[w->g.cur_player];
    int index = -1;
    if (p->hotel >= 1 && p->hotel <= 5)
        index = (p->hotel - 1) * 10 + p->stage;
    else if (p->hotel == 7)
        index = (w->upper ? 60 : 50) + p->stage;
    else if (p->hotel == 8)
        index = (w->upper ? 85 : 70) + p->stage;
    if (index < 0 || index >= 100)
        return;
    unsigned code = hm_secret[index];
    w->secret.mode = code & 7;
    w->secret.kind = code >> 12;
    w->secret.floor = (code >> 3) & 7;
    w->secret.col = (code >> 6) & 7;
    w->secret.remaining = (code >> 9) & 7;
    w->secret.toggle = 1;
    w->secret.cooldown = 0;
    if (w->secret.floor >= 5 || w->secret.col >= 7) {
        w->secret.mode = 0;
        return;
    }
    w->secret.x = w->secret.mode == 1 ? (w->secret.col ? 354 : 20) : hm_door_x[w->secret.col] + 16;
    w->secret.y = hm_floor_y[w->secret.floor] - (w->secret.mode == 1 ? 36 : 33);
}
void stage_secrets_tick(world *w) {
    mario_t *m = &w->mario;
    if (w->secret.mode != 1)
        w->secret.cooldown--;
    int cell = w->secret.floor * COLS + w->secret.col;
    if (w->g.unk_53a == 0) {
        if (w->secret.mode == 2 && m->x + 7 < w->secret.x && w->secret.x < m->x + 25 &&
            m->y + 5 < w->secret.y && w->secret.y < m->y + 28)
            collect(w, 6);
        if ((w->secret.mode == 3 || w->secret.mode == 4) && m->state_group == 4 && m->state == 0 &&
            m->door == w->doors.cell + cell) {
            if (w->secret.mode == 3) {
                w->warp_hotel = w->g.player[w->g.cur_player].hotel == 1 ? 3 : 7;
                w->warp_stage = w->warp_hotel == 3 ? 2 : 7;
                w->status = WORLD_WARP;
            } else
                collect(w, 4);
        }
    }
    items(w);
}
void stage_secret_fireball(world *w, const hm_fx *fb) {
    if (w->secret.mode != 1)
        return;
    w->secret.cooldown--;
    if (fb->bx0 < w->secret.x && w->secret.x < fb->bx1 && fb->by0 < w->secret.y &&
        w->secret.y < fb->by1)
        collect(w, 2);
}
