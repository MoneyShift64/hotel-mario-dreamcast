/* hm_plat_dc.c - Hotel Mario platform layer for the Dreamcast (KallistiOS).
 * See hm_plat.h for the model. Pictures are opaque polygons, sprites go in the
 * translucent list (their "no pixel" palette entry has alpha 0). Textures are
 * point-sampled so the pixel art stays sharp.
 */
#include <kos.h>
#include <dc/pvr.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <sh4zam/shz_sh4zam.h>
#include <dc/sound/stream.h>
#include "hm_plat.h"
#include "light_regions.h"
#include "sound.h"
#include "movie.h"

/* CD-i 384x280 -> 640x467, centred vertically on the 640x480 screen */
#define SCALE (640.0f / CDI_W)
#define TOP   ((480.0f - CDI_H * SCALE) * 0.5f)

static uint32_t sPalShadow[1024];          /* what's in palette RAM (for reads) */

void plat_init(void) {
    pvr_init_params_t params = {
        { PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_0 },
        512 * 1024, 0, 0, 0, 4
    };
    int i;
    pvr_init(&params);
    pvr_set_bg_color(0.0f, 0.0f, 0.0f);
    pvr_set_pal_format(PVR_PAL_ARGB8888);
    for (i = 0; i < 1024; i++) {
        pvr_set_pal_entry(i, 0xff000000);
        sPalShadow[i] = 0xff000000;
    }
}

