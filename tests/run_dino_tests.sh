#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# DINO host checks. Build the firmware first to generate its tables/assets.
set -eu
cd "$(dirname "$0")/.."
OUT=build/host/dino
mkdir -p "$OUT" build/dino-visuals build/dino-ui
[ -f build/gen/felucca_samples.h ] && [ -f build/gen/felucca_font.h ] || {
    echo 'Run build-dino.ps1 or build.sh first (generated firmware headers are required).' >&2
    exit 1
}
CC=${CC:-cc}
compile() {
    target=$1
    source=$2
    shift 2
    "$CC" -O2 -w -pthread -Ibuild/gen -Ifirmware/src -Ifirmware/hal "$@" -o "$OUT/$target" "tests/$source.c" -lm
}
for name in dino_live_test dino_store_test dino_ui_test dino_vis_test dino_render_test storage_test upreset_test project_test fm6_test recovery_test; do
    compile "$name" "$name"
done
compile dino_render_reference dino_render_test -DDINO_DSP_REFERENCE

"$OUT/dino_live_test"
"$OUT/dino_store_test"
"$OUT/dino_ui_test" build/dino-ui
"$OUT/dino_vis_test" build/dino-visuals
CORES=1 "$OUT/dino_render_test" > "$OUT/render-one.txt"
CORES=2 CHAOS=3 "$OUT/dino_render_test" > "$OUT/render-two.txt"
cmp "$OUT/render-one.txt" "$OUT/render-two.txt"
CORES=1 "$OUT/dino_render_test" zero > "$OUT/render-zero.txt"
CORES=1 "$OUT/dino_render_reference" zero > "$OUT/render-reference.txt"
cmp "$OUT/render-zero.txt" "$OUT/render-reference.txt"
cat "$OUT/render-one.txt" "$OUT/render-zero.txt"
for name in storage_test upreset_test project_test fm6_test recovery_test; do
    "$OUT/$name"
done

for name in dino_store_test dino_live_test dino_ui_test dino_render_test; do
    compile "${name}_asan" "$name" -O1 -g -fsanitize=address,undefined \
        -fno-sanitize=shift-base,signed-integer-overflow -fno-sanitize-recover=all
    CORES=1 "$OUT/${name}_asan"
done
node tests/dino_web_test.mjs
python3 tools/render_dino_visuals.py build/dino-visuals
python3 tools/render_dino_ui.py build/dino-ui
echo 'DINO host suite passed. Physical FM-1 timing/audio still requires a hardware test.'
