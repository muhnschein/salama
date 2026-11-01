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

    // Image = shape only; tint below is what shows. Decoded off UI thread at drawn width (art
    // taller than any cover, so width decides): full 704x1280 with mipmaps took 20-60 ms of
    // launch, 8 MB held. Not before laid out, else decoded twice.
    Image {
        id: dots

        objectName: "coverHalftoneDots"
        anchors.fill: parent
        visible: false
        fillMode: Image.PreserveAspectCrop
        smooth: true
        asynchronous: true
        sourceSize.width: width
        source: width > 0 ? Qt.resolvedUrl("../../art/cover/halftone.png") : ""
    }

    ColorOverlay {
        objectName: "coverHalftoneTint"
        anchors.fill: dots
        source: dots
        color: Theme.highlightColor
        opacity: halftone.strength
    }
}
