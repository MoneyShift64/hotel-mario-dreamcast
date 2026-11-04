/* doors.c - Hotel Mario doors and lifts. See doors.h. */
#include "doors.h"

const int hm_floor_y[FLOORS] = { 72, 108, 144, 180, 216 };              /* init_gds */
const int hm_door_x[COLS] = { 44, 88, 132, 176, 220, 264, 308 };        /* init_gds */

#define TILE_STEP 0x22          /* tiles are 32x31, 34 pixels apart on the sheet */
#define ROW_LIFTS 0x21

#define CELL(d, f, c) ((d)->cell[(f) * COLS + (c)])

void hm_doors_init(hm_doors *d, uint16_t *grid) {
    int f, c, u;
    d->cell = grid;
    d->count = d->open = 0;
    for (f = 0; f < FLOORS; f++) {
        for (u = 0; u < 0x17; u++) d->floor_map[f][u] = 0;
        for (c = 0; c < COLS; c++) {
            uint16_t v = CELL(d, f, c);
            d->moving[f][c] = 0;
            d->tick[f][c] = 0;
            d->anim[f][c] = 0;
            switch (v & 0xf000) {                                   /* init_doors */
            case 0x2000: d->count++; d->open++; d->anim[f][c] = 3; break;
            case 0x1000: d->count++; break;
            }
            if (v & 0xf000) {                                       /* init_floor_map */
                int m = hm_door_x[c] >> 1;
                for (u = m - 5; u <= m + 5; u++) d->floor_map[f][u >> 3] |= 1 << (u & 7);
            }
        }
    }
}

int hm_doors_kind(const hm_doors *d, int floor, int col) {
    uint16_t v;
    if (floor < 0 || floor >= FLOORS || col < 0 || col >= COLS) return DOOR_NONE;
    v = CELL(d, floor, col);
    switch (v & 0xf000) {
    case 0x1000: case 0x2000: return DOOR;
    case 0xe000:
    case 0xf000: {
        /* a working lift leads to another lift that leads straight back */
        int tf = (v >> 8) & 15, tc = (v >> 4) & 15;
        if (tf < FLOORS && tc < COLS && !(tf == floor && tc == col)) {
            uint16_t w = CELL(d, tf, tc);
            if ((w & 0xe000) == 0xe000 && (int)((w >> 8) & 15) == floor && (int)((w >> 4) & 15) == col)
                return (v & 0xf000) == 0xf000 ? LIFT_UP : LIFT_DOWN;
        }
        return (v & 0xf000) == 0xf000 ? ROOF_EXIT : LIFT_DOWN;
    }
    }
    return DOOR_NONE;
}

static int map_bit(const hm_doors *d, int floor, int u) {
    if (u < 0 || u >= 0x17 * 8) return 0;
    return (d->floor_map[floor][u >> 3] >> (u & 7)) & 1;
}

/* is_door_elev (Mario): bitmap test at x/2, then the column from fixed boundaries */
int hm_doors_at(const hm_doors *d, int floor, int x) {
    int p = x + 16;
    if (floor < 0 || floor >= FLOORS || !map_bit(d, floor, x >> 1)) return -1;
    if (p < 0x58) return 0;
    if (p < 0x84) return 1;
    if (p < 0xb0) return 2;
    if (p < 0xdc) return 3;
    if (p < 0x108) return 4;
    if (p < 0x134) return 5;
    return 6;
}

/* the stage modules' version (m_00076): x is the object's centre */
uint16_t *hm_door_cell_at(const hm_doors *d, int floor, int x) {
    int col;
    if (floor < 0 || floor >= FLOORS || !map_bit(d, floor, (x - 0x14) >> 1)) return 0;
    col = (x - 0x2c) / 0x2c;
    if (col < 0 || col >= COLS) return 0;
    return &CELL(d, floor, col);
}

/* Mario pressing button 2 at a door (mario, 0xa018..): returns the sound */
int hm_doors_toggle(hm_doors *d, int floor, int col, int32_t *score) {
    uint16_t *v;
    if (hm_doors_kind(d, floor, col) != DOOR) return SND_NONE;
    v = &CELL(d, floor, col);
    if (*v & 1) return SND_NONE;                    /* already changing this frame */
    if ((*v & 0xf000) == 0x1000) {
        *v = (*v & 0xefff) | 0x2000 | 1;
        d->anim[floor][col] = 3;                    /* (mario's ddm: drawn at once) */
        return SND_DOOR_OPEN;
    }
    *v = (*v & 0xdfff) | 0x1000 | 1;
    d->anim[floor][col] = 0;
    if (*v & 8) {                                   /* flagged door: 50 points, once */
        if (score) *score += 50;
        *v &= ~8;
    }
    return SND_DOOR_CLOSE;
}

int hm_doors_lift(const hm_doors *d, int floor, int col, int *to_floor, int *to_col) {
    int k = hm_doors_kind(d, floor, col);
    uint16_t v;
    if (k != LIFT_UP && k != LIFT_DOWN) return 0;
    v = CELL(d, floor, col);
    *to_floor = (v >> 8) & 15;
    *to_col = (v >> 4) & 15;
    return *to_floor < FLOORS && *to_col < COLS;
}

/* check_doors: a changed cell (bit 0) updates the open count and is redrawn at once
   (the CD-i draws the open or closed picture straight away: there is no in-between) */
void hm_doors_update(hm_doors *d) {
    int f, c;
    for (f = 0; f < FLOORS; f++) {
        for (c = 0; c < COLS; c++) {
            uint16_t *v = &CELL(d, f, c);
            if (*v & 4) continue;                   /* bit 2: an empty doorway */
            if (*v & 1) {
                if ((*v & 0xf000) == 0x1000) {
                    d->open--;
                    d->anim[f][c] = 0;
                    *v &= ~8;
                } else if ((*v & 0xf000) == 0x2000) {
                    d->open++;
                    d->anim[f][c] = 3;
                }
                *v &= ~1;
            } else if (*v & 2) {                    /* bit 1: redraw (set by the objects) */
                if ((*v & 0xf000) == 0x1000) d->anim[f][c] = 0;
                else if ((*v & 0xf000) == 0x2000) d->anim[f][c] = 3;
                *v &= ~2;
            }
            d->moving[f][c] = 0;
        }
    }
}

int hm_doors_tile(const hm_doors *d, int floor, int col, int *sx, int *sy) {
    switch (hm_doors_kind(d, floor, col)) {
    case DOOR:
        if (CELL(d, floor, col) & 4) { *sx = 4 * TILE_STEP; *sy = 0; return 1; }  /* 0x88: doorway */
        *sx = d->anim[floor][col] * TILE_STEP; *sy = 0; return 1;
    case LIFT_UP:
    case ROOF_EXIT: *sx = 0; *sy = ROW_LIFTS; return 1;
    case LIFT_DOWN: *sx = 6 * TILE_STEP; *sy = ROW_LIFTS; return 1;   /* 0xcc */
    }
    return 0;
}
