// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Home page: the page a new tab opens on -- the grid's plus, and the tab put in place
// of the last one closed. Its address typed, as sailfish-browser asks it; or, as
// Firefox's Home settings offer it, the page in front or a bookmark taken as it is; and
// back to the default (docs/DECISIONS/0028-settings-pages.md). A way that has nothing to
// take -- no page in front, no bookmarks, the default already -- is dimmed.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

Page {
    id: homePage

    // What the tab in front shows, when it is a page that could be a home page.
    readonly property string currentPage: /^https?:/.test(TabModel.activeUrl) ? TabModel.activeUrl
                                                                              : ""

    objectName: "homePageSettingsPage"
    allowedOrientations: Orientation.Portrait

    function chooseBookmark() {
        var picker = pageStack.push(Qt.resolvedUrl("BookmarkPickerPage.qml"))
        picker.bookmarkPicked.connect(function (bookmarkId, url) {
            Settings.homePage = url
        })
    }

    // The field shows the setting however it was last written: here, or by a way below.
    Connections {
        target: Settings
        onHomePageChanged: homePageField.text = Settings.homePage
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column

            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: qsTr("Home page")
            }

            TextField {
                id: homePageField

                objectName: "homePageField"
                width: parent.width
                //: The home page's address, typed
                label: qsTr("Address")
                placeholderText: label
                text: Settings.homePage
                inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase | Qt.ImhUrlCharactersOnly
                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: {
                    Settings.homePage = Settings.urlForInput(text)
                    focus = false
                }
            }

            SettingsEntry {
                objectName: "homePageCurrentEntry"
                enabled: homePage.currentPage.length > 0
                // The tab's own, as the grid's toolbar draws a tab.
                iconSource: "image://theme/icon-m-tabs"
                //: Makes the page in front the home page, as Firefox's Use Current Page
                text: qsTr("Use the page in front")
                onClicked: Settings.homePage = homePage.currentPage
            }

            SettingsEntry {
                objectName: "homePageBookmarkEntry"
                enabled: BookmarkModel.count > 0
                // The menu's own for the bookmarks.
                iconSource: "image://theme/icon-m-favorite"
                //: Makes a bookmark, picked from the list, the home page
                text: qsTr("Use a bookmark")
                onClicked: homePage.chooseBookmark()
            }

            SettingsEntry {
                objectName: "homePageDefaultEntry"
                enabled: Settings.homePage !== Settings.defaultHomePage()
                // The home page's own, on the way to the home page it came with.
                iconSource: "image://theme/icon-m-home"
                text: qsTr("Restore the default")
                onClicked: Settings.homePage = ""
            }
        }

        VerticalScrollDecorator {}
    }
}
