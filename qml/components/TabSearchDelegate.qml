// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 tuuli contributors
//
// One tab in the search results: its icon, title and address, under a heading naming
// its group on the first row of each group. The heading is part of the row rather
// than a section of the list, so that two unnamed groups holding the same number of
// tabs stay two headings rather than one.
import QtQuick 2.6
import Sailfish.Silica 1.0

Column {
    id: delegate

    signal chosen()

    objectName: "tabSearchDelegate"
    width: ListView.view.width

    SectionHeader {
        objectName: "tabSearchGroupHeader"
        visible: model.groupStart
        text: model.groupName.length > 0 ? model.groupName
                                         : qsTr("%n tab(s)", "", model.groupTabCount)
    }

    ListItem {
        id: item

        objectName: "tabSearchItem"
        width: parent.width
        contentHeight: Theme.itemSizeMedium
        onClicked: delegate.chosen()

        Image {
            id: favicon

            objectName: "tabSearchFavicon"
            anchors {
                left: parent.left
                leftMargin: Theme.horizontalPageMargin
                verticalCenter: parent.verticalCenter
            }
            width: Theme.iconSizeSmall
            height: width
            fillMode: Image.PreserveAspectFit
            source: model.favicon
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
                objectName: "tabSearchTitle"
                width: parent.width
                text: model.title.length > 0 ? model.title : model.url
                truncationMode: TruncationMode.Fade
                color: item.highlighted ? Theme.highlightColor : Theme.primaryColor
            }

            Label {
                objectName: "tabSearchUrl"
                width: parent.width
                text: model.privateTab ? qsTr("Private tab") : model.url
                truncationMode: TruncationMode.Fade
                font.pixelSize: Theme.fontSizeExtraSmall
                color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
            }
        }
    }
}
