// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Heading in row, not list section, so two unnamed groups with same tab count stay separate.
// Compact (landscape grid cell): no heading; picture, title, host and group in one low row.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Column {
    id: delegate

    signal chosen()

    property var match: null
    property bool compact: false
    readonly property string groupText: model.groupName.length > 0
                                        ? model.groupName
                                        : qsTr("%n tab(s)", "", model.groupTabCount)

    objectName: "tabSearchDelegate"
    width: compact ? GridView.view.cellWidth : ListView.view.width

    SectionHeader {
        objectName: "tabSearchGroupHeader"
        visible: model.groupStart && !delegate.compact
        text: delegate.groupText
    }

    TabRow {
        objectName: "tabSearchItem"
        width: parent.width
        visible: !delegate.compact
        title: model.title
        subtitle: model.url
        icon: delegate.compact ? "" : model.favicon
        match: delegate.match
        onClicked: delegate.chosen()
    }

    // Only made when compact: pictures load as rows are made.
    Loader {
        width: parent.width
        active: delegate.compact
        sourceComponent: compactRow
    }

    Component {
        id: compactRow

        ListItem {
            id: row

            readonly property string host: SearchSettings.displayAddress(model.url)
            readonly property string shownTitle: model.title.length > 0 ? model.title : model.url

            objectName: "tabSearchCompactItem"
            contentHeight: Theme.itemSizeSmall
            onClicked: delegate.chosen()

            Rectangle {
                id: picture

                objectName: "tabSearchPicture"
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                height: row.contentHeight - 2 * Theme.paddingSmall
                width: Math.round(height * 3 / 2)
                clip: true
                color: Theme.rgba(Theme.highlightBackgroundColor, Theme.highlightBackgroundOpacity)

                // Top of page, as grid previews.
                Image {
                    objectName: "tabSearchThumbnail"
                    anchors.fill: parent
                    fillMode: Image.PreserveAspectCrop
                    verticalAlignment: Image.AlignTop
                    asynchronous: true
                    source: model.thumbnail.length > 0 ? "file://" + model.thumbnail : ""
                    visible: status === Image.Ready
                }

                Image {
                    anchors.centerIn: parent
                    width: Theme.iconSizeSmall
                    height: width
                    fillMode: Image.PreserveAspectFit
                    source: model.thumbnail.length > 0 ? "" : model.favicon
                }
            }

            Column {
                anchors {
                    left: picture.right
                    right: parent.right
                    leftMargin: Theme.paddingMedium
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }

                Label {
                    objectName: "tabSearchCompactTitle"
                    width: parent.width
                    text: delegate.match ? Theme.highlightText(row.shownTitle, delegate.match,
                                                               Theme.highlightColor)
                                         : row.shownTitle
                    textFormat: delegate.match ? Text.StyledText : Text.AutoText
                    truncationMode: TruncationMode.Fade
                    font.pixelSize: Theme.fontSizeSmall
                    color: row.highlighted ? Theme.highlightColor : Theme.primaryColor
                }

                Item {
                    width: parent.width
                    height: hostLabel.height

                    Label {
                        id: hostLabel

                        objectName: "tabSearchHost"
                        width: parent.width - groupLabel.width - Theme.paddingMedium
                        text: delegate.match ? Theme.highlightText(row.host, delegate.match,
                                                                   Theme.secondaryHighlightColor)
                                             : row.host
                        textFormat: delegate.match ? Text.StyledText : Text.AutoText
                        truncationMode: TruncationMode.Fade
                        font.pixelSize: Theme.fontSizeExtraSmall
                        color: row.highlighted ? Theme.secondaryHighlightColor
                                               : Theme.secondaryColor
                    }

                    Label {
                        id: groupLabel

                        objectName: "tabSearchGroup"
                        anchors.right: parent.right
                        width: Math.min(implicitWidth, parent.width / 2)
                        text: delegate.groupText
                        truncationMode: TruncationMode.Fade
                        font.pixelSize: Theme.fontSizeExtraSmall
                        color: Theme.secondaryHighlightColor
                    }
                }
            }
        }
    }
}
