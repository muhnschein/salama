// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// icons/render.sh renders white dots to art/cover/halftone.png; tinted here to follow
// ambience. Taller than any cover, cropped centred.
import QtQuick 2.6
import QtGraphicalEffects 1.0
import Sailfish.Silica 1.0

Item {
    id: halftone

    property real strength: 1

    objectName: "coverHalftone"

    // Image = shape only; tint below is what shows.
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
