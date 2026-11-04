/* sound.h - Hotel Mario's music and sound effects (CD-i ADPCM, mixed in software). */
#ifndef HM_SOUND_H
#define HM_SOUND_H
#include <stdint.h>

#define SND_RATE 37800                    /* CD-i ADPCM level B */

void snd_init_state(void);
void snd_music(const uint8_t *music_xa, int loop);   /* NULL: stop */
int  snd_music_playing(void);
void snd_sfx_bank(const uint8_t *sfx_xa);            /* the stage's 16 effects */
void snd_play_afx(int n);                            /* playAFX(n) */
void snd_stop_afx(void);
void snd_render(int16_t *out, int samples);          /* mono 16-bit at SND_RATE */

/* the game's effect numbers (playAFX) */
enum {
    AFX_JUMP = 0, AFX_DOOR_CLOSE = 1, AFX_DOOR_OPEN = 2, AFX_LIFT = 3, AFX_STOMP = 5,
    AFX_COIN = 6, AFX_DYING = 7, AFX_1UP = 8, AFX_FIREBALL = 9, AFX_HURT = 10,
    AFX_POWER = 11, AFX_BUMP = 14
};

#endif
