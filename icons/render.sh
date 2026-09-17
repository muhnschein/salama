#!/bin/bash
# Regenerate the launcher icons from icons/harbour-tuuli.svg at the sizes Harbour expects.
# The PNGs are committed so the device build needs no SVG tooling.
set -euo pipefail
cd "$(dirname "$0")"
for size in 86 108 128 172; do
    mkdir -p "${size}x${size}"
    rsvg-convert -w "$size" -h "$size" harbour-tuuli.svg -o "${size}x${size}/harbour-tuuli.png"
done

# And once more into art/, which is installed beside the QML: the cover draws the icon
# itself in its icon-only style, and that needs a file an Image can resolve on the
# device. Not inside qml/ -- ci/harbour-check.sh holds that directory to QML files
# alone. Twice the largest launcher size, because a cover is half a screen wide and the
# icon is drawn across a good part of it.
mkdir -p ../art
rsvg-convert -w 344 -h 344 harbour-tuuli.svg -o ../art/harbour-tuuli.png
echo "icons rendered"
