// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
// Picks bookmark for caller (CoverSettingsPage.qml); backing out picks nothing.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Page {
    id: pickerPage

    property string query

    signal bookmarkPicked(int bookmarkId, string url, string title)

    objectName: "bookmarkPickerPage"
    allowedOrientations: Orientation.Portrait | Orientation.LandscapeMask

    // Field above list, not its header: each keystroke replaces model, list then focuses its
    // new current item (field loses keyboard) and scrolls content under field.
    Column {
        id: heading

        objectName: "bookmarkPickerHeading"
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }

        PageHeader {
            //: Over the list of bookmarks, when picking the one the cover's quick
            //: action opens
            title: qsTr("Choose a bookmark")
        }

        SearchField {
            objectName: "bookmarkPickerSearch"
            width: parent.width
            placeholderText: qsTr("Search bookmarks")
            onTextChanged: pickerPage.query = text
        }
    }

    SilicaListView {
        id: pickerList

        objectName: "bookmarkPickerList"
        anchors {
            top: heading.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        // Rows otherwise draw over field when scrolled.
        clip: true
        // Maps so row reads modelData. Revision read -> re-query on bookmark change.
        model: (BookmarkModel.revision, BookmarkModel.matching(pickerPage.query))

        delegate: ListItem {
            id: row

            objectName: "bookmarkPickerRow"
            contentHeight: Theme.itemSizeMedium
            onClicked: {
                pickerPage.bookmarkPicked(modelData.bookmarkId, modelData.url, modelData.title)
                pageStack.pop()
            }

            Image {
                id: favicon

                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                width: Theme.iconSizeSmall
                height: width
                fillMode: Image.PreserveAspectFit
                source: modelData.favicon
            }

            Column {
                anchors {
                    left: favicon.right
                    leftMargin: Theme.paddingMedium
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }

                Label {
                    objectName: "bookmarkPickerTitle"
                    width: parent.width
                    textFormat: Text.PlainText
                    text: modelData.title
                    truncationMode: TruncationMode.Fade
                    color: row.highlighted ? Theme.highlightColor : Theme.primaryColor
                }

                Label {
                    width: parent.width
                    textFormat: Text.PlainText
                    text: modelData.url
                    truncationMode: TruncationMode.Fade
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: row.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                }
            }
        }

        ViewPlaceholder {
            objectName: "bookmarkPickerPlaceholder"
            enabled: pickerList.count === 0
            text: pickerPage.query.length > 0 ? qsTr("No matches") : qsTr("No bookmarks")
        }

        VerticalScrollDecorator {}
    }
}
