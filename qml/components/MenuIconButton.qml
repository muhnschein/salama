// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Page action in menu's top row: plain Silica icon, no disc or caption; lit while on or pressed.
import QtQuick 2.6
import Sailfish.Silica 1.0

BackgroundItem {
    id: button

    property string iconSource
    property bool checked: false
    readonly property bool lit: highlighted || checked

    height: Theme.itemSizeMedium
    opacity: enabled ? 1.0 : Theme.opacityLow
    highlightedColor: "transparent"

    Icon {
        objectName: "menuButtonIcon"
        anchors.centerIn: parent
        width: Theme.iconSizeMedium
        height: width
        source: button.iconSource
        highlighted: button.lit
    }
}
