#!/usr/bin/env python3
"""Host-check the actual platform callback's pause branch with the real decoder."""
from pathlib import Path
import os, subprocess, sys
root=Path(__file__).resolve().parents[1]
build=root/'dc/build/pc';build.mkdir(parents=True,exist_ok=True)
platform=(root/'dc/hm_plat_dc.c').read_text()
callback=platform[platform.index('static int sSoundPaused;'):platform.index('void plat_sound_init(void)')]
source='''#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sound.c"
#include "movie_audio.c"
typedef int snd_stream_hnd_t;
#define SND_STREAM_INVALID -1
'''+callback+'''
int main(int argc,char **argv) {
    assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);
    fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);unsigned char*music=malloc(n);
    assert(music && fread(music,1,n,f)==(size_t)n);fclose(f);
    int16_t first[512],second[512];
    snd_init_state();snd_music(music,1);snd_render(first,512);snd_render(second,512);
    snd_init_state();snd_music(music,1);int received;
    int16_t*out=sound_cb(0,1024,&received);
    assert(received==1024 && !memcmp(first,out,1024));
    unsigned char saved[sizeof S];memcpy(saved,&S,sizeof S);
    plat_sound_pause(1);
    for(int i=0;i<50;i++) {
        out=sound_cb(0,4096,&received);assert(received==4096);
        for(int j=0;j<received/2;j++)assert(!out[j]);
        assert(!memcmp(saved,&S,sizeof S));
    }
    plat_sound_pause(0);
    int16_t movie_pcm[3]={123,-456,789};movie_audio_begin(movie_pcm,3);
    out=sound_cb(0,1024,&received);assert(received==1024);
    assert(!memcmp(out,movie_pcm,sizeof movie_pcm));
    for(int j=3;j<received/2;j++)assert(!out[j]);
    assert(!memcmp(saved,&S,sizeof S));
    movie_audio_end();out=sound_cb(0,1024,&received);
    assert(received==1024 && !memcmp(second,out,1024));free(music);
    puts("Audio pause passed: actual callback emits silence, retains all decoder state, movie PCM leaves gameplay decoder unchanged, resumes sample-exactly.");
}
'''
path=build/'audio_pause_test.c';path.write_text(source)
subprocess.run([os.environ.get('CC','clang'),'-g','-O1','-fsanitize=address,undefined','-fno-sanitize-recover=all','-I'+str(root/'dc'),str(path),'-o',str(build/'audio_pause_test')],check=True)
subprocess.run([str(build/'audio_pause_test'),str(root/'dc/data/music_7_5.xa')],check=True)
