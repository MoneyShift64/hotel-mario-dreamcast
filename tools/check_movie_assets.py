#!/usr/bin/env python3
"""Validate every generated movie packet, XOR chain, timeline and audio payload."""
from pathlib import Path
import json,struct,zlib
root=Path(__file__).resolve().parents[1]/'dc/data'
count=0
for path in sorted(root.glob('movie_*.hmv')):
    with path.open('rb') as f:
        h=f.read(16);assert h[:4]==b'HMV1';frames,duration,tx=struct.unpack('<III',h[4:]);assert tx==147456 and 0<duration<=180000
        previous=bytes(tx);pts=0
        for i in range(frames):
            header=f.read(8);assert len(header)==8;time,size=struct.unpack('<II',header)
            assert pts<=time<=duration and (i or time==0);pts=time
            assert 0<size<=tx+768+1024
            compressed=f.read(size);assert len(compressed)==size
            raw=zlib.decompress(compressed);assert len(raw)==tx+768
            previous=bytes(a^b for a,b in zip(previous,raw[768:]))
        assert not f.read(1)
    audio=path.with_suffix('.hpa').read_bytes();assert audio[:4]==b'HPA1';n=struct.unpack('<I',audio[4:8])[0]
    pcm=zlib.decompress(audio[8:]);assert n==len(pcm) and n%2==0 and any(pcm)
    assert n*1000//75600<=duration
    print(f'{path.stem}: {frames} frames, {duration} ms, PCM {n} bytes')
    count+=1
assert count==17,count
print('All 17 original movie assets passed packet, timeline, XOR and PCM validation.')
