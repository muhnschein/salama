// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
import QtQuick 2.6
import Sailfish.Silica 1.0

BackgroundItem {
    id: action

    property string iconSource
    property string title
    property string subtitle

    height: Theme.itemSizeMedium

    Icon {
        id: glyph

        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeMedium
        height: width
        source: action.iconSource
        highlighted: action.highlighted
    }

    Column {
        anchors {
            left: glyph.right
            leftMargin: Theme.paddingMedium
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }

        Label {
            objectName: "omnibarActionTitle"
            width: parent.width
            text: action.title
            textFormat: Text.PlainText
            truncationMode: TruncationMode.Fade
            color: action.highlighted ? Theme.highlightColor : Theme.primaryColor
        }

        Label {
            objectName: "omnibarActionSubtitle"
            width: parent.width
            visible: text.length > 0
            text: action.subtitle
            textFormat: Text.PlainText
            truncationMode: TruncationMode.Fade
            font.pixelSize: Theme.fontSizeExtraSmall
            color: action.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
        }
    }
}
