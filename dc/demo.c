/* Native attract-mode pilot: original rooms/rules, never a saved campaign. */
#include "demo.h"
#include <stdlib.h>
uint32_t demo_input(const world *w, unsigned frame) {
    const mario_t *m = &w->mario;
    /* The original controller requires Down to leave a lift or hiding door.
     * Steering alone leaves the attract player waiting inside indefinitely. */
    if ((m->state_group == 1 || m->state_group == 2 || m->state_group == 4) &&
        m->state == 0)
        return IN_DOWN;
    int target = -1, distance = 999;
    if (m->floor < 0 || m->floor >= 5)
        return 0;
    for (int c = 0; c < 7; c++)
        if ((w->doors.cell[m->floor * 7 + c] & 0xf000) == 0x2000) {
            int gap = abs(hm_door_x[c] - m->x);
            if (gap < distance) {
                distance = gap;
                target = c;
            }
        }
    uint32_t input = IN_RUN;
    if (target >= 0) {
        int dx = hm_door_x[target] - m->x;
        if (abs(dx) < 8) {
            if (frame % 24 < 6)
                input |= IN_B2;
        } else
            input |= dx < 0 ? IN_LEFT : IN_RIGHT;
    } else {
        int lift = -1;
        for (int c = 0; c < 7; c++)
            if ((w->doors.cell[m->floor * 7 + c] & 0xf000) == 0xf000)
                lift = c;
        if (lift >= 0) {
            int dx = hm_door_x[lift] - m->x;
            input |= abs(dx) < 8 ? IN_UP : dx < 0 ? IN_LEFT : IN_RIGHT;
        } else
            input |= (frame / 180) % 2 ? IN_LEFT : IN_RIGHT;
    }
    for (int i = 0; i < w->room.n_actors; i++) {
        const actor_t *a = &w->room.actors[i];
        if (a->type > 9 && a->state == 3 && a->cur_floor == m->floor && abs(a->x - m->x) < 44 &&
            frame % 48 < 24)
            input |= IN_B1;
    }
    if (w->g.player[w->g.cur_player].power == 2)
        input |= IN_ITEM;
    return input;
}
