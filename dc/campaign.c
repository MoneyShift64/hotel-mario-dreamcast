#include <stdio.h>
#include <string.h>
#include "campaign.h"

void campaign_start(hm_campaign *c, int players) {
    int i;
    memset(c, 0, sizeof *c);
    c->players = players == 2 ? 2 : 1;
    for (i = 0; i < c->players; i++) {
        c->player[i].hotel = c->player[i].progress = 1;
        c->player[i].lives = 3;
    }
}

void campaign_capture(hm_campaign *c, const world *w) {
    c->player[c->current] = w->g.player[w->g.cur_player];
    c->bonus_time = w->status == WORLD_CLEARED && w->g.time_left > 0 ? w->g.time_left : 0;
    c->bonus_rate = c->player[c->current].hotel * 10; /* bonus.c */
    int multiplier = campaign_special_multiplier(w);
    c->special_bonus = w->status == WORLD_CLEARED && multiplier > 1 ?
                       c->player[c->current].hotel * multiplier * 200 : 0;
    c->special_paid = 0;
}

/* get_the_fxxxxxx_bonus, checked against original 68000 instructions
 * 0x07bc..0x088e: room+0x7c/0x7e are actors[].unk_048/unk_04a. */
int campaign_special_multiplier(const world *w) {
    int all_doors = 1, remaining_48 = 0, remaining_4a = 0;
    for (int i = 0; i < FLOORS * COLS; i++)
        if (!w->g.unk_550[2 * i] && !w->g.unk_550[2 * i + 1]) all_doors = 0;
    int door_bonus = all_doors ? 25 : 0;
    if (!w->mario.hit) return door_bonus + 75;
    for (int i = 0; i < w->room.n_actors; i++) {
        if (w->room.actors[i].unk_048) remaining_48 = 1;
        if (w->room.actors[i].unk_04a) remaining_4a = 1;
    }
    if (!remaining_48) return door_bonus + 50;
    if (!remaining_4a) return door_bonus + 25;
    return door_bonus ? door_bonus : 1;
}

void campaign_finish_bonus(hm_campaign *c) {
    if (!c->special_paid && c->bonus_time == 0) {
        c->player[c->current].score += c->special_bonus;
        c->special_paid = 1;
    }
}

int campaign_bonus_tick(hm_campaign *c) {
    if (c->bonus_time <= 0) return 0;
    c->player[c->current].score += c->bonus_rate;
    c->bonus_time--;
    return 1;
}

int campaign_advance(hm_campaign *c, int cleared) {
    int current = c->current, other = current ^ 1;
    player_t *p = &c->player[current];
    if (cleared) {
        if (++p->stage >= (p->hotel == 8 ? 15 : 10)) {
            if (p->hotel == 8) {
                c->finished[current] = 1;
            } else {
                p->stage = 0;
                if (++p->hotel == 6) p->hotel = 7;
                p->got_bonus = 1;
                if (!(p->flags & 4)) p->unk_10 = 0;
            }
        }
        if (p->hotel > p->progress) {
            p->progress = p->hotel;
            p->unk_18 = p->stage;
        } else if (p->hotel == p->progress && p->stage > p->unk_18) {
            p->unk_18 = p->stage;
        }
    } else {
        if (p->lives > 0) p->lives--;
        p->power = 0;
    }
    /* Alternate after a clear or a lost life, as after_the_game does. A player
     * who finished or ran out of lives no longer receives turns. */
    if (c->players == 2 && c->player[other].lives > 0 && !c->finished[other])
        c->current = other;
    else if (p->lives <= 0 || c->finished[current]) {
        int any_finished = c->finished[0] || c->finished[1];
        return any_finished ? CAMPAIGN_COMPLETE : CAMPAIGN_GAME_OVER;
    }
    return p->lives <= 0 || c->finished[current] ? CAMPAIGN_PLAYER_OUT : CAMPAIGN_NEXT;
}

int campaign_tag(const hm_campaign *c, char *tag, unsigned size) {
    const player_t *p = &c->player[c->current];
    int n;
    if (c->finished[c->current] || p->lives <= 0 || p->hotel < 1 || p->hotel > 8 || p->hotel == 6 ||
        p->stage < 0 || p->stage >= (p->hotel == 8 ? 15 : 10)) return 0;
    n = snprintf(tag, size, "%d_%02d", p->hotel, p->stage + 1);
    return n >= 0 && (unsigned)n < size;
}
