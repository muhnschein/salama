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

# Then the halftone the cover is drawn over (qml/components/CoverHalftone.qml): the
# launcher icon's bolt as a field of dots, once, in white -- the cover tints it with the
# ambience's highlight colour itself, and fades it under what it has to say. The bolt is
# harbour-salama.svg's outline, as a polygon, 6.4 of the picture's units to the icon's,
# its bounds centred on the picture. Largest inside the bolt and thinning towards its
# foot; a halo just outside it; a faint even field everywhere else. Taller than any
# cover, so the cover fills itself with it and cuts it top and bottom alike, which keeps
# the bolt in the cover's middle whatever the cover's shape. Drawn at twice the size it
# is laid out at, so the cover only ever draws it smaller.
awk -v pitch=16 -v width=352 -v height=640 '
function inside(x, y,    i, j, hit) {
    hit = 0
    for (i = 0; i < corners; i++) {
        j = i == 0 ? corners - 1 : i - 1
        if ((py[i] > y) != (py[j] > y) &&
            x < (px[j] - px[i]) * (y - py[i]) / (py[j] - py[i]) + px[i]) {
            hit = !hit
        }
    }
    return hit
}
function distance(x, y,    i, j, dx, dy, t, ex, ey, d, best) {
    best = -1
    for (i = 0; i < corners; i++) {
        j = i == 0 ? corners - 1 : i - 1
        dx = px[i] - px[j]
        dy = py[i] - py[j]
        t = ((x - px[j]) * dx + (y - py[j]) * dy) / (dx * dx + dy * dy)
        t = t < 0 ? 0 : t > 1 ? 1 : t
        ex = px[j] + t * dx - x
        ey = py[j] + t * dy - y
        d = sqrt(ex * ex + ey * ey)
        if (best < 0 || d < best) {
            best = d
        }
    }
    return best
}
BEGIN {
    n = split("56.24 14.05 44.6 26.56 34.37 39.1 30.33 44.51 31.28 45.87 41.72 45.09 " \
              "42.01 45.54 32.97 68.93 57.35 40.05 59.49 37.2 59.08 35.84 48.27 36.62 " \
              "48.11 35.55", q, " ")
    corners = n / 2
    for (i = 0; i < corners; i++) {
        px[i] = q[2 * i + 1] * 6.4
        py[i] = q[2 * i + 2] * 6.4
        left = i == 0 || px[i] < left ? px[i] : left
        right = i == 0 || px[i] > right ? px[i] : right
        top = i == 0 || py[i] < top ? py[i] : top
        bottom = i == 0 || py[i] > bottom ? py[i] : bottom
    }
    tall = bottom - top
    dx = (width - (right - left)) / 2 - left
    dy = (height - tall) / 2 - top
    for (i = 0; i < corners; i++) {
        px[i] += dx
        py[i] += dy
    }
    top += dy
    printf "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%d\" height=\"%d\">\n", width, height
    small = 0.18 * pitch
    halo = 2.5 * pitch
    for (y = pitch / 2; y < height; y += pitch) {
        for (x = pitch / 2; x < width; x += pitch) {
            t = (y - top) / tall
            t = t < 0 ? 0 : t > 1 ? 1 : t
            big = 0.86 * pitch * (1 - 0.4 * t)
            if (inside(x, y)) {
                size = big
                strength = 1
            } else if ((d = distance(x, y)) < halo) {
                f = 1 - d / halo
                size = small + (0.75 * big - small) * f * f
                strength = 0.3 + 0.6 * f
            } else {
                size = small
                strength = 0.24 * (1 - 0.5 * y / height)
            }
            printf "<circle cx=\"%g\" cy=\"%g\" r=\"%.2f\" fill=\"#ffffff\" fill-opacity=\"%.3f\"/>\n",
                x, y, size / 2, strength
        }
    }
    print "</svg>"
}' > "$work/halftone.svg"
rsvg-convert -w 704 -h 1280 "$work/halftone.svg" -o ../art/cover/halftone.png

# The launcher icon again, larger, for the tutorial's first card, which shows it over the
# application's name (qml/components/TutorialCard.qml): drawn at an extra-large item's
# size, which is never larger than this on a phone this runs on.
rsvg-convert -w 512 -h 512 harbour-salama.svg -o ../art/logo.png

# And the made-up pages the tutorial's sketch of the tab grid shows in its cells, at
# the size the grid draws a preview: half the screen across.
mkdir -p ../art/tutorial
for source in tutorial/*.svg; do
    rsvg-convert -w 540 -h 1120 "$source" -o "../art/tutorial/$(basename "$source" .svg).png"
done
echo "icons rendered"
