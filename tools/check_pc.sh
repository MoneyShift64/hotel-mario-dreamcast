#!/bin/bash
# SH4ZAM gate + ASan/UBSan campaign and existing world simulations.
set -euo pipefail
cd "$(dirname "$0")/.."
python3 tools/test_check_sh4zam.py
python3 tools/check_sh4zam.py
mkdir -p dc/build/pc
CC=${CC:-clang}
common=(-g -O1 -fsanitize=address,undefined -fno-sanitize-recover=all -Idc -Isrc)
sources=(dc/lifts.c dc/stage_features.c dc/secrets.c dc/world.c dc/mario.c dc/doors.c dc/fx.c dc/obj_*.c)
"$CC" "${common[@]}" dc/pc_test/campaign_test.c dc/campaign.c "${sources[@]}" -o dc/build/pc/campaign_test
dc/build/pc/campaign_test
"$CC" "${common[@]}" dc/pc_test/world_test.c "${sources[@]}" -o dc/build/pc/world_test
dc/build/pc/world_test dc/data "${1:-6000}" > dc/build/pc/world_test.log
printf 'World simulation passed; results in dc/build/pc/world_test.log\n'

"$CC" "${common[@]}" dc/pc_test/stage_features_test.c dc/campaign.c "${sources[@]}" -o dc/build/pc/stage_features_test
dc/build/pc/stage_features_test dc/data
"$CC" "${common[@]}" dc/pc_test/frontend_test.c dc/frontend.c dc/campaign.c -o dc/build/pc/frontend_test
dc/build/pc/frontend_test
"$CC" "${common[@]}" -Idc/pc_test/hostinclude dc/pc_test/frontend_ui_test.c dc/persistence.c dc/demo.c dc/frontend.c dc/campaign.c dc/sound.c "${sources[@]}" -o dc/build/pc/frontend_ui_test
dc/build/pc/frontend_ui_test dc/data
"$CC" "${common[@]}" dc/pc_test/cheats_test.c dc/campaign.c "${sources[@]}" -o dc/build/pc/cheats_test
dc/build/pc/cheats_test dc/data
python3 tools/check_audio_pause.py
python3 tools/check_lighting_render.py
"$CC" "${common[@]}" dc/pc_test/controls_test.c "${sources[@]}" -o dc/build/pc/controls_test
dc/build/pc/controls_test
"$CC" "${common[@]}" dc/pc_test/persistence_test.c dc/persistence.c dc/campaign.c -o dc/build/pc/persistence_test
dc/build/pc/persistence_test

"$CC" "${common[@]}" dc/pc_test/light_regions_test.c dc/light_regions.c "${sources[@]}" -o dc/build/pc/light_regions_test
dc/build/pc/light_regions_test dc/data
python3 tools/check_vmu_filesystem.py

bash tools/check_movies.sh
