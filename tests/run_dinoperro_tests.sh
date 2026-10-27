#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# SLOOP behavior and real Dinoperro LCD renderer; no hardware or flash writes.
set -eu
cd "$(dirname "$0")/.."
OUT=build/host/dinoperro
mkdir -p "$OUT" "$OUT/ui" build/dinoperro-visuals
[ -f build/gen/felucca_samples.h ] && [ -f build/gen/felucca_font.h ] || {
    echo 'Run build-dinoperro.ps1 or tools/build_dinoperro.sh first.' >&2
    exit 1
}
CC=${CC:-cc}
compile() {
    name=$1
    source=$2
    shift 2
    "$CC" -O2 -w -pthread -Ibuild/gen -Ifirmware/src -Ifirmware/hal "$@" \
        -o "$OUT/$name" "tests/$source.c" -lm
}
for name in storage_test recovery_test arranger_test seq2_test scale_test upreset_test project_test midi_uart_test encoder_test song_audio_test song_ui_test mix_split_test regress; do
    compile "$name" "$name"
done
compile ui_pages_test ui_pages_test -DFELUCCA_COSMIC=1
compile dinoperro_vis_test dinoperro_vis_test

for name in storage_test recovery_test arranger_test seq2_test scale_test upreset_test project_test midi_uart_test encoder_test; do
    "$OUT/$name"
done
"$OUT/song_audio_test" "$OUT/song.wav"
"$OUT/song_ui_test" "$OUT/song-screen.ppm"
"$OUT/ui_pages_test" "$OUT/ui"
"$OUT/dinoperro_vis_test" build/dinoperro-visuals

compile dinoperro_vis_asan dinoperro_vis_test -O1 -g -fsanitize=address,undefined \
    -fno-sanitize=shift-base,signed-integer-overflow -fno-sanitize-recover=all
"$OUT/dinoperro_vis_asan"

if command -v valgrind >/dev/null 2>&1; then export CPU_VALGRIND=1; fi
"$OUT/regress" tests/golden.txt tests/cpu_baseline.txt
CORES=1 "$OUT/mix_split_test"
CORES=2 CHAOS=7 "$OUT/mix_split_test"
CORES=2 CPU1_TIMEOUT_AT=700 "$OUT/mix_split_test"

if [ -f build/upstream-v2.8.1/build/felucca.dis ]; then
    python3 - <<'PY'
import runpy
budget = runpy.run_path("tests/target_budget.py")
def costs(path):
    result = {name: budget["cost"](code) for name, code in budget["functions"](path).items()}
    assert len(result) == len(budget["FUNCS"]), f"Missing target functions in {path}"
    return result
assert costs("build/felucca.dis") == costs("build/upstream-v2.8.1/build/felucca.dis"), "Target audio code differs from the pinned SLOOP 2.8.1 build"
print("Target audio costs match the pinned SLOOP 2.8.1 build exactly (18 functions).")
PY
else
    echo 'Target audio comparison skipped: build/upstream-v2.8.1/build/felucca.dis is absent.'
fi
python3 tools/render_dino_visuals.py build/dinoperro-visuals
python3 tests/package_dinoperro_test.py
echo 'SLOOP 2.8.1 + Dinoperro host checks passed. Target budgets and physical timing/audio are separate checks.'
