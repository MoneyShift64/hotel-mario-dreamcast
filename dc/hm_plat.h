/* hm_plat.h - Hotel Mario Dreamcast platform layer.
 *
 * Mirrors the CD-i model the game was written for:
 *   - pictures are 8-bit paletted (CLUT7), so they become PAL8 PowerVR textures;
 *   - each CD-i plane gets its own 256-entry palette bank: plane B (the stage and
 *     every sprite) uses bank 0, plane A (title cards, the loading screen) bank 1;
 *   - colour changes (player colours, fire flower, colour cycling) are palette writes,
 *     exactly like the game's dc_wrfct() calls, and cost nothing per pixel;
 *   - sprites are quads cut from a texture atlas instead of compiled 68000 code.
 * Coordinates given to the drawing calls are CD-i screen pixels (384 x 280).
 */
#ifndef HM_PLAT_H
#define HM_PLAT_H
#include <stdint.h>

#define CDI_W 384
#define CDI_H 280
#define BANK_PLANE_B 0
#define BANK_PLANE_A 1
#define PAL_TRANSPARENT 0xff     /* sprite texel meaning "no pixel" */

typedef struct hm_tex {          /* a picture: one or two textures stacked vertically */
    int parts;
    void *vram[2];               /* PowerVR texture memory */
    int w[2], h[2];              /* texture sizes (power of two) */
    int y0[2], rows[2];          /* which picture lines each part holds */
    int used_w, used_h;          /* the picture */
    int bank;                    /* palette bank used to draw it */
} hm_tex;

typedef struct hm_frame { uint16_t x, y, w, h; int16_t ox, oy; } hm_frame;

typedef struct hm_sprite_set {
    hm_tex tex;
    int n_frames, n_slots;
    hm_frame frames[128];
    uint16_t slot[128];          /* the game's frame slots (as filled in by ms_init) */
} hm_sprite_set;

/* buttons, in the order the game thinks of them */
enum {
    PAD_LEFT = 1, PAD_RIGHT = 2, PAD_UP = 4, PAD_DOWN = 8,
    PAD_B1 = 16,                 /* CD-i button 1 (A) */
    PAD_B2 = 32,                 /* CD-i button 2 (B) */
    PAD_X = 64, PAD_Y = 128, PAD_START = 256,
    PAD_L = 512, PAD_R = 1024     /* shoulder triggers */
};

/* Movie-only platform operations; gameplay sound remains on the original path. */
void plat_movie_yield(void);
unsigned plat_movie_clock_ms(void);
int plat_movie_texture_create(hm_tex *t);
void plat_movie_texture_update(hm_tex *t, const uint8_t *twiddled);
void plat_movie_sound_start(const int16_t *pcm, unsigned samples);
void plat_movie_sound_stop(void);
void plat_init(void);
void *plat_load_file(const char *name, uint32_t *size);   /* from /cd/data, 32-byte aligned */

int  plat_load_tex(const char *name, int bank, hm_tex *t);
void plat_free_tex(hm_tex *t);
int  plat_tex_from_mem(const uint8_t *file, int bank, hm_tex *t);       /* upload a loaded .tex */
int  plat_pal_from_mem(const uint8_t *file, int bank, int first, uint32_t rgb_out[], int max);
void *plat_thread_start(void *(*fn)(void *), void *arg);                /* background loading */
void plat_thread_join(void *thread);
int  plat_load_pal(const char *name, int bank, int first, uint32_t rgb_out[], int max);
void plat_set_colour(int bank, int index, uint32_t rgb);  /* 0xRRGGBB */
void plat_set_transparent(int bank, int index);
void plat_save_palette(int bank, uint32_t out[256]);
void plat_restore_palette(int bank, const uint32_t saved[256]);
void plat_draw_panel(int x, int y, int width, int height, uint32_t argb);
void plat_draw_overlay(const hm_tex *t, unsigned alpha);
void plat_sound_pause(int paused); /* emit silence without advancing the decoder */
int  plat_load_sprites(const char *tex_name, const char *frm_name, hm_sprite_set *s);

/* World drawing and scene submission. */
void plat_set_light_mask(int (*light)(void *, int, int), void *context);
void plat_draw_frame_masked(const hm_sprite_set *s, int frame, int x, int y, float depth);
void plat_frame_begin(void);
void plat_wait_ready(void);
void plat_draw_picture_lit(const hm_tex *t, int offset_y);
void plat_draw_tile_lit(const hm_tex *t, int sx, int sy, int w, int h, int x, int y);
void plat_draw_picture_offset(const hm_tex *t, int offset_y);
void plat_draw_picture(const hm_tex *t);                     /* full screen, opaque */
void plat_draw_frame(const hm_sprite_set *s, int frame, int x, int y, float depth);
void plat_draw_frame_scaled(const hm_sprite_set *s, int frame, int x, int y, float depth, int scale);
/* opaque w x h block from a sheet texture (like the game's dm()) at CD-i (x, y) */
void plat_draw_tile(const hm_tex *t, int sx, int sy, int w, int h, int x, int y);
void plat_frame_end(void);

/* sound: one mono 16-bit stream at SND_RATE, filled by snd_render() (sound.c) */
void plat_sound_init(void);
void plat_sound_poll(void);                 /* once a frame */

uint32_t plat_pad(void);          /* PAD_* bits currently held */
int plat_soft_reset(void);        /* A+B+X+Y+Start held: restart the game */

#endif
