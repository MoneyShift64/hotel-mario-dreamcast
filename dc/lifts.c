/* elev_switch / do_switch: original two layouts, swapped every 17 time units.
 * The original defers changes while Mario rides a lift or a boss owns the lift lock. */
#include "world.h"
#include <string.h>

static int mode(const player_t *p) {
    static const int normal[9] = {0, 0, 1, 0, 2, 0, 0, 1, 0};
    static const int boss[9] = {0, 1, 0, 2, 0, 1, 0, 0, 0};
    if (p->hotel == 0)
        return p->stage == 2 || p->stage == 5 || p->stage == 8 ? 2 : 0;
    if (p->hotel < 1 || p->hotel > 8)
        return 0;
    return p->stage == 9 ? boss[p->hotel] : normal[p->hotel];
}
static void reverse_layout(uint16_t *grid) {
    int previous = 8;
    for (int c = 0; c < COLS; c++)
        if ((grid[c] & 0xf000) == 0xe000) {
            grid[c] = (grid[c] & 0xfff) | 0xf000;
            break;
        }
    for (int c = 0; c < COLS; c++)
        if ((grid[28 + c] & 0xf000) == 0xf000) {
            grid[28 + c] = (grid[28 + c] & 0xfff) | 0xe000;
            break;
        }
    for (int f = 0; f < 4; f++) {
        int a = -1, b = -1;
        for (int c = 0; c < COLS; c++)
            if (c != previous && (grid[f * COLS + c] & 0xf000) == 0xf000) {
                a = c;
                break;
            }
        for (int c = 0; c < COLS; c++)
            if ((grid[(f + 1) * COLS + c] & 0xf000) == 0xe000) {
                b = c;
                break;
            }
        if (a < 0 || b < 0)
            return; /* malformed data, never follow an uninitialised pointer */
        grid[f * COLS + a] = (grid[f * COLS + a] & 0x000f) | 0xe000 | ((f + 1) << 8) | (b << 4);
        grid[(f + 1) * COLS + b] =
            (grid[(f + 1) * COLS + b] & 0x000f) | 0xf000 | (f << 8) | (a << 4);
        previous = b;
    }
}
static void paired_layout(world *w, const int *loc, int source) {
    uint16_t *a = w->lift_grid[0], *b = w->lift_grid[1];
    int floor = (a[loc[source]] >> 8) & 15, lo = floor * 2 - 1, hi = floor * 2;
    if (lo < 0 || hi >= 8)
        return;
    b[loc[lo]] = a[loc[hi]];
    b[loc[hi]] = a[loc[lo]];
    int partner = loc[source] / COLS == ((a[loc[lo]] >> 8) & 15) ? hi : lo;
    int col = loc[partner] - floor * COLS;
    b[loc[source]] = (a[loc[source]] & 0xff0f) | (col << 4);
    int target = ((a[loc[partner]] >> 8) & 15) * COLS + ((a[loc[partner]] >> 4) & 15);
    for (int k = 0; k < 8; k++)
        if (loc[k] == target) {
            b[loc[k]] = (a[loc[source]] & 0xff0) | (lo < k ? 0xf000 : 0xe000);
            break;
        }
}
void stage_lifts_start(world *w) {
    int loc[8], n = 0;
    w->lift_kind = mode(&w->g.player[w->g.cur_player]);
    if (!w->lift_kind)
        return;
    for (int i = 0; i < 35; i++)
        if ((w->doors.cell[i] & 0xe000) == 0xe000) {
            if (n == 8) {
                w->lift_kind = 0;
                return;
            }
            loc[n++] = i;
        }
    if (n != 8) {
        w->lift_kind = 0;
        return;
    }
    w->lift_n = n;
    memcpy(w->lift_grid[0], w->doors.cell, sizeof w->lift_grid[0]);
    memcpy(w->lift_grid[1], w->doors.cell, sizeof w->lift_grid[1]);
    if (w->lift_kind == 1)
        reverse_layout(w->lift_grid[1]);
    else {
        paired_layout(w, loc, 0);
        paired_layout(w, loc, 7);
    }
    w->lift_phase = 0;
    w->lift_pending = 0;
    w->lift_time = 0;
}
void stage_lifts_tick(world *w) {
    gds *g = &w->g;
    if (!w->lift_kind || g->unk_53a != 0 || g->time_left <= 0)
        return;
    if (g->time_left % 17 && !w->lift_pending)
        return;
    if (w->lift_time == g->time_left)
        return;
    if (w->mario.state_group == 1 || w->mario.state_group == 2 || w->mario.state_group == 7 ||
        g->unk_530) {
        w->lift_pending = 1;
        return;
    }
    w->lift_phase ^= 1;
    for (int i = 0; i < 35; i++)
        if ((w->lift_grid[0][i] & 0xe000) == 0xe000) {
            w->doors.cell[i] = w->lift_grid[w->lift_phase][i];
            w->tile[i / COLS][i % COLS].on = 0;
        }
    w->lift_time = g->time_left;
    w->lift_pending = 0;
    g->unk_538 = w->lift_phase ? 0 : 1;
    hm_afx(4);
}
