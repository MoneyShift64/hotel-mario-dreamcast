/* Separate movie PCM path. The original gameplay ADPCM decoder is untouched. */
#include <string.h>
#include "movie.h"
static const int16_t *source;
static unsigned remaining;
void movie_audio_begin(const int16_t *pcm,unsigned samples){source=pcm;remaining=samples;}
void movie_audio_end(void){source=0;remaining=0;}
int movie_audio_render(int16_t *out,unsigned samples){
    if(!source)return 0;
    unsigned n=remaining<samples?remaining:samples;
    memcpy(out,source,n*sizeof *out);memset(out+n,0,(samples-n)*sizeof *out);
    source+=n;remaining-=n;return 1;
}
