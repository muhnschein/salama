// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Own pull: DockedPanel drag ignores pulls begun on icons, so icons sit in a SilicaFlickable
// whose overscroll drives sheet.
import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Share 1.0
import harbour.salama 1.0

DockedPanel {
    id: menu

    property Item view: null
    readonly property bool hasPage: TabModel.activeUrl.length > 0
    readonly property QtObject reader: view !== null && view.reader ? view.reader : null
    // Never for reader view: no connection behind it.
    readonly property bool tlsBroken: {
        if (view === null || TabModel.activeUrl.indexOf("https://") !== 0
                || (reader !== null && reader.active)) {
            return false
        }
        var security = view.security
        return !!security && !!security.validState && !security.allGood
    }
    // Overscroll is half finger travel (rubber band); x2 so sheet follows finger.
    readonly property real pull: 2 * sheet.overscroll
    readonly property real closeDistance: Math.min(height / 3, Theme.itemSizeLarge)

    signal findRequested()

    objectName: "browserMenu"
    width: parent.width
    height: content.height
    dock: Dock.Bottom
    modal: true

    function openPage(page) {
        hide()
        pageStack.push(Qt.resolvedUrl("../pages/" + page))
    }

    // Page reads view via bindings that survive view going away.
    function openSiteDetails() {
        hide()
        pageStack.push(Qt.resolvedUrl("../pages/SiteDetailsPage.qml"), {
                           "url": TabModel.activeUrl,
                           "title": TabModel.activeTitle,
                           "view": menu.view
                       })
    }

    // Not once closing: DockedPanel moves it then.
    onPullChanged: {
        if (open) {
            y = parent.height - height + pull
        }
    }

    Connections {
        target: TabModel
        onActiveTabChanged: menu.hide()
    }

    ShareAction {
        id: shareAction

        objectName: "shareAction"
        mimeType: "text/x-url"
        resources: [{
                "type": "text/x-url",
                "linkTitle": TabModel.activeTitle,
                "status": TabModel.activeUrl
            }]
    }

    SheetBackground {
        anchors.fill: parent
    }

    // Over nav bar: sheet already hidden by then.
    Notice {
        id: copiedNotice

        objectName: "addressCopiedNotice"
        duration: Notice.Short
        verticalOffset: -Theme.itemSizeLarge
        //: Shown for a moment once the menu's copy button has put the page's address on
        //: the clipboard
        text: qsTr("Address copied")
    }

    SilicaFlickable {
        id: sheet

        readonly property real overscroll: Math.max(0, originY - contentY)

        objectName: "menuSheet"
        width: parent.width
        height: parent.height
        // Cancels overscroll so icons stay under finger.
        y: -overscroll
        contentHeight: content.height
        // Not AutoFlick: content fits, and auto with nothing to scroll won't drag.
        flickableDirection: Flickable.VerticalFlick
        boundsBehavior: Flickable.DragOverBounds
        onDragEnded: {
            if (menu.pull > menu.closeDistance) {
                menu.hide()
            }
        }

        Column {
            id: content

            width: parent.width
            bottomPadding: Theme.paddingMedium

            Item {
                width: parent.width
                height: Theme.paddingLarge

                DragHandle {
                    objectName: "menuDragHandle"
                    x: (parent.width - width) / 2
                    y: Theme.paddingSmall
                }
            }

            MenuPageHeader {
                width: parent.width
                url: TabModel.activeUrl
                title: TabModel.activeTitle
                favicon: TabModel.activeFavicon
                tlsBroken: menu.tlsBroken
                onDetailsRequested: menu.openSiteDetails()
                onCopyRequested: {
                    menu.hide()
                    Clipboard.text = TabModel.activeUrl
                    copiedNotice.show()
                }
            }

            Grid {
                objectName: "menuPageRow"
                width: parent.width
                columns: 5

                MenuButton {
                    objectName: "findMenuButton"
                    width: menu.width / 5
                    round: true
                    enabled: menu.hasPage && menu.view !== null
                    iconSource: "image://theme/icon-m-search-on-page"
                    text: qsTr("Find in page")
                    onClicked: {
                        menu.hide()
                        menu.findRequested()
                    }
                }

                MenuButton {
                    objectName: "bookmarkMenuButton"
                    width: menu.width / 5
                    round: true
                    enabled: menu.hasPage
                    checked: BookmarkModel.activeUrlBookmarked
                    iconSource: checked ? "image://theme/icon-m-favorite-selected"
                                        : "image://theme/icon-m-favorite"
                    text: qsTr("Bookmark")
                    onClicked: {
                        menu.hide()
                        if (BookmarkModel.activeUrlBookmarked) {
                            BookmarkModel.removeByUrl(TabModel.activeUrl)
                        } else {
                            BookmarkModel.add(TabModel.activeUrl, TabModel.activeTitle,
                                              TabModel.activeFavicon)
                        }
                    }
                }

                MenuButton {
                    objectName: "shareMenuButton"
                    width: menu.width / 5
                    round: true
                    enabled: menu.hasPage
                    iconSource: "image://theme/icon-m-share"
                    text: qsTr("Share")
                    onClicked: {
                        menu.hide()
                        shareAction.trigger()
                    }
                }

                // Lasts view's life: view unloaded past loaded-page limit returns as phone.
                MenuButton {
                    objectName: "desktopMenuButton"
                    width: menu.width / 5
                    round: true
                    enabled: menu.hasPage && menu.view !== null
                    checked: menu.view !== null && menu.view.desktopMode === true
                    iconSource: "image://theme/icon-m-computer"
                    text: qsTr("Desktop site")
                    onClicked: {
                        menu.hide()
                        menu.view.desktopMode = !menu.view.desktopMode
                    }
                }

                MenuButton {
                    objectName: "readerMenuButton"
                    width: menu.width / 5
                    round: true
                    enabled: menu.reader !== null && (menu.reader.readerable || menu.reader.active)
                    checked: menu.reader !== null && menu.reader.active
                    iconSource: "image://theme/icon-m-file-formatted"
                    text: qsTr("Reader view")
                    onClicked: {
                        menu.hide()
                        menu.reader.toggle()
                    }
                }
            }

            // Fades both ends: Silica Separator fades right only, so two, left one mirrored.
            Item {
                width: parent.width
                height: Theme.paddingLarge

                Row {
                    objectName: "menuSeparator"
                    anchors.centerIn: parent

                    Separator {
                        width: menu.width / 2 - Theme.horizontalPageMargin
                        color: Theme.primaryColor
                        rotation: 180
                    }

                    Separator {
                        width: menu.width / 2 - Theme.horizontalPageMargin
                        color: Theme.primaryColor
                    }
                }
            }

            Grid {
                objectName: "menuBrowserRow"
                width: parent.width
                columns: 4

                MenuButton {
                    objectName: "bookmarksMenuButton"
                    width: menu.width / 4
                    iconSource: "image://theme/icon-m-favorite-selected"
                    text: qsTr("Bookmarks")
                    onClicked: menu.openPage("BookmarksPage.qml")
                }

                MenuButton {
                    objectName: "historyMenuButton"
                    width: menu.width / 4
                    iconSource: "image://theme/icon-m-history"
                    text: qsTr("History")
                    onClicked: menu.openPage("HistoryPage.qml")
                }

                MenuButton {
                    objectName: "downloadsMenuButton"
                    width: menu.width / 4
                    busy: DownloadModel.runningCount > 0
                    progress: DownloadModel.runningProgress / 100
                    iconSource: "image://theme/icon-m-downloads"
                    text: qsTr("Downloads")
                    onClicked: menu.openPage("DownloadsPage.qml")
                }

                MenuButton {
                    objectName: "settingsMenuButton"
                    width: menu.width / 4
                    iconSource: "image://theme/icon-m-setting"
                    text: qsTr("Settings")
                    onClicked: menu.openPage("SettingsPage.qml")
                }
            }
        }
    }
}