void *plat_load_file(const char *name, uint32_t *size) {
    char path[128];
    FILE *f;
    long n;
    void *d;
    snprintf(path, sizeof path, "/cd/data/%s", name);
    f = fopen(path, "rb");
    if (f == NULL) {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    d = memalign(32, n);
    if (d != NULL && fread(d, 1, n, f) != (size_t)n) n = 0;
    fclose(f);
    if (size) *size = (uint32_t)n;
    return d;
}

/* ---------------------------------------------------------------- palettes */
void plat_set_colour(int bank, int index, uint32_t rgb) {
    int i = bank * 256 + (index & 255);
    sPalShadow[i] = 0xff000000 | (rgb & 0xffffff);
    pvr_set_pal_entry(i, sPalShadow[i]);
}

void plat_set_transparent(int bank, int index) {
    int i = bank * 256 + (index & 255);
    sPalShadow[i] = 0;
    pvr_set_pal_entry(i, 0);
}

void plat_save_palette(int bank, uint32_t out[256]) {
    memcpy(out, sPalShadow + bank * 256, 256 * sizeof *out);
}
void plat_restore_palette(int bank, const uint32_t saved[256]) {
    memcpy(sPalShadow + bank * 256, saved, 256 * sizeof *saved);
    for (int i = 0; i < 256; i++) pvr_set_pal_entry(bank * 256 + i, saved[i]);
}

/* .pal file -> palette RAM (from colour 'first'); also returns the colours */
int plat_pal_from_mem(const uint8_t *d, int bank, int first, uint32_t rgb_out[], int max) {
    int n, i;
    if (d == NULL || memcmp(d, "HMPL", 4) != 0) return 0;
    n = d[4] | (d[5] << 8);
    for (i = 0; i < n; i++) {
        uint32_t rgb = (d[8 + 3 * i] << 16) | (d[9 + 3 * i] << 8) | d[10 + 3 * i];
        if (rgb_out && i < max) rgb_out[i] = rgb;
        if (bank >= 0) plat_set_colour(bank, first + i, rgb);
    }
    return n;
}

int plat_load_pal(const char *name, int bank, int first, uint32_t rgb_out[], int max) {
    uint32_t size;
    uint8_t *d = plat_load_file(name, &size);
    int n = plat_pal_from_mem(d, bank, first, rgb_out, max);
    if (d) free(d);
    return n;
}

/* ---------------------------------------------------------------- textures */
static inline int rd16(const uint8_t *p) { return p[0] | (p[1] << 8); }

int plat_tex_from_mem(const uint8_t *d, int bank, hm_tex *t) {
    int i, o = 8;
    memset(t, 0, sizeof *t);
    if (d == NULL || memcmp(d, "HMT2", 4) != 0) return 0;
    t->parts = rd16(d + 4);
    t->used_w = rd16(d + 6);
    t->bank = bank;
    if (t->parts < 1 || t->parts > 2) return 0;
    for (i = 0; i < t->parts; i++) {
        t->w[i] = rd16(d + o);
        t->h[i] = rd16(d + o + 2);
        t->y0[i] = rd16(d + o + 4);
        t->rows[i] = rd16(d + o + 6);
        t->vram[i] = pvr_mem_malloc(t->w[i] * t->h[i]);
        pvr_txr_load(d + o + 8, t->vram[i], t->w[i] * t->h[i]);   /* already twiddled */
        o += 8 + t->w[i] * t->h[i];
        t->used_h = t->y0[i] + t->rows[i];
    }
    return 1;
}

int plat_load_tex(const char *name, int bank, hm_tex *t) {
    uint32_t size;
    uint8_t *d = plat_load_file(name, &size);
    int ok = plat_tex_from_mem(d, bank, t);
    if (d) free(d);
    return ok;
}

void plat_free_tex(hm_tex *t) {
    int i;
    for (i = 0; i < 2; i++) {
        if (t->vram[i]) pvr_mem_free(t->vram[i]);
        t->vram[i] = NULL;
    }
    t->parts = 0;
}

void *plat_thread_start(void *(*fn)(void *), void *arg) {
    return thd_create(0, fn, arg);
}

void plat_thread_join(void *thread) {
    if (thread) thd_join((kthread_t *)thread, NULL);
}

int plat_load_sprites(const char *tex_name, const char *frm_name, hm_sprite_set *s) {
    uint32_t size;
    uint8_t *d;
    int i;
    if (!plat_load_tex(tex_name, BANK_PLANE_B, &s->tex)) return 0;
    d = plat_load_file(frm_name, &size);
    if (d == NULL || memcmp(d, "HMFR", 4) != 0) return 0;
    s->n_frames = d[4] | (d[5] << 8);
    s->n_slots = d[6] | (d[7] << 8);
    memcpy(s->frames, d + 8, s->n_frames * sizeof(hm_frame));
    for (i = 0; i < s->n_slots; i++) {
        const uint8_t *p = d + 8 + s->n_frames * sizeof(hm_frame) + 2 * i;
        s->slot[i] = p[0] | (p[1] << 8);
    }
    free(d);
    return 1;
}

/* ---------------------------------------------------------------- drawing */
static void quad_format(int list, const hm_tex *t, int part, float x0, float y0, float x1, float y1,
                 float u0, float v0, float u1, float v1, float z, unsigned format, uint32_t argb) {
    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t hdr;
    pvr_vertex_t v;
    pvr_poly_cxt_txr(&cxt, list,
                     format,
                     t->w[part], t->h[part], t->vram[part], PVR_FILTER_NONE);
    pvr_poly_compile(&hdr, &cxt);
    pvr_prim(&hdr, sizeof hdr);
    v.argb = argb;
    v.oargb = 0;
    v.z = z;
    v.flags = PVR_CMD_VERTEX;
    v.x = x0; v.y = y1; v.u = u0; v.v = v1; pvr_prim(&v, sizeof v);
    v.x = x0; v.y = y0; v.u = u0; v.v = v0; pvr_prim(&v, sizeof v);
    v.x = x1; v.y = y1; v.u = u1; v.v = v1; pvr_prim(&v, sizeof v);
    v.flags = PVR_CMD_VERTEX_EOL;
    v.x = x1; v.y = y0; v.u = u1; v.v = v0; pvr_prim(&v, sizeof v);
}

static void quad(int list, const hm_tex *t, int part, float x0, float y0, float x1, float y1,
                 float u0, float v0, float u1, float v1, float z) {
    quad_format(list, t, part, x0, y0, x1, y1, u0, v0, u1, v1, z,
                PVR_TXRFMT_PAL8BPP | PVR_TXRFMT_8BPP_PAL(t->bank) | PVR_TXRFMT_TWIDDLED, 0xffffffff);
}

/* CD-i pixel position -> Dreamcast screen position */
static inline shz_vec2_t to_screen(float x, float y) {
    return shz_vec2_add(shz_vec2_scale(shz_vec2_init(x, y), SCALE), shz_vec2_init(0.0f, TOP));
}

static int sInList;   /* 0 none, 1 opaque, 2 translucent */
static struct { const hm_sprite_set *s; int f, x, y, scale; float z; } sQueue[256];
static int sQueued;
static hm_light_region sLightRegions[HM_LIGHT_REGION_MAX];
static int sLightCount,sLightRequested;
void plat_set_light_mask(int (*light)(void *,int,int),void *context) {
    sLightCount=light_regions_build(light,context,sLightRegions,HM_LIGHT_REGION_MAX);
    if(sLightCount<0)sLightCount=0;
}
/* Both planes share the same light field. Darken the composed scene once,
 * underneath the HUD/UI, instead of splitting every texture into scanlines. */
static void draw_light_regions(void) {
    if(!sLightRequested || !sLightCount)return;
    pvr_poly_cxt_t c; pvr_poly_hdr_t h; pvr_vertex_t v={0};
    pvr_poly_cxt_col(&c,PVR_LIST_TR_POLY);pvr_poly_compile(&h,&c);pvr_prim(&h,sizeof h);
    v.z=3.9f;
    for(int i=0;i<sLightCount;i++) {
        const hm_light_region*r=&sLightRegions[i];
        shz_vec2_t a=to_screen(r->x,r->y),b=to_screen(r->x+r->w,r->y+r->h);
        v.argb=(uint32_t)r->alpha<<24;v.flags=PVR_CMD_VERTEX;
        v.x=a.x;v.y=b.y;pvr_prim(&v,sizeof v);
        v.x=a.x;v.y=a.y;pvr_prim(&v,sizeof v);
        v.x=b.x;v.y=b.y;pvr_prim(&v,sizeof v);
        v.flags=PVR_CMD_VERTEX_EOL;v.x=b.x;v.y=a.y;pvr_prim(&v,sizeof v);
    }
}
void plat_draw_picture_lit(const hm_tex*t,int offset_y) {
    sLightRequested=1;plat_draw_picture_offset(t,offset_y);
}
void plat_draw_tile_lit(const hm_tex*t,int sx,int sy,int w,int h,int x,int y) {
    plat_draw_tile(t,sx,sy,w,h,x,y);
}

void plat_frame_begin(void) {
    pvr_wait_ready();
    pvr_scene_begin();
    pvr_list_begin(PVR_LIST_OP_POLY);
    sInList = 1;
    sQueued = 0;
    sLightRequested=0;
}

void plat_wait_ready(void) { pvr_wait_ready(); }

void plat_draw_picture_offset(const hm_tex *t, int offset_y) {
    int i;
    for (i = 0; i < t->parts; i++) {
        shz_vec2_t a = to_screen(0, t->y0[i] + offset_y), b = to_screen(t->used_w, t->y0[i] + t->rows[i] + offset_y);
        quad(PVR_LIST_OP_POLY, t, i, a.x, a.y, b.x, b.y,
             0.0f, 0.0f, (float)t->used_w / t->w[i], (float)t->rows[i] / t->h[i], 1.0f);
    }
}

void plat_draw_panel(int x, int y, int width, int height, uint32_t argb) {
    if (sInList == 1) {
        pvr_list_finish(); pvr_list_begin(PVR_LIST_TR_POLY); sInList = 2;
    }
    pvr_poly_cxt_t c; pvr_poly_hdr_t h; pvr_vertex_t v = {0};
    pvr_poly_cxt_col(&c, PVR_LIST_TR_POLY); pvr_poly_compile(&h, &c);
    pvr_prim(&h, sizeof h);
    shz_vec2_t a = to_screen(x, y), b = to_screen(x + width, y + height);
    v.argb = argb; v.z = 4.5f; v.flags = PVR_CMD_VERTEX;
    v.x=a.x;v.y=b.y;pvr_prim(&v,sizeof v);
    v.x=a.x;v.y=a.y;pvr_prim(&v,sizeof v);
    v.x=b.x;v.y=b.y;pvr_prim(&v,sizeof v);
    v.flags=PVR_CMD_VERTEX_EOL;v.x=b.x;v.y=a.y;pvr_prim(&v,sizeof v);
}

void plat_draw_overlay(const hm_tex *t, unsigned alpha) {
    if (sInList == 1) {
        pvr_list_finish();
        pvr_list_begin(PVR_LIST_TR_POLY);
        sInList = 2;
    }
    for (int i = 0; i < t->parts; i++) {
        shz_vec2_t a = to_screen(0, t->y0[i]);
        shz_vec2_t b = to_screen(t->used_w, t->y0[i] + t->rows[i]);
        quad_format(PVR_LIST_TR_POLY, t, i, a.x, a.y, b.x, b.y, 0, 0,
            (float)t->used_w/t->w[i], (float)t->rows[i]/t->h[i], 2,
            PVR_TXRFMT_PAL8BPP | PVR_TXRFMT_8BPP_PAL(t->bank) | PVR_TXRFMT_TWIDDLED,
            ((alpha & 255) << 24) | 0xffffff);
    }
}

void plat_draw_picture(const hm_tex *t) { plat_draw_picture_offset(t, 0); }

void plat_draw_tile(const hm_tex *t, int sx, int sy, int w, int h, int x, int y) {
    shz_vec2_t a = to_screen(x, y), b = to_screen(x + w, y + h);
    int i = (t->parts > 1 && sy >= t->y0[1]) ? 1 : 0;      /* (sheets fit in one part) */
    float tw = (float)t->w[i], th = (float)t->h[i];
    sy -= t->y0[i];
    quad(PVR_LIST_OP_POLY, t, i, a.x, a.y, b.x, b.y,
         sx / tw, sy / th, (sx + w) / tw, (sy + h) / th, 1.5f);
}

/* (sprites are collected and drawn in the translucent list at the end) */
void plat_draw_frame_scaled(const hm_sprite_set *s, int frame, int x, int y, float depth, int scale) {
    if (sQueued < 256 && frame >= 0 && frame < s->n_frames) {
        sQueue[sQueued].s = s; sQueue[sQueued].f = frame;
        sQueue[sQueued].x = x; sQueue[sQueued].y = y; sQueue[sQueued].z = depth; sQueue[sQueued].scale = scale;
        sQueued++;
    }
}

void plat_draw_frame(const hm_sprite_set *s, int frame, int x, int y, float depth) {
    plat_draw_frame_scaled(s, frame, x, y, depth, 1);
}

void plat_draw_frame_masked(const hm_sprite_set*s,int frame,int x,int y,float depth) {
    plat_draw_frame(s,frame,x,y,depth);
}

void plat_frame_end(void) {
    plat_sound_poll();
    int i;
    if (sInList == 1) {
        pvr_list_finish();
        pvr_list_begin(PVR_LIST_TR_POLY);
    }
    for (i = 0; i < sQueued; i++) {
        const hm_sprite_set *s = sQueue[i].s;
        const hm_frame *f = &s->frames[sQueue[i].f];
        int scale = sQueue[i].scale;
        shz_vec2_t a = to_screen(sQueue[i].x + f->ox * scale, sQueue[i].y + f->oy * scale);
        shz_vec2_t b = to_screen(sQueue[i].x + (f->ox + f->w) * scale,
                                sQueue[i].y + (f->oy + f->h) * scale);
        float tw = (float)s->tex.w[0], th = (float)s->tex.h[0];
        quad(PVR_LIST_TR_POLY, &s->tex, 0, a.x, a.y, b.x, b.y,
             f->x / tw, f->y / th, (f->x + f->w) / tw, (f->y + f->h) / th, 2.0f + sQueue[i].z);
    }
    draw_light_regions();
    pvr_list_finish();
    pvr_scene_finish();
    sInList = 0;
}

/* ---------------------------------------------------------------- controls */
uint32_t plat_pad(void) {
    maple_device_t *dev = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
    cont_state_t *st;
    uint32_t b = 0;
    if (dev == NULL || (st = (cont_state_t *)maple_dev_status(dev)) == NULL) return 0;
    if (st->buttons & CONT_DPAD_LEFT)  b |= PAD_LEFT;
    if (st->buttons & CONT_DPAD_RIGHT) b |= PAD_RIGHT;
    if (st->buttons & CONT_DPAD_UP)    b |= PAD_UP;
    if (st->buttons & CONT_DPAD_DOWN)  b |= PAD_DOWN;
    if (st->buttons & CONT_A) b |= PAD_B1;
    if (st->buttons & CONT_B) b |= PAD_B2;
    if (st->buttons & CONT_X) b |= PAD_X;
    if (st->buttons & CONT_Y) b |= PAD_Y;
    if (st->buttons & CONT_START) b |= PAD_START;
    if (st->ltrig > 128) b |= PAD_L;
    if (st->rtrig > 128) b |= PAD_R;
    /* the analogue stick works like the d-pad too */
    if (st->joyx < -64) b |= PAD_LEFT;
    if (st->joyx > 64)  b |= PAD_RIGHT;
    if (st->joyy < -64) b |= PAD_UP;
    if (st->joyy > 64)  b |= PAD_DOWN;
    return b;
}

int plat_soft_reset(void) {
    const uint32_t all = PAD_B1 | PAD_B2 | PAD_X | PAD_Y | PAD_START;
    return (plat_pad() & all) == all;
}

/* ---------------------------------------------------------------- sound */
static int sSoundPaused;
void plat_sound_pause(int paused) { sSoundPaused = paused; }

static snd_stream_hnd_t sStream = SND_STREAM_INVALID;
static int16_t sSoundBuf[8192] __attribute__((aligned(32)));

static void *sound_cb(snd_stream_hnd_t hnd, int bytes_req, int *bytes_recv) {
    int n = bytes_req / 2;                       /* (KOS asks in bytes) */
    (void)hnd;
    if (n > 8192) n = 8192;
    n &= ~15;
    if (sSoundPaused) memset(sSoundBuf, 0, n * sizeof *sSoundBuf);
    else if (!movie_audio_render(sSoundBuf, n)) snd_render(sSoundBuf, n);
    *bytes_recv = n * 2;
    return sSoundBuf;
}

void plat_sound_init(void) {
    snd_init_state();
    if (snd_stream_init() < 0) return;
    sStream = snd_stream_alloc(sound_cb, 8192);  /* ~0.1 s of buffering */
    if (sStream == SND_STREAM_INVALID) return;
    snd_stream_start(sStream, SND_RATE, 0);
}

void plat_sound_poll(void) {
    if (sStream != SND_STREAM_INVALID) snd_stream_poll(sStream);
}

/* Movie texture buffers are allocated once and updated without changing game art. */
void plat_movie_yield(void) { thd_sleep(1); }
unsigned plat_movie_clock_ms(void) { return (unsigned)(timer_us_gettime64()/1000); }
int plat_movie_texture_create(hm_tex *t) {
    memset(t,0,sizeof *t);t->parts=2;t->used_w=384;t->used_h=280;t->bank=0;
    for(int i=0;i<2;i++) {
        t->w[i]=512;t->h[i]=i?32:256;t->y0[i]=i?256:0;t->rows[i]=i?24:256;
        t->vram[i]=pvr_mem_malloc(512*t->h[i]);
        if(!t->vram[i]){plat_free_tex(t);return 0;}
    }
    return 1;
}
void plat_movie_texture_update(hm_tex *t,const uint8_t *data) {
    pvr_txr_load(data,t->vram[0],512*256);
    pvr_txr_load(data+512*256,t->vram[1],512*32);
}
void plat_movie_sound_start(const int16_t *pcm,unsigned samples) {
    if(sStream==SND_STREAM_INVALID)return;
    snd_stream_stop(sStream);movie_audio_begin(pcm,samples);sSoundPaused=0;
    snd_stream_start(sStream,SND_RATE,0);
}
void plat_movie_sound_stop(void) {
    if(sStream!=SND_STREAM_INVALID)snd_stream_stop(sStream);
    movie_audio_end();
    if(sStream!=SND_STREAM_INVALID)snd_stream_start(sStream,SND_RATE,0);
}
