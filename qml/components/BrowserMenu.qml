// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// What the menu button on the navigation bar brings up: an opaque sheet that comes up
// from under the bar. Its head names the page in front -- icon, title, padlock and host
// -- with a button to copy its address, and a tap on the rest of it opens the page's site
// details (docs/DECISIONS/0040-site-details.md); under it the page's five actions in one row,
// each an icon on a disc with its name under it, and after a line the browser's four.
// A tap on one does it and puts the sheet away; a tap outside it puts it away alone,
// and so does pulling it back down. The same DockedPanel, on the same ground, the
// grid's list of closed tabs is, so the two sheets come and go alike
// (docs/DECISIONS/0021-menu-sheet.md). A new tab is not asked for here: the plus at
// the foot of the tab grid opens one, and so does the cover's search quick action
// (docs/DECISIONS/0029-quick-action.md).
//
// The pull is the sheet's own. On device DockedPanel's drag did not take a pull begun
// on the icons, while the list of closed tabs -- rows in a Silica list -- goes down
// with one. So the icons sit in a Silica flickable too, and a pull on them is its
// overscroll, the way the tab grid is pulled back to the page: the sheet follows the
// finger down, the flickable is moved up by as much as it draws the icons down so they
// go with the sheet, and released far enough down the sheet goes away.
import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Share 1.0
import harbour.salama 1.0

DockedPanel {
    id: menu

    // The page in front, which the page's row acts on, or null while it is made.
    property Item view: null
    readonly property bool hasPage: TabModel.activeUrl.length > 0
    // The page in front's reader view (docs/DECISIONS/0024-reader-view.md).
    readonly property QtObject reader: view !== null && view.reader ? view.reader : null
    // The engine is not satisfied with the page's connection: the bar's warning, which
    // the sheet's head repeats. Gecko's own verdict, read as the bar reads it, and not
    // for a reader view, whose document came over no connection at all.
    readonly property bool tlsBroken: {
        if (view === null || TabModel.activeUrl.indexOf("https://") !== 0
                || (reader !== null && reader.active)) {
            return false
        }
        var security = view.security
        return !!security && !!security.validState && !security.allGood
    }
    // How far a finger has the sheet pulled down past where it sits open, and how far
    // letting go puts it away rather than back: DockedPanel's own distance, a third of
    // the sheet up to about a large item's height. A flickable draws a pull past its
    // bounds at half the finger's way, as a rubber band gives; the sheet goes all of
    // it, as the list of closed tabs does.
    readonly property real pull: 2 * sheet.overscroll
    readonly property real closeDistance: Math.min(height / 3, Theme.itemSizeLarge)

    // Searching the page is done over the navigation bar, which the browsing page has.
    signal findRequested()

    objectName: "browserMenu"
    width: parent.width
    height: content.height
    dock: Dock.Bottom
    modal: true

    // A page of its own over the browsing page, which stays where it was under it.
    function openPage(page) {
        hide()
        pageStack.push(Qt.resolvedUrl("../pages/" + page))
    }

    // The site of the page in front, on a page of its own: the connection, tracking
    // protection and what the site may do (docs/DECISIONS/0040-site-details.md). It is
    // given the view to read the connection from and to load again, which it reads
    // through bindings that cope with its being gone.
    function openSiteDetails() {
        hide()
        pageStack.push(Qt.resolvedUrl("../pages/SiteDetailsPage.qml"), {
                           "url": TabModel.activeUrl,
                           "title": TabModel.activeTitle,
                           "view": menu.view
                       })
    }

    // The sheet goes down with the pull and back up as a short one springs back. Not
    // once it is being put away: DockedPanel moves it from there.
    onPullChanged: {
        if (open) {
            y = parent.height - height + pull
        }
    }

    // What the sheet offers is for the page in front, and another page in front -- a
    // new tab from the cover, say -- makes it out of date.
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

    // Opaque, as the list of closed tabs is: see-through, the page under the sheet was
    // one more thing to read past what it offers.
    SheetBackground {
        anchors.fill: parent
    }

    // Says the address is on the clipboard, as sailfish-browser says it, over the
    // navigation bar: the sheet has gone down behind the bar by then.
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

        // How far the icons are drawn down past the top of the flickable.
        readonly property real overscroll: Math.max(0, originY - contentY)

        objectName: "menuSheet"
        width: parent.width
        height: parent.height
        // Up by as much, so the icons go with the sheet and stay under the finger.
        y: -overscroll
        contentHeight: content.height
        // Vertical rather than automatic, and past its bounds: the icons fit the sheet,
        // and an automatic flickable with nothing to scroll is not dragged at all.
        flickableDirection: Flickable.VerticalFlick
        boundsBehavior: Flickable.DragOverBounds
        // The sheet is fixed-size: what this flickable is here for is the pull down, the
        // sheet's own (docs/DECISIONS/0021-menu-sheet.md), and nothing else may move.
        // A flick is out outright, so no throw carries the sheet on after the finger;
        // a drag up is met with content that will not go, so nothing reads as scrolling;
        // and no quick scroll bar is drawn over the icons (issue #38).
        maximumFlickVelocity: 0
        quickScroll: false
        onContentYChanged: {
            if (contentY > originY) {
                contentY = originY
            }
        }
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

                // High in its strip, close under the sheet's top edge (issue #38).
                DragHandle {
                    objectName: "menuDragHandle"
                    x: (parent.width - width) / 2
                    y: Theme.paddingSmall / 2
                }
            }

            // Which page the row below acts on, and the way to copy its address.
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

            // Parting the head that names the page from the page's row of actions
            // (issue #38).
            Item {
                width: parent.width
                height: Theme.paddingLarge
            }

            // All five of the page's actions in one row, each on a disc, lit while it is a
            // switch that is on. On the start page there is no page, and all five are
            // dimmed and do nothing.
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

                // Named alike on and off: lit, it says the page is a bookmark, and a tap
                // takes it out of them.
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

                // On while the page in front is in its desktop version. The only way to
                // ask for one: every page starts in its phone version. Set on the view,
                // as sailfish-browser sets it on its own
                // (apps/browser/qml/pages/components/PopUpMenuItem.qml), and the
                // engine loads the page again in the version asked for. The view keeps
                // it for as long as it lives; one given up past the limit of loaded
                // pages comes back in its phone version.
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

                // The page's article alone, in Firefox's reader view. Offered when
                // Readability finds the page reads as one, and on while it is shown,
                // when a tap goes back to the page, as back does.
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

            // Between the page's actions and the browser's, a line that fades out towards
            // both ends. Silica's Separator fades towards one, its right, so the line is
            // two of them end to end, the left one turned about.
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

                // A ring round the icon while something is coming, filling as the
                // downloads under way go along together.
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
