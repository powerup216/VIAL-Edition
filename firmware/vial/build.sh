#!/usr/bin/env bash
# Build the Vial firmware for the Ploopy A+ (rev2_001).
#
# Usage:  ./build.sh [path-to-vial-qmk]
#   If no path is given, vial-qmk is cloned next to this script.
# Needs: git, make, the QMK CLI / Python deps, and arm-none-eabi-gcc
#        (any normal QMK build environment, e.g. QMK MSYS or `qmk setup`).
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
VIAL_QMK="${1:-$HERE/vial-qmk}"

# drashna's community modules at the commit Ploopy's A+ firmware was written against.
DRASHNA_MODULES_COMMIT=ac529c8fe24cb66403fdf0b4a308910c705cc64c

if [ ! -d "$VIAL_QMK/.git" ]; then
    git clone --depth 1 https://github.com/vial-kb/vial-qmk "$VIAL_QMK"
fi
git -C "$VIAL_QMK" submodule update --init --depth 1 \
    lib/chibios lib/chibios-contrib lib/pico-sdk lib/printf lib/lufa lib/vusb lib/googletest lib/lvgl

if [ ! -f "$VIAL_QMK/modules/drashna/pointing_device_gestures/qmk_module.json" ]; then
    rm -rf "$VIAL_QMK/modules/drashna"
    mkdir -p "$VIAL_QMK/modules/drashna"
    git -C "$VIAL_QMK/modules/drashna" init -q
    git -C "$VIAL_QMK/modules/drashna" fetch -q --depth 1 https://github.com/drashna/qmk_modules "$DRASHNA_MODULES_COMMIT"
    git -C "$VIAL_QMK/modules/drashna" checkout -q FETCH_HEAD
fi

# Overlay the A+ sources (keyboard, shared Ploopy code, TMAG5273 wheel driver).
cp -r "$HERE/keyboards/." "$VIAL_QMK/keyboards/"

make -C "$VIAL_QMK" ploopyco/aplus/rev2_001:vial SKIP_GIT=yes -j"$(nproc 2>/dev/null || echo 4)"
cp "$VIAL_QMK/ploopyco_aplus_rev2_001_vial.uf2" "$HERE/"
echo "Built: $HERE/ploopyco_aplus_rev2_001_vial.uf2"
