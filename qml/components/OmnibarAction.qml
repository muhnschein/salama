// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// One of the rows the omnibar keeps directly above the bar, in reach of the thumb that
// typed: go to what was typed as an address, or search the web for it
// (docs/DECISIONS/0027-omnibar.md). Laid out as the rows found above it are, the glyph
// where they have their icon, so that the pane reads as one list.
import QtQuick 2.6
import Sailfish.Silica 1.0

BackgroundItem {
    id: action

    property string iconSource
    property string title
    // Where the row leads, under it in the secondary colour; nothing, and the title
    // alone is centred.
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
