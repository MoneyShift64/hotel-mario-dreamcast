#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p dc/build/pc/movie-fixtures
for clip in movie_entry_1 movie_intro movie_credits; do
 cp "dc/data/$clip.hmv" "dc/data/$clip.hpa" dc/build/pc/movie-fixtures/
done
${CC:-clang} -g -O1 -fsanitize=address,undefined -fno-sanitize-recover=all -Idc \
 -DMOVIE_DATA_ROOT='"dc/build/pc/movie-fixtures"' dc/pc_test/movie_test.c dc/movie.c dc/movie_audio.c \
 -lz -lpthread -o dc/build/pc/movie_test
dc/build/pc/movie_test
