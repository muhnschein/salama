// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
import QtQuick 2.6
import Sailfish.Silica 1.0

ListItem {
    id: row

    property string title
    property string subtitle
    property string icon
    // String or RegExp; null = no search.
    property var match: null
    readonly property string shownTitle: title.length > 0 ? title : subtitle

    contentHeight: Theme.itemSizeMedium

    Image {
        id: favicon

        objectName: "tabRowIcon"
        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeSmall
        height: width
        fillMode: Image.PreserveAspectFit
        source: row.icon
    }

    Column {
        anchors {
            left: favicon.right
            right: parent.right
            leftMargin: Theme.paddingMedium
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }

        Label {
            objectName: "tabRowTitle"
            width: parent.width
            text: row.match ? Theme.highlightText(row.shownTitle, row.match, Theme.highlightColor)
                            : row.shownTitle
            textFormat: row.match ? Text.StyledText : Text.AutoText
            truncationMode: TruncationMode.Fade
            color: row.highlighted ? Theme.highlightColor : Theme.primaryColor
        }

        Label {
            objectName: "tabRowSubtitle"
            width: parent.width
            text: row.match ? Theme.highlightText(row.subtitle, row.match,
                                                  Theme.secondaryHighlightColor)
                            : row.subtitle
            textFormat: row.match ? Text.StyledText : Text.AutoText
            truncationMode: TruncationMode.Fade
            font.pixelSize: Theme.fontSizeExtraSmall
            color: row.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
        }
    }
}
