// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// What the cover is drawn over: the launcher icon's bolt as a field of dots, in the
// ambience's highlight colour (docs/DECISIONS/0037-cover-is-where-you-were.md). On its
// own, with nothing to say, it is the whole cover; under what the cover says, it is faint.
//
// One picture, drawn once: icons/render.sh renders the dots white into
// art/cover/halftone.png, and they are tinted here, so they take on whatever ambience
// the phone has. The picture is taller than any cover and is cut to fill it, top and
// bottom alike, so the bolt is in the middle of whatever it is laid over. Nothing about
// it moves or is drawn again while the cover shows it.
import QtQuick 2.6
import QtGraphicalEffects 1.0
import Sailfish.Silica 1.0

Item {
    id: halftone

    /// How strongly it is drawn: 1 on its own, less under words.
    property real strength: 1

    objectName: "coverHalftone"

    // The picture is the dots' shape only; what the reader sees is the tint below.
    Image {
        id: dots

        objectName: "coverHalftoneDots"
        anchors.fill: parent
        visible: false
        fillMode: Image.PreserveAspectCrop
        smooth: true
        mipmap: true
        source: Qt.resolvedUrl("../../art/cover/halftone.png")
    }

    ColorOverlay {
        objectName: "coverHalftoneTint"
        anchors.fill: dots
        source: dots
        color: Theme.highlightColor
        opacity: halftone.strength
    }
}
