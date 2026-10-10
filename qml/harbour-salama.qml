// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "pages"

ApplicationWindow {
    id: window

    objectName: "applicationWindow"
    allowedOrientations: Orientation.Portrait | Orientation.LandscapeMask
    initialPage: Component {
        BrowserPage {}
    }
    cover: Qt.resolvedUrl("cover/CoverPage.qml")

    // Here, not on cover: needs browsing page and page stack. Top pages popped and overlays
    // cleared first. Search makes no tab until something chosen.
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
        var action = CoverSettings.quickAction
        var bookmark = action === CoverSettings.QuickActionBookmark ? findQuickActionBookmark() : 0
        if (action === CoverSettings.QuickActionSearch) {
            page.openOmnibar(true)
        } else if (bookmark > 0) {
            var url = BookmarkModel.urlOf(bookmark)
            var tabId = TabModel.tabIdForUrl(url)
            if (tabId > 0) {
                TabModel.activateTabById(tabId)
            } else {
                TabModel.newTab(url)
            }
        } else if (action === CoverSettings.QuickActionBookmarks
                   || action === CoverSettings.QuickActionBookmark) {
            pageStack.push(Qt.resolvedUrl("pages/BookmarksPage.qml"))
        } else if (action === CoverSettings.QuickActionDownloads) {
            pageStack.push(Qt.resolvedUrl("pages/DownloadsPage.qml"))
        } else if (action === CoverSettings.QuickActionHistory) {
            pageStack.push(Qt.resolvedUrl("pages/HistoryPage.qml"))
        }
    }

    // By picked id, else by address (menu Bookmark twice re-adds under new id). Setting
    // silently updated when found so renamed/re-addressed bookmark still matches. 0 if neither.
    function findQuickActionBookmark() {
        var id = CoverSettings.quickActionBookmark
        if (!BookmarkModel.hasBookmark(id)) {
            id = BookmarkModel.idForUrl(CoverSettings.quickActionBookmarkUrl)
        }
        if (id > 0) {
            CoverSettings.setQuickActionBookmark(id, BookmarkModel.urlOf(id), BookmarkModel.titleOf(id))
        }
        return id
    }

    // Page handles tap itself.
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

    function openSharedLink(url) {
        var page = pageStack.find(function (candidate) {
            return candidate.objectName === "browserPage"
        })
        if (page) {
            pageStack.pop(page)
            page.uncover()
        }
        TabModel.newTabInDefaultGroup(url)
        activate()
    }

    Connections {
        target: ShareReceiver
        onLinkShared: window.openSharedLink(url)
    }

    Component.onCompleted: ShareReceiver.setReady()

    // Counted shown on appearance so skip/back won't force it again.
    function showTutorial() {
        Settings.tutorialShown = true
        pageStack.push(Qt.resolvedUrl("pages/TutorialPage.qml"), { welcome: true },
                       PageStackAction.Immediate)
    }

    // Zero-length timer fires next event loop turn (Qt 5.6).
    Timer {
        objectName: "tutorialTimer"
        interval: 0
        running: !Settings.tutorialShown
        onTriggered: window.showTutorial()
    }

    // On bookmark change too: edited address after re-add under new id would stop matching.
    Connections {
        target: BookmarkModel
        onRevisionChanged: window.findQuickActionBookmark()
    }
}
