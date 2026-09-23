// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The texture of Silica's glass: the ambience's pattern, tiled a pixel to a pixel of the
// screen and drawn at a tenth, as Silica's glass material draws its pattern -- the
// keyboard's is this one. Over a tint it makes a pane of that glass rather than a flat
// colour. The material itself is in Sailfish.Silica.Background, which a Harbour
// application may not import; the pattern is the theme's.
import QtQuick 2.6
import Sailfish.Silica 1.0

Image {
    source: Theme._patternImage
    fillMode: Image.Tile
    opacity: 0.1
}
