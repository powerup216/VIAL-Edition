#!/usr/bin/env bash
# Regenerate the keymap sheet: docs/img/keymap_vial.pdf and .png
# Needs python3, wkhtmltopdf, poppler-utils (pdftoppm) and the Montserrat font.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
IMG="$HERE/../../../docs/img"
python3 "$HERE/make_keymap.py" >/dev/null
wkhtmltopdf -q --page-size Letter -T 0 -B 0 -L 0 -R 0 --disable-smart-shrinking \
    "$HERE/keymap_vial.html" "$IMG/keymap_vial.pdf"
pdftoppm -r 120 -png -singlefile "$IMG/keymap_vial.pdf" "$IMG/keymap_vial"
rm -f "$HERE/keymap_vial.html"
echo "Wrote $IMG/keymap_vial.pdf and $IMG/keymap_vial.png"
