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
    // Landscape: one band, head at left and all nine actions in a row after it.
    readonly property SheetCells cells: SheetCells {
        width: menu.width
        height: menu.parent ? menu.parent.height : 0
    }
    readonly property bool wide: cells.wide
    readonly property real pageCell: wide ? cells.cell : width / 5
    readonly property real placeCell: wide ? cells.cell : width / 4
    readonly property real labelSize: wide ? Theme.fontSizeExtraSmall : Theme.fontSizeTiny

    signal findRequested()

    objectName: "browserMenu"
    width: parent.width
    height: content.height
    dock: Dock.Bottom
    // Shade sits one under: over page, so taps beside sheet reach it.
    z: 2

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

    SheetShade {
        objectName: "menuShade"
        edgeOf: menu
    }

    SheetGrip {
        objectName: "menuDragHandle"
        edgeOf: menu
    }

    SheetFlickable {
        id: sheet

        objectName: "menuSheet"
        width: parent.width
        height: parent.height
        contentHeight: content.height
        onDragEnded: {
            if (menu.pull > menu.closeDistance) {
                menu.hide()
            }
        }

        // Portrait: one column, head over rows. Landscape: one row of head and rows.
        Grid {
            id: content

            width: parent.width
            columns: menu.wide ? 5 : 1
            verticalItemAlignment: Grid.AlignVCenter
            topPadding: Theme.paddingMedium
            bottomPadding: Theme.paddingMedium

            MenuPageHeader {
                id: header

                width: menu.wide ? menu.cells.headWidth : parent.width
                height: menu.wide ? pageRow.height : Theme.itemSizeSmall
                wide: menu.wide
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

            // Header ink to first row's discs as far as one row's labels to next row's (#38).
            Item {
                objectName: "menuHeaderGap"
                width: parent.width
                height: Math.max(0, Theme.paddingLarge + Theme.paddingMedium - header.inkMargin)
                visible: !menu.wide
            }

            MenuSeparator {
                width: menu.cells.gap
                height: pageRow.height
                visible: menu.wide
                vertical: true
                lineName: "menuHeadDivider"
            }

            Grid {
                id: pageRow

                objectName: "menuPageRow"
                width: 5 * menu.pageCell
                columns: 5

                MenuButton {
                    objectName: "findMenuButton"
                    width: menu.pageCell
                    labelSize: menu.labelSize
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
                    width: menu.pageCell
                    labelSize: menu.labelSize
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
                    width: menu.pageCell
                    labelSize: menu.labelSize
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
                    width: menu.pageCell
                    labelSize: menu.labelSize
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
                    width: menu.pageCell
                    labelSize: menu.labelSize
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

            MenuSeparator {
                width: menu.wide ? menu.cells.gap : parent.width
                height: menu.wide ? pageRow.height : Theme.paddingLarge
                vertical: menu.wide
                lineName: "menuSeparator"
            }

            Grid {
                objectName: "menuBrowserRow"
                width: 4 * menu.placeCell
                columns: 4

                MenuButton {
                    objectName: "bookmarksMenuButton"
                    width: menu.placeCell
                    labelSize: menu.labelSize
                    iconSource: "image://theme/icon-m-favorite-selected"
                    text: qsTr("Bookmarks")
                    onClicked: menu.openPage("BookmarksPage.qml")
                }

                MenuButton {
                    objectName: "historyMenuButton"
                    width: menu.placeCell
                    labelSize: menu.labelSize
                    iconSource: "image://theme/icon-m-history"
                    text: qsTr("History")
                    onClicked: menu.openPage("HistoryPage.qml")
                }

                MenuButton {
                    objectName: "downloadsMenuButton"
                    width: menu.placeCell
                    labelSize: menu.labelSize
                    busy: DownloadModel.runningCount > 0
                    progress: DownloadModel.runningProgress / 100
                    iconSource: "image://theme/icon-m-downloads"
                    text: qsTr("Downloads")
                    onClicked: menu.openPage("DownloadsPage.qml")
                }

                MenuButton {
                    objectName: "settingsMenuButton"
                    width: menu.placeCell
                    labelSize: menu.labelSize
                    iconSource: "image://theme/icon-m-setting"
                    text: qsTr("Settings")
                    onClicked: menu.openPage("SettingsPage.qml")
                }
            }
        }
    }
}
