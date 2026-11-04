/* Campaign state, independent of rendering and disc ordering.
 * Ordinary campaign rules from src/game/after_the_game.c; special flag modes
 * and the original bonus presentation remain separate work. Stages are zero-based.
 */
#ifndef HM_CAMPAIGN_H
#define HM_CAMPAIGN_H
#include "world.h"
typedef struct hm_campaign {
    player_t player[2];
    int current, players;
    int finished[2];
    int bonus_time, bonus_rate;
    int special_bonus, special_paid;
} hm_campaign;
enum { CAMPAIGN_NEXT, CAMPAIGN_PLAYER_OUT, CAMPAIGN_GAME_OVER, CAMPAIGN_COMPLETE };
void campaign_start(hm_campaign *c, int players);
/* Capture a world result once, after its ending animation. */
void campaign_capture(hm_campaign *c, const world *w);
/* Transfer one remaining second of the time bonus to the score. */
int campaign_bonus_tick(hm_campaign *c);
int campaign_special_multiplier(const world *w);
void campaign_finish_bonus(hm_campaign *c);
/* Advance/retry and select the next living, unfinished player. */
int campaign_advance(hm_campaign *c, int cleared);
int campaign_tag(const hm_campaign *c, char *tag, unsigned size);
#endif
