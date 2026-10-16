#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Pinned build used by build-dino.ps1; building never installs firmware.
set -eu
cd "$(dirname "$0")/.."
export JIELI_TOOLCHAIN=/root/.jieli/jieli-linux-toolchains-20260730.1
export AC79_SDK=build/deps/ac79
printf '%s  %s\n' c45189ee624de0430deb0db02e6dce171fb5538e7fe323e3964dc8ac6236240f "$JIELI_TOOLCHAIN/pi32v2/bin/clang" | sha256sum -c -
sha256sum -c <<'DINO_SDK'
4e3b4c220dc96641cb5a723f41e68ce41d5261ae9434bb33fbd7f2c59976ded4  build/deps/ac79/cpu/wl82/tools/uboot.boot
276579954f076886a6a7694f65dc71c034a63a2c204b76749065c0ac7b010d1b  build/deps/ac79/cpu/wl82/tools/cfg_tool.bin
41167491bffed4651750719c973d2758adeb9021a5670d02d6a53c85ed80ea7d  build/deps/ac79/cpu/wl82/tools/cfg/eq_cfg_hw.bin
DINO_SDK
sh build.sh
test -s build/felucca.fwsc
