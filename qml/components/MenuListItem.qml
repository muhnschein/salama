// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Menu destination row, as sailfish-browser's menu rows: icon, then name. Busy: ring round icon,
// percent at row's end.
import QtQuick 2.6
import Sailfish.Silica 1.0

BackgroundItem {
    id: row

    property string iconSource
    // Sign at icon's bottom right: see BadgedIcon.
    property string badge
    property string text
    // progress 0..1.
    property bool busy: false
    property real progress: 0

    height: Theme.itemSizeSmall
    opacity: enabled ? 1.0 : Theme.opacityLow

    Item {
        id: iconSlot

        objectName: "menuButtonIconSlot"
        x: Theme.horizontalPageMargin
        anchors.verticalCenter: parent.verticalCenter
        width: Theme.iconSizeMedium
        height: width

        ProgressCircle {
            objectName: "menuButtonProgress"
            anchors.centerIn: parent
            width: Theme.iconSizeMedium + Theme.paddingMedium
            height: width
            visible: row.busy
            value: row.progress
            progressColor: Theme.highlightColor
            backgroundColor: Theme.rgba(Theme.primaryColor, Theme.opacityFaint)
            borderWidth: Theme.paddingSmall / 2
        }

        BadgedIcon {
            objectName: "menuButtonIcon"
            anchors.centerIn: parent
            source: row.iconSource
            badge: row.badge
            highlighted: row.highlighted
        }
    }

    Label {
        objectName: "menuButtonLabel"
        anchors {
            left: iconSlot.right
            leftMargin: Theme.paddingLarge
            right: percent.visible ? percent.left : parent.right
            rightMargin: percent.visible ? Theme.paddingMedium : Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        text: row.text
        truncationMode: TruncationMode.Fade
        color: row.highlighted ? Theme.highlightColor : Theme.primaryColor
    }

    Label {
        id: percent

        objectName: "menuButtonPercent"
        anchors {
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        visible: row.busy
        //: How far the running downloads are, at the end of the menu's Downloads row: "42%"
        text: qsTr("%1%").arg(Math.round(row.progress * 100))
        font.pixelSize: Theme.fontSizeSmall
        color: Theme.highlightColor
    }
}
