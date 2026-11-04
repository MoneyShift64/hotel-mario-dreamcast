/* Exercise the real streaming player/PCM path using pthread and GPU/audio mocks. */
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "hm_plat.h"
#include "movie.h"
static unsigned clock_ms,frames,uploads,audio_calls;static int mode,started;
static uint32_t palettes[4][256],saved[4][256];
void *plat_thread_start(void *(*fn)(void*),void *arg){pthread_t *t=malloc(sizeof *t);assert(t);assert(!pthread_create(t,0,fn,arg));return t;}
void plat_thread_join(void *t){pthread_join(*(pthread_t*)t,0);free(t);}
void plat_movie_yield(void){usleep(100);}
unsigned plat_movie_clock_ms(void){return clock_ms;}
void plat_sound_poll(void){}
void plat_wait_ready(void){}
void *plat_load_file(const char *name,uint32_t *size){char p[512];snprintf(p,sizeof p,"dc/build/pc/movie-fixtures/%s",name);FILE*f=fopen(p,"rb");if(!f)return 0;fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);void*d=malloc(n);assert(d);assert(fread(d,1,n,f)==(unsigned)n);fclose(f);*size=n;return d;}
void plat_save_palette(int b,uint32_t *p){memcpy(p,palettes[b],1024);}
void plat_restore_palette(int b,const uint32_t *p){memcpy(palettes[b],p,1024);}
void plat_set_colour(int b,int i,uint32_t c){palettes[b][i]=c;}
int plat_movie_texture_create(hm_tex *t){t->parts=2;return 1;}
void plat_movie_texture_update(hm_tex *t,const uint8_t *d){assert(t->parts==2&&d);uploads++;}
void plat_free_tex(hm_tex *t){t->parts=0;}
void plat_movie_sound_start(const int16_t*p,unsigned n){assert(n);movie_audio_begin(p,n);started=1;}
void plat_movie_sound_stop(void){movie_audio_end();started=0;}
int plat_soft_reset(void){return mode==2&&clock_ms>=240;}
uint32_t plat_pad(void){if(mode==1)return clock_ms<80?PAD_B1:clock_ms>=240?PAD_B1:0;return 0;}
void plat_frame_begin(void){}
void plat_draw_picture(const hm_tex *t){assert(t->parts==2);}
void plat_frame_end(void){int16_t samples[604];assert(movie_audio_render(samples,604));audio_calls++;frames++;clock_ms+=16;usleep(100);}
static unsigned prepared_calls;
static void check_prepared(void *context){assert(context==&prepared_calls);assert(!started&&!uploads);prepared_calls++;}
static void run(const char *name,int testmode,int expected,unsigned total,unsigned duration){
    clock_ms=frames=uploads=audio_calls=0;mode=testmode;
    for(int b=0;b<4;b++)for(int i=0;i<256;i++)palettes[b][i]=saved[b][i]=b*256+i;
    prepared_calls=0;
    assert(movie_play_prepared(name,check_prepared,&prepared_calls)==expected);
    assert(prepared_calls==1);
    assert(!started&&!memcmp(saved,palettes,sizeof saved));assert(frames&&uploads&&audio_calls);
    if(!mode){assert(uploads==total);assert(clock_ms>=duration);}else assert(clock_ms==240);
}
static void invalid_files(void){
    const char *root="dc/build/pc/movie-fixtures/invalid.hmv";
    FILE *f=fopen(root,"wb");assert(f);fwrite("HMV1",1,4,f);fclose(f);
    assert(movie_play("invalid")==0);
    /* Valid header/audio, deliberately oversized compressed frame. */
    f=fopen("dc/build/pc/movie-fixtures/movie_entry_1.hmv","rb");assert(f);
    unsigned char header[16];assert(fread(header,1,16,f)==16);fclose(f);
    f=fopen(root,"wb");assert(f);fwrite(header,1,16,f);
    const unsigned char packet[]={0,0,0,0,255,255,255,127};fwrite(packet,1,8,f);fclose(f);
    f=fopen("dc/build/pc/movie-fixtures/movie_entry_1.hpa","rb");assert(f);
    FILE *out=fopen("dc/build/pc/movie-fixtures/invalid.hpa","wb");assert(out);
    int c;while((c=fgetc(f))!=EOF)fputc(c,out);fclose(f);fclose(out);
    assert(movie_play("invalid")==0);assert(!started&&!memcmp(saved,palettes,sizeof saved));
}
int main(void){
    int16_t pcm[]={12,-9,34},out[8];assert(!movie_audio_render(out,8));
    movie_audio_begin(pcm,3);assert(movie_audio_render(out,8));assert(!memcmp(pcm,out,6));for(int i=3;i<8;i++)assert(!out[i]);
    assert(movie_audio_render(out,8));for(int i=0;i<8;i++)assert(!out[i]);movie_audio_end();assert(!movie_audio_render(out,8));
    assert(movie_play("missing_movie")==0);run("movie_entry_1",0,1,80,9386);run("movie_entry_1",1,1,80,9386);run("movie_entry_1",2,-1,80,9386);
    run("movie_intro",0,1,436,48853);run("movie_credits",0,1,1377,55140);mode=0;invalid_files();
    puts("Movie checks passed: complete frame chain, PCM silence at EOF, held-button release, skip/reset and palette restoration.");
}
