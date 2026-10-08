// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Own MouseArea above cell handler, so tap never reaches cell.
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: button

    // MouseArea clicked carries mouse event tests can't supply.
    signal clicked()

    property string markName
    readonly property bool pressed: tap.pressed
    default property alias mark: disc.data

    width: Theme.iconSizeMedium + Theme.paddingSmall
    height: width

    MouseArea {
        id: tap

        anchors.fill: parent
        onClicked: button.clicked()
    }

    Rectangle {
        id: disc

        objectName: button.markName
        anchors.centerIn: parent
        width: Theme.iconSizeExtraSmall + Theme.paddingSmall
        height: width
        radius: width / 2
        // Alpha in colour, not opacity, so mark stays opaque.
        color: Theme.rgba(Theme.overlayBackgroundColor, tap.pressed ? 1.0 : 0.5)
    }
}
