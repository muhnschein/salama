// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Line fading out to both ends: Silica Separator fades right only, so two, left mirrored.
// Vertical: same line turned a quarter, between head and actions of a landscape sheet's band.
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: separator

    property alias lineName: line.objectName
    property bool vertical: false
    readonly property real length: vertical ? height : width
    readonly property real margin: vertical ? Theme.paddingMedium : Theme.horizontalPageMargin

    height: Theme.paddingLarge

    Row {
        id: line

        anchors.centerIn: parent
        rotation: separator.vertical ? 90 : 0

        Separator {
            width: separator.length / 2 - separator.margin
            color: Theme.primaryColor
            rotation: 180
        }

        Separator {
            width: separator.length / 2 - separator.margin
            color: Theme.primaryColor
        }
    }
}
