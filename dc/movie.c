/* Bounded three-frame decoder queue: disc reads never run in the audio/render loop.
 * PAL8 art and PCM are decoded at build time from the original CD-i disc.
 * HMV1: magic, frame count, duration ms, texture bytes; then pts/size/zlib packets.
 * Each packet is 768 RGB bytes + XOR against the previous twiddled texture.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdatomic.h>
#ifdef __APPLE__
static void *movie_aligned_alloc(size_t n){void *p=0;return posix_memalign(&p,32,n)?NULL:p;}
#else
#include <malloc.h>
static void *movie_aligned_alloc(size_t n){return memalign(32,n);}
#endif
#include <zlib.h>
#include "hm_plat.h"
#include "movie.h"
#ifndef MOVIE_DATA_ROOT
#define MOVIE_DATA_ROOT "/cd/data"
#endif
#define TXSIZE 147456
#define RAW (TXSIZE+768)
#define MAXPACKET (RAW+1024)
#define MAXAUDIO (37800u*2u*180u)
typedef struct {unsigned pts;uint8_t *data;} frame;
typedef struct {
    FILE *file;unsigned count,duration;
    frame slots[3];
    atomic_uint produced,consumed;
    atomic_int stop,error,done;
} decoder;
static unsigned le32(const uint8_t *p){return p[0]|p[1]<<8|p[2]<<16|(uint32_t)p[3]<<24;}
static void *decode(void *arg){
    decoder *d=arg;uint8_t *packet=malloc(MAXPACKET),*previous=calloc(1,TXSIZE);
    if(!packet||!previous){d->error=1;goto end;}
    unsigned previous_pts=0;
    for(unsigned i=0;i<d->count&&!d->stop;i++){
        uint8_t header[8];
        while(d->produced-d->consumed==3&&!d->stop)plat_movie_yield();
        if(d->stop)break;
        frame *f=&d->slots[d->produced%3];
        if(fread(header,1,8,d->file)!=8){d->error=1;break;}
        unsigned pts=le32(header),size=le32(header+4);uLongf raw=RAW;
        if(size>MAXPACKET||pts>d->duration||(i&&pts<previous_pts)||fread(packet,1,size,d->file)!=size||
           uncompress(f->data,&raw,packet,size)!=Z_OK||raw!=RAW){d->error=1;break;}
        for(unsigned j=0;j<TXSIZE;j++)f->data[768+j]^=previous[j];
        memcpy(previous,f->data+768,TXSIZE);f->pts=pts;previous_pts=pts;
        /* Publish only after the frame is complete; also safe in host pthread tests. */
        d->produced=d->produced+1;
    }
end:
    free(packet);free(previous);d->done=1;return NULL;
}
int movie_play(const char *name){return movie_play_prepared(name,NULL,NULL);}
int movie_play_prepared(const char *name,void (*prepared)(void *),void *context){
    decoder d={0};uint8_t header[16];char path[128];void *worker=NULL;
    uint8_t *packed=NULL,*audio=NULL;uint32_t audio_size=0;hm_tex tex={0};
    uint32_t saved[4][256];int result=0,audio_started=0,shown=0;
    snprintf(path,sizeof path,MOVIE_DATA_ROOT "/%s.hmv",name);d.file=fopen(path,"rb");
    if(!d.file)return 0;
    if(fread(header,1,16,d.file)!=16||memcmp(header,"HMV1",4)||le32(header+12)!=TXSIZE)goto cleanup;
    d.count=le32(header+4);d.duration=le32(header+8);
    if(!d.count||d.count>20000||!d.duration||d.duration>180000)goto cleanup;
    snprintf(path,sizeof path,"%s.hpa",name);packed=plat_load_file(path,&audio_size);
    if(!packed||audio_size<8||memcmp(packed,"HPA1",4))goto cleanup;
    uLongf bytes=le32(packed+4);if(!bytes||bytes>MAXAUDIO||bytes%2)goto cleanup;
    audio=movie_aligned_alloc(bytes);if(!audio)goto cleanup;
    if(uncompress(audio,&bytes,packed+8,audio_size-8)!=Z_OK||bytes!=le32(packed+4))goto cleanup;
    free(packed);packed=NULL;
    for(int i=0;i<3;i++){d.slots[i].data=movie_aligned_alloc(RAW);if(!d.slots[i].data)goto cleanup;}
    for(int i=0;i<4;i++)plat_save_palette(i,saved[i]);
    worker=plat_thread_start(decode,&d);if(!worker)goto cleanup;
    /* Keep the existing picture visible while priming, with a responsive reset. */
    while(!d.produced&&!d.done){if(plat_soft_reset()){result=-1;goto cleanup;}plat_sound_poll();plat_movie_yield();}
    if(d.error||!d.produced)goto cleanup;
    if(!plat_movie_texture_create(&tex))goto cleanup;
    if(prepared)prepared(context);
    uint32_t held=plat_pad();unsigned start=plat_movie_clock_ms();
    plat_movie_sound_start((const int16_t*)audio,bytes/2);audio_started=1;
    result=1;
    for(;;){
        uint32_t pad=plat_pad();held&=pad;
        if(plat_soft_reset()){result=-1;break;}
        if((pad&~held)&(PAD_B1|PAD_B2|PAD_START))break;
        unsigned elapsed=plat_movie_clock_ms()-start;
        if(d.consumed<d.produced){
            frame *f=&d.slots[d.consumed%3];
            if(!shown||f->pts<=elapsed){
                plat_wait_ready();plat_movie_texture_update(&tex,f->data+768);
                for(int c=0;c<256;c++)plat_set_colour(0,c,(f->data[c*3]<<16)|(f->data[c*3+1]<<8)|f->data[c*3+2]);
                d.consumed=d.consumed+1;shown=1;
            }
        }
        if(d.error){result=0;break;}
        if(elapsed>=d.duration&&d.consumed==d.count)break;
        plat_frame_begin();if(shown)plat_draw_picture(&tex);plat_frame_end();
    }
cleanup:
    d.stop=1;if(worker)plat_thread_join(worker);
    if(audio_started)plat_movie_sound_stop();
    plat_wait_ready();plat_free_tex(&tex);
    if(worker)for(int i=0;i<4;i++)plat_restore_palette(i,saved[i]);
    for(int i=0;i<3;i++)free(d.slots[i].data);
    free(packed);free(audio);if(d.file)fclose(d.file);
    return result;
}
