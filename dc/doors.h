/* doors.h - Hotel Mario doors and lifts (no platform code: also builds on a PC).
 *
 * The grid is the game's own: stage_info + 0x12, 5 floors x 7 columns of 16-bit cells,
 * shared with the enemy code (goombas open doors by writing it, as on the CD-i).
 *   top 4 bits  0x1000 door closed, 0x2000 door open, 0xE000 lift down, 0xF000 lift up
 *               (0xF000 on the top floor of hotels 7/8 with no destination = roof exit)
 *   lifts       bits 8-11 destination floor, bits 4-7 destination column
 *   low bits    1: changed (check_doors updates the open count), 2: redraw,
 *               4: an object is standing in the doorway (drawn as an empty doorway),
 *               8: closing this door scores 50
 * Rules from the CD-i code: init_doors, init_floor_map, is_door_elev, mario (button 2),
 * check_doors and exit_check.
 */
#ifndef HM_DOORS_H
#define HM_DOORS_H
#include <stdint.h>

#define FLOORS 5
#define COLS 7

extern const int hm_floor_y[FLOORS];     /* gds->floor_y */
extern const int hm_door_x[COLS];        /* gds->door_x */

typedef struct hm_doors {
    uint16_t *cell;                      /* -> stage_info + 0x12, [floor * COLS + col] */
    uint8_t  anim[FLOORS][COLS];         /* door picture 0 (closed) .. 3 (open) */
    int8_t   moving[FLOORS][COLS];       /* +1 opening, -1 closing, 0 still */
    uint8_t  tick[FLOORS][COLS];
    uint8_t  floor_map[FLOORS][0x17];    /* gds->unk_4b4: door_x +-10 pixels, 2 pixels a bit */
    int count;                           /* doors in the stage  (gds->doors_count) */
    int open;                            /* doors open now      (gds->doors_open) */
} hm_doors;

enum { DOOR_NONE, DOOR, LIFT_UP, LIFT_DOWN, ROOF_EXIT };
enum { SND_NONE = 0, SND_DOOR_CLOSE = 1, SND_DOOR_OPEN = 2 };  /* the game's playAFX numbers */

void hm_doors_init(hm_doors *d, uint16_t *grid);
int  hm_doors_kind(const hm_doors *d, int floor, int col);
int  hm_doors_at(const hm_doors *d, int floor, int x);        /* is_door_elev: column, or -1 */
uint16_t *hm_door_cell_at(const hm_doors *d, int floor, int x);   /* the objects' version */
int  hm_doors_toggle(hm_doors *d, int floor, int col, int32_t *score);   /* Mario's button 2 */
int  hm_doors_lift(const hm_doors *d, int floor, int col, int *to_floor, int *to_col);
void hm_doors_update(hm_doors *d);                           /* check_doors + animation */
int  hm_doors_tile(const hm_doors *d, int floor, int col, int *sx, int *sy);   /* 0 = draw nothing */

#endif
