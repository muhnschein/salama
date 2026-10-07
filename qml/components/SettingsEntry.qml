// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
import QtQuick 2.6
import Sailfish.Silica 1.0

BackgroundItem {
    id: entry

    property string iconSource
    property string text
    property string value

    width: parent.width
    height: Theme.itemSizeMedium
    opacity: enabled ? 1.0 : Theme.opacityLow

    Icon {
        id: icon

        objectName: "settingsEntryIcon"
        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeMedium
        height: width
        visible: entry.iconSource.length > 0
        source: entry.iconSource
        highlighted: entry.highlighted
    }

    Column {
        anchors {
            left: entry.iconSource.length > 0 ? icon.right : parent.left
            right: parent.right
            leftMargin: entry.iconSource.length > 0 ? Theme.paddingMedium
                                                    : Theme.horizontalPageMargin
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }

        Label {
            objectName: "settingsEntryName"
            width: parent.width
            text: entry.text
            truncationMode: TruncationMode.Fade
            color: entry.highlighted ? Theme.highlightColor : Theme.primaryColor
        }

        Label {
            objectName: "settingsEntryValue"
            width: parent.width
            visible: text.length > 0
            text: entry.value
            textFormat: Text.PlainText
            truncationMode: TruncationMode.Fade
            font.pixelSize: Theme.fontSizeExtraSmall
            color: entry.highlighted ? Theme.highlightColor : Theme.secondaryHighlightColor
        }
    }
}
