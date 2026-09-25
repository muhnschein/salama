// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// One tab as a row of a list: its icon, its title and its address. What the search
// results and the recently closed tabs are made of, and the start page's pages read
// last.
import QtQuick 2.6
import Sailfish.Silica 1.0

ListItem {
    id: row

    property string title
    property string subtitle
    property string icon

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
            text: row.title.length > 0 ? row.title : row.subtitle
            truncationMode: TruncationMode.Fade
            color: row.highlighted ? Theme.highlightColor : Theme.primaryColor
        }

        Label {
            objectName: "tabRowSubtitle"
            width: parent.width
            text: row.subtitle
            truncationMode: TruncationMode.Fade
            font.pixelSize: Theme.fontSizeExtraSmall
            color: row.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
        }
    }
}
