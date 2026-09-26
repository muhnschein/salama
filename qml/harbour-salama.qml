// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "pages"

ApplicationWindow {
    id: window

    objectName: "applicationWindow"
    allowedOrientations: Orientation.Portrait
    initialPage: Component {
        BrowserPage {}
    }
    cover: Qt.resolvedUrl("cover/CoverPage.qml")

    // What the cover's quick action asks for, as the cover's settings chose it
    // (docs/DECISIONS/0029-quick-action.md). A search is the address bar opened for a new
    // tab, empty, with its pane over the page listing the bookmarks until something is
    // typed, and no tab made until something is chosen (0027-omnibar.md). The bookmarks,
    // the downloads and the history are their pages, as the menu opens them. One
    // bookmark is its page: the tab it is open in already, in whichever group, or a new
    // one; a bookmark that can no longer be found opens the bookmarks, where another is
    // one tap away.
    //
    // Here rather than on the cover because all of it is the browsing page's and the
    // page stack's, and a cover has neither in its scope. Whatever page is on top --
    // settings, a dialog -- is popped first, and what lies over the browsing page is put
    // away with it: the menu sheet, the address being edited, the grid. Each action starts
    // from the page, whatever was left open when the application was.
    function quickAction() {
        var page = pageStack.find(function (candidate) {
            return candidate.objectName === "browserPage"
        })
        if (page) {
            pageStack.pop(page)
            page.uncover()
            startQuickAction(page)
        }
        activate()
    }

    function startQuickAction(page) {
        var action = Settings.quickAction
        var bookmark = action === Settings.QuickActionBookmark ? findQuickActionBookmark() : 0
        if (action === Settings.QuickActionSearch) {
            page.openOmnibar(true)
        } else if (bookmark > 0) {
            var url = BookmarkModel.urlOf(bookmark)
            var tabId = TabModel.tabIdForUrl(url)
            if (tabId > 0) {
                TabModel.activateTabById(tabId)
            } else {
                TabModel.newTab(url)
            }
        } else if (action === Settings.QuickActionBookmarks
                   || action === Settings.QuickActionBookmark) {
            pageStack.push(Qt.resolvedUrl("pages/BookmarksPage.qml"))
        } else if (action === Settings.QuickActionDownloads) {
            pageStack.push(Qt.resolvedUrl("pages/DownloadsPage.qml"))
        } else if (action === Settings.QuickActionHistory) {
            pageStack.push(Qt.resolvedUrl("pages/HistoryPage.qml"))
        }
    }

    // The quick action's bookmark: by the id it was picked under, and once that is gone
    // by its address -- the menu's Bookmark, tapped twice, takes a bookmark away and adds
    // it back under a new id. What the setting keeps of it is brought up to date as it is
    // found, without a word, so that a bookmark given another title or address is still
    // found by it after that. 0 when neither finds one.
    function findQuickActionBookmark() {
        var id = Settings.quickActionBookmark
        if (!BookmarkModel.hasBookmark(id)) {
            id = BookmarkModel.idForUrl(Settings.quickActionBookmarkUrl)
        }
        if (id > 0) {
            Settings.setQuickActionBookmark(id, BookmarkModel.urlOf(id), BookmarkModel.titleOf(id))
        }
        return id
    }

    // A notification a page showed was tapped: its tab comes to the front, in its group,
    // over whatever was left open, and the browser with it -- where Firefox for Android
    // opens on the tab. The page hears of the tap itself
    // (docs/DECISIONS/0033-web-notifications.md).
    function showNotifiedTab(tabId) {
        var page = pageStack.find(function (candidate) {
            return candidate.objectName === "browserPage"
        })
        if (page) {
            pageStack.pop(page)
            page.uncover()
        }
        TabModel.activateTabById(tabId)
        activate()
    }

    Connections {
        target: WebNotifications
        onTabRequested: window.showNotifiedTab(tabId)
    }

    // The tutorial, over the browsing page, until it has come up once: on the first
    // start, and on the first start of a build that has it. It counts as shown as it
    // comes up, so one left by back is not forced on the reader again; Settings >
    // Tutorial shows it whenever it is asked for (docs/DECISIONS/0034-tutorial.md).
    function showTutorial() {
        Settings.tutorialShown = true
        pageStack.push(Qt.resolvedUrl("pages/TutorialPage.qml"), {}, PageStackAction.Immediate)
    }

    // Once the window is made and the browsing page is on the stack: a timer of no
    // length fires on the first turn of the event loop after that, which is Qt 5.6's
    // way of saying "next".
    Timer {
        objectName: "tutorialTimer"
        interval: 0
        running: !Settings.tutorialShown
        onTriggered: window.showTutorial()
    }

    // As the bookmarks change, and not only when the action is taken: an address edited
    // after the bookmark came back under a new id is one the old id's address would no
    // longer find.
    Connections {
        target: BookmarkModel
        onRevisionChanged: window.findQuickActionBookmark()
    }
}
