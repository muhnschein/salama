// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Line fading out to both ends: Silica Separator fades right only, so two, left mirrored.
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: separator

    property alias lineName: line.objectName

    height: Theme.paddingLarge

    Row {
        id: line

        anchors.centerIn: parent

        Separator {
            width: separator.width / 2 - Theme.horizontalPageMargin
            color: Theme.primaryColor
            rotation: 180
        }

        Separator {
            width: separator.width / 2 - Theme.horizontalPageMargin
            color: Theme.primaryColor
        }
    }
}
