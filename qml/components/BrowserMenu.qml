// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Own pull: DockedPanel drag ignores pulls begun on icons, so icons sit in SilicaFlickable
// whose overscroll drives sheet. Landscape: same sheet docked right, standing on bar.
import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Share 1.0
import harbour.salama 1.0

DockedPanel {
    id: menu

    property Item view: null
    // Bar's top on page: side sheet stands on it.
    property real barTop: parent ? parent.height : 0
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
    readonly property real closeDistance: Math.min((side ? width : height) / 3,
                                                   Theme.itemSizeLarge)
    readonly property bool side: !!parent && parent.width > parent.height

    signal findRequested()

    objectName: "browserMenu"
    // Side sheet: 42% of page, whole pixels, so page still reads beside it.
    width: side ? Math.floor(parent.width * 0.42) : parent.width
    height: side ? Math.min(content.height, barTop) : content.height
    dock: side ? Dock.Right : Dock.Bottom
    // Shade sits one under: over page, so taps beside sheet reach it.
    z: 2

    // Side sheet would slide out right while its page slides out left: on screen it would stand
    // still over incoming page. Leaves with its page instead, shut once page out of sight.
    // Bottom sheet slides down as page slides away.
    property bool leaving: false

    function leave() {
        if (side) {
            leaving = true
        } else {
            hide()
        }
    }

    function openPage(page) {
        leave()
        pageStack.push(Qt.resolvedUrl("../pages/" + page))
    }

    // Page reads view via bindings that survive view going away.
    function openSiteDetails() {
        leave()
        pageStack.push(Qt.resolvedUrl("../pages/SiteDetailsPage.qml"), {
                           "url": TabModel.activeUrl,
                           "title": TabModel.activeTitle,
                           "view": menu.view
                       })
    }

    // DockedPanel leaves side sheet's y alone but zeroes it on turning, before this runs.
    function stand() {
        if (side) {
            y = barTop - height
        }
    }

    // Not once closing: DockedPanel moves it then.
    onPullChanged: {
        if (!open) {
            return
        }
        if (side) {
            x = parent.width - width + pull
        } else {
            y = parent.height - height + pull
        }
    }
    onDockChanged: stand()
    onHeightChanged: stand()
    onBarTopChanged: stand()
    onOpenChanged: stand()
    Component.onCompleted: stand()

    Connections {
        target: TabModel
        onActiveTabChanged: menu.hide()
    }

    Connections {
        target: menu.leaving ? menu.parent : null
        onStatusChanged: {
            if (menu.parent.status === PageStatus.Inactive) {
                menu.leaving = false
                menu.hide(true)
            }
        }
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

    // Head: page's name and actions, to middle of gap before browser's rows.
    SheetBackground {
        anchors.fill: parent
        headHeight: headGap.y + headGap.height / 2
        side: menu.side
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

    SheetShade {
        objectName: "menuShade"
        edgeOf: menu
    }

    SheetGrip {
        objectName: "menuDragHandle"
        edgeOf: menu
        upright: menu.side
    }

    SheetFlickable {
        id: sheet

        objectName: "menuSheet"
        width: parent.width
        height: parent.height
        across: menu.side
        contentHeight: content.height
        onDragEnded: {
            if (menu.pull > menu.closeDistance) {
                menu.hide()
            }
        }

        Column {
            id: content

            width: parent.width
            topPadding: Theme.paddingMedium
            bottomPadding: Theme.paddingMedium

            MenuPageHeader {
                id: header

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
                id: pageRow

                objectName: "menuPageRow"
                width: parent.width
                columns: 5

                MenuIconButton {
                    objectName: "findMenuButton"
                    width: menu.width / 5
                    enabled: menu.hasPage && menu.view !== null
                    iconSource: "image://theme/icon-m-search-on-page"
                    onClicked: {
                        menu.hide()
                        menu.findRequested()
                    }
                }

                MenuIconButton {
                    objectName: "bookmarkMenuButton"
                    width: menu.width / 5
                    enabled: menu.hasPage
                    checked: BookmarkModel.activeUrlBookmarked
                    // Plus to add, lit check once added; Bookmarks row below has list.
                    iconSource: checked ? "image://theme/icon-m-favorite-selected"
                                        : "image://theme/icon-m-favorite"
                    badge: checked ? "check" : "add"
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

                MenuIconButton {
                    objectName: "shareMenuButton"
                    width: menu.width / 5
                    enabled: menu.hasPage
                    iconSource: "image://theme/icon-m-share"
                    onClicked: {
                        menu.hide()
                        shareAction.trigger()
                    }
                }

                // Lasts view's life: view unloaded past loaded-page limit returns as phone.
                MenuIconButton {
                    objectName: "desktopMenuButton"
                    width: menu.width / 5
                    enabled: menu.hasPage && menu.view !== null
                    checked: menu.view !== null && menu.view.desktopMode === true
                    iconSource: "image://theme/icon-m-computer"
                    onClicked: {
                        menu.hide()
                        menu.view.desktopMode = !menu.view.desktopMode
                    }
                }

                MenuIconButton {
                    objectName: "readerMenuButton"
                    width: menu.width / 5
                    enabled: menu.reader !== null && (menu.reader.readerable || menu.reader.active)
                    checked: menu.reader !== null && menu.reader.active
                    iconSource: "image://theme/icon-m-file-formatted"
                    onClicked: {
                        menu.hide()
                        menu.reader.toggle()
                    }
                }
            }

            Item {
                id: headGap

                objectName: "menuHeadGap"
                width: parent.width
                height: Theme.paddingMedium
            }

            Column {
                objectName: "menuBrowserRow"
                width: parent.width

                MenuListItem {
                    objectName: "bookmarksMenuButton"
                    width: parent.width
                    iconSource: "image://theme/icon-m-favorite"
                    badge: "list"
                    text: qsTr("Bookmarks")
                    onClicked: menu.openPage("BookmarksPage.qml")
                }

                MenuListItem {
                    objectName: "historyMenuButton"
                    width: parent.width
                    iconSource: "image://theme/icon-m-history"
                    text: qsTr("History")
                    onClicked: menu.openPage("HistoryPage.qml")
                }

                MenuListItem {
                    objectName: "downloadsMenuButton"
                    width: parent.width
                    busy: DownloadModel.runningCount > 0
                    progress: DownloadModel.runningProgress / 100
                    iconSource: "image://theme/icon-m-downloads"
                    text: qsTr("Downloads")
                    onClicked: menu.openPage("DownloadsPage.qml")
                }

                MenuListItem {
                    objectName: "settingsMenuButton"
                    width: parent.width
                    iconSource: "image://theme/icon-m-setting"
                    text: qsTr("Settings")
                    onClicked: menu.openPage("SettingsPage.qml")
                }
            }
        }
    }
}
