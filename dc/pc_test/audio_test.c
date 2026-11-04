/* audio_test.c - play a scripted run and write the sound to a WAV (PC check of sound.c)
 *   audio_test <data dir> <H_SS> <out.wav> <script: n:keys ...>   (keys as scene.c) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "world.h"
#include "sound.h"
static uint8_t *load(const char *p) { FILE *f = fopen(p, "rb"); long n; uint8_t *d; if (!f) return 0;
    fseek(f, 0, SEEK_END); n = ftell(f); fseek(f, 0, SEEK_SET); d = malloc(n); if (fread(d, 1, n, f)) {} fclose(f); return d; }
int main(int argc, char **argv) {
    static world w; char p[512]; uint8_t *music[11] = {0};
    int hotel = argv[2][0] - '0', stage = atoi(argv[2] + 2) - 1, fld = 0, total = 0;
    FILE *out = fopen(argv[3], "wb");
    static int16_t buf[700];
    snprintf(p, sizeof p, "%s/stage_%s.inf", argv[1], argv[2]); uint8_t *inf = load(p);
    snprintf(p, sizeof p, "%s/room_%s.room", argv[1], argv[2]); uint8_t *room = load(p);
    snprintf(p, sizeof p, "%s/sfx_%s.xa", argv[1], argv[2]); uint8_t *sfx = load(p);
    for (int r = 4; r <= 10; r++) { snprintf(p, sizeof p, "%s/music_%d_%d.xa", argv[1], hotel, r); music[r] = load(p); }
    snd_init_state();
    snd_sfx_bank(sfx);
    world_start(&w, inf, room, hotel, stage, 0);
    fwrite("RIFF\0\0\0\0WAVEfmt \x10\0\0\0\x01\0\x01\0\xa8\x93\0\0\x50\x27\x01\0\x02\0\x10\0data\0\0\0\0", 1, 44, out);
    for (int k = 4; k < argc; k++) {
        int n = atoi(argv[k]); char *c = strchr(argv[k], ':'); uint32_t h = 0;
        for (c = c ? c + 1 : ""; *c; c++) {
            if (*c == 'L') h |= IN_LEFT;
            if (*c == 'R') h |= IN_RIGHT;
            if (*c == 'U') h |= IN_UP;
            if (*c == 'D') h |= IN_DOWN;
            if (*c == 'A') h |= IN_B1;
            if (*c == 'B') h |= IN_B2;
        }
        for (int i = 0; i < n; i++) {
            world_frame(&w, h);
            fld++;
            if (w.afx_req >= 0) { printf("field %d: afx %d\n", fld, w.afx_req); snd_play_afx(w.afx_req); w.afx_req = -1; }
            if (w.music_req >= 0) { printf("field %d: music %d\n", fld, w.music_req);
                snd_music(w.music_req > 0 && w.music_req < 11 ? music[w.music_req] : NULL, w.music_loop); w.music_req = -1; }
            snd_render(buf, 630);                 /* 37800 / 60 */
            fwrite(buf, 2, 630, out);
            total += 630;
        }
    }
    { uint32_t v = total * 2; fseek(out, 40, SEEK_SET); fwrite(&v, 4, 1, out); v += 36; fseek(out, 4, SEEK_SET); fwrite(&v, 4, 1, out); }
    fclose(out);
    return 0;
}
