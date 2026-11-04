/* sound.c - Hotel Mario's sound, the way the CD-i makes it (platform-free).
 *
 * The CD-i streams the stage tune as CD-i ADPCM sectors (37.8 kHz, 4 bit) into a sound
 * map, and cd_mix ORs the current sound effect into each sector as it arrives: the
 * tune uses the left channel's sound units, the effects the right one's, and the audio
 * hardware mixes both to the speakers. playAFX just points cd_xbuf at an effect; the
 * next sector picks it up (18 sound groups = 2016 samples, ~53 ms, at a time).
 * Here: the same, decoded in software and mixed to one 16-bit channel at 37800 Hz.
 * Data (tools/make_dc_assets.py): 60-byte groups of 4 parameter bytes + 28 words.
 */
#include <string.h>
#include "sound.h"

#define GROUPS_PER_SECTOR 18
#define SECTOR_SAMPLES (GROUPS_PER_SECTOR * 112)

typedef struct xa_dec { int32_t o1, o2; } xa_dec;

typedef struct xa_track {
    const uint8_t *g;               /* groups */
    uint32_t n, pos, end;           /* group count, next group, stop at */
    int loop, active;
    xa_dec dec;
} xa_track;

static const int16_t K0[4] = { 0, 60, 115, 98 }, K1[4] = { 0, 0, -52, -55 };

static void decode_group(const uint8_t *grp, xa_dec *d, int16_t *out) {
    int k, t;
    for (k = 0; k < 4; k++) {
        int range = grp[k] & 15, f = (grp[k] >> 4) & 3;
        int32_t o1 = d->o1, o2 = d->o2;
        for (t = 0; t < 28; t++) {
            const uint8_t *w = grp + 4 + 2 * t;
            int nib = ((w[0] | (w[1] << 8)) >> (4 * k)) & 15;
            int32_t s = (int32_t)(int16_t)(nib << 12) >> range;
            s += (o1 * K0[f] + o2 * K1[f] + 32) >> 6;
            if (s > 32767) s = 32767;
            if (s < -32768) s = -32768;
            o2 = o1;
            o1 = s;
            *out++ = (int16_t)s;
        }
        d->o1 = o1;
        d->o2 = o2;
    }
}

static struct {
    xa_track music, sfx;
    const uint8_t *sfx_data;        /* group data of the stage's effects */
    uint32_t sfx_offs[17];
    int sfx_req;                    /* playAFX: next effect (-1 none) */
    int16_t mbuf[SECTOR_SAMPLES], sbuf[SECTOR_SAMPLES];
    int16_t mix[SECTOR_SAMPLES];
    int mix_pos;                    /* samples of mix[] already handed out */
    int music_vol, sfx_vol;         /* 0..256 */
} S;

static uint32_t rd32(const uint8_t *p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }

void snd_init_state(void) {
    memset(&S, 0, sizeof S);
    S.sfx_req = -1;
    S.mix_pos = SECTOR_SAMPLES;
    S.music_vol = 192;
    S.sfx_vol = 192;
}

void snd_music(const uint8_t *file, int loop) {         /* cd_smap / cd_play: NULL stops */
    xa_track *t = &S.music;
    memset(t, 0, sizeof *t);
    if (file == NULL || memcmp(file, "HMXA", 4) != 0) return;
    t->g = file + 8;
    t->n = t->end = rd32(file + 4);
    t->loop = loop;
    t->active = 1;
}

int snd_music_playing(void) { return S.music.active; }

void snd_sfx_bank(const uint8_t *file) {                /* the stage's 16 effects */
    int i;
    S.sfx.active = 0;
    S.sfx_data = NULL;
    if (file == NULL || memcmp(file, "HMSF", 4) != 0) return;
    for (i = 0; i < 17; i++) S.sfx_offs[i] = rd32(file + 4 + 4 * i);
    S.sfx_data = file + 4 + 17 * 4;
}

void snd_play_afx(int n) {                              /* playAFX */
    if (n >= 0 && n < 16) S.sfx_req = n;
}

void snd_stop_afx(void) { S.sfx_req = -1; S.sfx.active = 0; }

/* one sector: the tune's 18 groups, the effect's 18 chunks (cd_mix), mixed */
static void next_sector(void) {
    int i;
    xa_track *m = &S.music, *e = &S.sfx;
    if (S.sfx_req >= 0 && S.sfx_data) {                 /* cd_xbuf = afx[n], cd_xend = afx[n+1] */
        e->g = S.sfx_data;
        e->pos = S.sfx_offs[S.sfx_req];
        e->end = S.sfx_offs[S.sfx_req + 1];
        e->active = e->end > e->pos;
        S.sfx_req = -1;
    }
    for (i = 0; i < GROUPS_PER_SECTOR; i++) {
        if (m->active) {
            decode_group(m->g + 60 * m->pos, &m->dec, S.mbuf + 112 * i);
            if (++m->pos >= m->end) {
                if (m->loop) m->pos = 0;
                else m->active = 0;
            }
        } else memset(S.mbuf + 112 * i, 0, 112 * 2);
        if (e->active) {
            decode_group(e->g + 60 * e->pos, &e->dec, S.sbuf + 112 * i);
            e->pos++;
        } else memset(S.sbuf + 112 * i, 0, 112 * 2);
    }
    if (e->active && e->pos >= e->end) e->active = 0;   /* (checked once a sector, as cd_mix) */
    for (i = 0; i < SECTOR_SAMPLES; i++) {
        int32_t v = (S.mbuf[i] * S.music_vol + S.sbuf[i] * S.sfx_vol) >> 8;
        if (v > 32767) v = 32767;
        if (v < -32768) v = -32768;
        S.mix[i] = (int16_t)v;
    }
    S.mix_pos = 0;
}

void snd_render(int16_t *out, int samples) {
    while (samples > 0) {
        int n;
        if (S.mix_pos >= SECTOR_SAMPLES) next_sector();
        n = SECTOR_SAMPLES - S.mix_pos;
        if (n > samples) n = samples;
        memcpy(out, S.mix + S.mix_pos, n * 2);
        out += n;
        S.mix_pos += n;
        samples -= n;
    }
}
