#!/bin/bash
# Build the Hotel Mario Dreamcast test: hotel_mario_test.cdi
# Needs KallistiOS with SH4ZAM (kos-ports) and mkdcdisc, like the other projects.
#     bash dc/build_dc.sh
set -e
cd "$(dirname "$0")"
python3 ../tools/check_sh4zam.py
if ! command -v kos-cc >/dev/null 2>&1; then
    if [ -f /opt/toolchains/dc/kos/environ.sh ]; then
        source /opt/toolchains/dc/kos/environ.sh
    else
        echo "Can't find KallistiOS (/opt/toolchains/dc/kos/environ.sh)"; exit 1
    fi
fi
if [ ! -f data/stage_1_01.tex ]; then
    echo "dc/data is empty: the converted game files are missing (see README)"; exit 1
fi
PORTS_DIR=${KOS_PORTS:-/opt/toolchains/dc/kos-ports}
mkdir -p build disc/data
rm -f hotel_mario_test.elf hotel_mario_test.cdi
OBJS=""
for f in hm_plat_dc.c movie.c movie_audio.c doors.c world.c light_regions.c lifts.c stage_features.c secrets.c frontend.c persistence.c demo.c vmu_store_dc.c campaign.c mario.c fx.c sound.c obj_*.c test_main.c; do
    o=build/$(basename "${f%.c}").o
    kos-cc -O2 -ffast-math -Wall -Wno-unused-function -I../src -I"$PORTS_DIR/zlib/inst/include" -c "$f" -o "$o"
    OBJS="$OBJS $o"
done
kos-cc -o hotel_mario_test.elf $OBJS -L"$PORTS_DIR/lib" -lsh4zam -lz -lm
cp data/* disc/data/
M=$(command -v mkdcdisc || find "$HOME/dreamcast-tools" "$HOME/dreamcast-projects" /opt /usr/local -name mkdcdisc -type f 2>/dev/null | head -1)
"$M" -e hotel_mario_test.elf -D disc -n "HOTEL MARIO" -o hotel_mario_test.cdi --allow-overwrite
ls -la hotel_mario_test.cdi
