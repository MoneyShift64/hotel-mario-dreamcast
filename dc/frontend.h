/* Native menu navigation. Hotel/progress rules follow init_the_game's map branch. */
#ifndef HM_FRONTEND_H
#define HM_FRONTEND_H
#include "world.h"
void frontend_continue_player(player_t *player);
int frontend_next_hotel(int hotel, int direction, int progress);
void frontend_choose_hotel(player_t *player, int hotel);
void frontend_record_progress(player_t best[2], const player_t current[2]);
const char *frontend_arrival(int hotel,int *x);
const char *frontend_ready(int month,int day,int *x);
#endif
