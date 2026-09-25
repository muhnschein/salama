#!/bin/bash
# Regenerate the launcher icons from icons/harbour-salama.svg at the sizes Harbour expects.
# The PNGs are committed so the device build needs no SVG tooling.
set -euo pipefail
cd "$(dirname "$0")"
for size in 86 108 128 172; do
    mkdir -p "${size}x${size}"
    rsvg-convert -w "$size" -h "$size" harbour-salama.svg -o "${size}x${size}/harbour-salama.png"
done

# The cover's own pictures go into art/, which is installed beside the QML so that an
# Image can resolve them on the device. Not inside qml/ -- ci/harbour-check.sh holds that
# directory to QML files alone.
#
# First its quick-action icons, icons/cover/*.svg, into art/cover/, as
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

# Then the bolt the cover is drawn with, once, in white: the cover tints it with the
# ambience's highlight colour itself (qml/components/CoverLightning.qml). Taller than a
# cover is on any phone this runs on, so it is only ever drawn smaller, and at the
# aspect the cover stretches it to, so it is not drawn out of shape on the way.
rsvg-convert -w 414 -h 1024 cover-bolt.svg -o ../art/cover/bolt.png
echo "icons rendered"
