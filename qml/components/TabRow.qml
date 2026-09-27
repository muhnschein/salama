// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// One tab as a row of a list: its icon, its title and its address. What the search
// results and the recently closed tabs are made of, and the start page's pages read
// last. A row that answers a search lights what was searched for in its title and its
// address, as Silica's own search results light it: Theme.highlightText(), which hands
// the text back as StyledText with every match in the colour given -- the highlight
// colour in the title, and its secondary in the address, as the two lines are coloured
// under a finger.
import QtQuick 2.6
import Sailfish.Silica 1.0

ListItem {
    id: row

    property string title
    property string subtitle
    property string icon
    // What was searched for, as Theme.highlightText() takes it -- a string, or a RegExp
    // as Jolla's own contacts and media player hand it one; null for a row that answers
    // no search, whose lines are drawn as they are.
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
