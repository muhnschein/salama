#!/bin/bash
# Regenerate the launcher icons from icons/harbour-salama.svg at the sizes Harbour expects.
# The PNGs are committed so the device build needs no SVG tooling.
set -euo pipefail
cd "$(dirname "$0")"
for size in 86 108 128 172; do
    mkdir -p "${size}x${size}"
    rsvg-convert -w "$size" -h "$size" harbour-salama.svg -o "${size}x${size}/harbour-salama.png"
done

# And once more into art/, which is installed beside the QML: the cover draws the icon
# itself in its icon-only style, and that needs a file an Image can resolve on the
# device. Not inside qml/ -- ci/harbour-check.sh holds that directory to QML files
# alone. Twice the largest launcher size, because a cover is half a screen wide and the
# icon is drawn across a good part of it.
mkdir -p ../art
rsvg-convert -w 344 -h 344 harbour-salama.svg -o ../art/harbour-salama.png

# And the cover's own quick-action icons, icons/cover/*.svg, into art/cover/, as
# <name>-<size>-<ink>.png. The home screen draws a cover action's icon itself, from the
# file, so the picture has to arrive at the size and in the colour it is shown in: one
# per size Silica's small icon takes at the scales a phone runs at, 1.0 to 2.0, in white
# for a dark ambience and black for a light one -- the way piirit hands over its own.
# The cover picks the file (qml/cover/CoverPage.qml). The sources draw in white; the
# black ones are the same file recoloured.
mkdir -p ../art/cover
rm -f ../art/cover/*.png
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
for source in cover/*.svg; do
    name=$(basename "$source" .svg)
    sed 's/#ffffff/#000000/g' "$source" > "$work/$name-black.svg"
    for size in 32 40 48 56 64; do
        rsvg-convert -w "$size" -h "$size" "$source" -o "../art/cover/$name-$size-white.png"
        rsvg-convert -w "$size" -h "$size" "$work/$name-black.svg" \
            -o "../art/cover/$name-$size-black.png"
    done
done
echo "icons rendered"
