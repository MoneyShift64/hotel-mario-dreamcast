#ifndef HM_MOVIE_H
#define HM_MOVIE_H
#include <stdint.h>
/* 1 finished/skipped, 0 missing/invalid, -1 return-to-title chord. */
int movie_play(const char *name);
/* Optional loading artwork callback, after priming and before movie audio starts. */
int movie_play_prepared(const char *name,void (*prepared)(void *),void *context);
void movie_audio_begin(const int16_t *pcm, unsigned samples);
void movie_audio_end(void);
int movie_audio_render(int16_t *out, unsigned samples);
#endif
