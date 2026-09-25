// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// A bookmark to be taken for something -- the page the cover's quick action opens, the
// home page: every bookmark, narrowed to those that hold each word typed over them as
// every search in the browser narrows, and a tap picks one. The page says which and goes
// back; what the choice means is the page that asked's to decide
// (pages/CoverSettingsPage.qml, pages/HomePageSettingsPage.qml,
// docs/DECISIONS/0029-quick-action.md), and backing out of it has picked nothing.
//
// The rows are the list of bookmarks' own, without its menu: choosing is all there is to
// do with one here.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Page {
    id: pickerPage

    // What the field over the list holds, which the list is narrowed by. A property of the
    // page's because the field is the list's header, whose name the page cannot see.
    property string query

    signal bookmarkPicked(int bookmarkId, string url, string title)

    objectName: "bookmarkPickerPage"
    allowedOrientations: Orientation.Portrait

    SilicaListView {
        id: pickerList

        objectName: "bookmarkPickerList"
        anchors.fill: parent
        // Maps rather than the model's rows, so a row reads modelData. Asked again as the
        // bookmarks change, which is what reading the revision is for: a call alone would
        // never be.
        model: (BookmarkModel.revision, BookmarkModel.matching(pickerPage.query))
        header: Column {
            width: parent.width

            PageHeader {
                //: Over the list of bookmarks, when picking the one the cover's quick
                //: action opens, or the home page
                title: qsTr("Choose a bookmark")
            }

            SearchField {
                objectName: "bookmarkPickerSearch"
                width: parent.width
                placeholderText: qsTr("Search bookmarks")
                onTextChanged: pickerPage.query = text
            }
        }

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
