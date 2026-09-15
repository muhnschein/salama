// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 tuuli contributors
//
// The only file that imports Sailfish.WebView (SCOPE.md §5): a missing engine package
// breaks browsing, not the application.
//
// The page is the top half of a deck two screens tall: browsing above, the tab grid
// below. Dragging the navigation bar upwards raises the deck and brings the grid up
// from under the page; dragging the grid past its top lowers it again. Nothing is
// pushed onto the page stack for it, so there is no sideways transition and no second
// page to come back from.
import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.WebView 1.0
import harbour.tuuli 1.0
import "../components"

WebViewPage {
    id: browserPage

    // The WebView of the active tab, or null while it is being created.
    property Item currentView: null

    // Where the deck is headed, where a finger is holding it, and where it is drawn.
    // tabsOpen is the settled answer and changes the moment a gesture commits;
    // tabsOffset is the picture, and takes the spring below to get there.
    property bool tabsOpen: false
    property bool dragging: false
    property real dragOffset: 0
    property real tabsOffset: dragging ? dragOffset : (tabsOpen ? height : 0)

    // What the engine is told to keep clear at the foot of the viewport, and what the
    // bar is drawn over. The page can then be scrolled until its own last line sits
    // above the bar instead of under it, while the bar stays translucent over what is
    // still behind it.
    readonly property real barHeight: navigationBar.height

    // Gecko's own verdict on the connection, if this engine build hands one out.
    // Only for https: a page served over http is not broken TLS, it is no TLS, and a
    // warning on every plain page is a warning nobody reads.
    readonly property bool tlsBroken: {
        if (!currentView || TabModel.activeUrl.indexOf("https://") !== 0) {
            return false
        }
        var security = currentView.security
        return !!security && !security.allGood
    }

    // How far the deck must be dragged for the gesture to commit when the finger
    // lifts. Short, because the deck follows the finger: by then the movement has
    // already shown what letting go will do.
    readonly property real pullThreshold: Theme.itemSizeLarge

    objectName: "browserPage"
    allowedOrientations: Orientation.Portrait

    // Enabled and disabled from the functions below rather than by a binding: the
    // drag ends by changing what tabsOffset is bound to, and two bindings on the same
    // property are not ordered against each other -- the spring has to be back on
    // before the target moves, not in the same breath.
    Behavior on tabsOffset {
        id: deckSpring

        NumberAnimation {
            id: deckSlide

            duration: 250
            easing.type: Easing.OutQuad
        }
    }

    function openUrl(url) {
        if (url.length === 0) {
            return
        }
        if (currentView) {
            currentView.url = url
        } else {
            TabModel.newTab(url)
        }
    }

    function updateCurrentView() {
        var loader = webViews.itemAt(TabModel.activeTabIndex)
        currentView = loader && loader.item ? loader.item : null
    }

    function ensureTab() {
        if (TabModel.count === 0) {
            TabModel.newTab(Settings.homePage)
        }
    }

    // Refresh the grid's picture of this tab before it can be seen.
    function captureCurrent() {
        if (currentView) {
            currentView.captureThumbnail()
        }
    }

    // A finger takes the deck off whatever the spring was doing with it: a disabled
    // Behavior does not stop an animation already under way, and one still running
    // would go on writing its own idea of the offset over the finger's.
    function beginDrag() {
        deckSpring.enabled = false
        deckSlide.stop()
        dragOffset = tabsOffset
        dragging = true
    }

    function dragTo(offset) {
        dragOffset = Math.max(0, Math.min(height, offset))
    }

    // A gesture that has ended: the deck goes all the way, one way or the other.
    function settle(open) {
        deckSpring.enabled = true
        tabsOpen = open
        dragging = false
    }

    // The way to the grid that needs no gesture, for the menu to call.
    function showTabs() {
        captureCurrent()
        settle(true)
    }

    Component.onCompleted: {
        ensureTab()
        updateCurrentView()
    }

    Connections {
        target: TabModel
        onActiveTabChanged: browserPage.updateCurrentView()
        onCountChanged: browserPage.ensureTab()
    }

    Item {
        id: deck

        width: parent.width
        height: parent.height * 2
        y: -browserPage.tabsOffset

        Item {
            id: browserLayer

            width: parent.width
            height: browserPage.height

            // The engine gets the whole page; the bar lies over its foot.
            Item {
                id: viewArea

                anchors.fill: parent

                // One WebView per tab that has been shown this session. Restored tabs
                // stay unloaded until first activated
                // (docs/DECISIONS/0003-one-webview-per-tab.md).
                Repeater {
                    id: webViews

                    objectName: "webViews"
                    model: TabModel
                    delegate: Loader {
                        readonly property int tabId: model.tabId
                        readonly property bool isCurrent: model.activeTab
                        readonly property bool privateTab: model.privateTab
                        readonly property string initialUrl: model.url
                        property bool shown: false

                        objectName: "webViewLoader"
                        anchors.fill: parent
                        active: shown
                        visible: isCurrent
                        sourceComponent: webViewComponent
                        onIsCurrentChanged: {
                            if (isCurrent) {
                                shown = true
                            }
                        }
                        onItemChanged: browserPage.updateCurrentView()
                        Component.onCompleted: {
                            if (isCurrent) {
                                shown = true
                            }
                        }
                    }
                }
            }

            NavigationBar {
                id: navigationBar

                objectName: "navigationBar"
                anchors {
                    bottom: parent.bottom
                    left: parent.left
                    right: parent.right
                }
                url: TabModel.activeUrl
                privateTab: TabModel.activeIsPrivate
                canGoBack: browserPage.currentView ? browserPage.currentView.canGoBack : false
                loading: browserPage.currentView ? browserPage.currentView.loading : false
                loadProgress: browserPage.currentView ? browserPage.currentView.loadProgress : 0
                tlsBroken: browserPage.tlsBroken
                onAccepted: browserPage.openUrl(Settings.urlForInput(text))
                onBack: browserPage.currentView.goBack()
                onReload: browserPage.currentView.reload()
                onStop: browserPage.currentView.stop()
                onShowMenu: pageStack.push(Qt.resolvedUrl("MenuPage.qml"), {
                                               "browserPage": browserPage
                                           })
                // The grid is about to be uncovered, so the picture of the tab being
                // left is taken before the first pixel of it shows.
                onDragStarted: {
                    browserPage.captureCurrent()
                    browserPage.beginDrag()
                }
                onDragMoved: browserPage.dragTo(distance)
                onDragFinished: browserPage.settle(distance > browserPage.pullThreshold)
            }
        }

        TabsView {
            id: tabsView

            width: parent.width
            height: browserPage.height
            y: browserPage.height
            // Nothing to draw while the page covers it, and the engine has the screen
            // to itself again for as long as that lasts.
            visible: browserPage.tabsOffset > 0
            onPullStarted: browserPage.beginDrag()
            onPulled: browserPage.dragTo(browserPage.height - distance)
            onPullFinished: browserPage.settle(distance <= browserPage.pullThreshold)
            onTabActivated: browserPage.settle(false)
        }
    }

    Component {
        id: webViewComponent

        WebView {
            id: webView

            objectName: "webView"
            active: isCurrent && Qt.application.state === Qt.ApplicationActive
                    && (browserPage.status === PageStatus.Active
                        || browserPage.status === PageStatus.Deactivating)
            privateMode: privateTab
            desktopMode: Settings.desktopMode
            downloadsEnabled: true

            // Through Binding rather than as a property of its own: footerMargin
            // belongs to Sailfish.WebView's RawWebView, and an engine build without it
            // should cost a warning in the log, not a page that fails to load.
            Binding {
                target: webView
                property: "footerMargin"
                value: browserPage.barHeight
            }

            function fetchFavicon() {
                var pageUrl = url
                runJavaScript(EngineMessages.faviconScript, function (href) {
                    TabModel.updateFavicon(tabId, EngineMessages.resolveFavicon(pageUrl, href))
                }, function () {
                    TabModel.updateFavicon(tabId, EngineMessages.defaultFavicon(pageUrl))
                })
            }

            // The model hands out a fresh file name per capture and removes the one it
            // replaces; a private tab is given none, so nothing of it reaches the disk.
            function captureThumbnail() {
                if (!isCurrent) {
                    return
                }
                var path = TabModel.thumbnailPath(tabId)
                if (path.length === 0) {
                    return
                }
                // Half size in each direction: the grab is a read back from the GPU
                // and a PNG encode, both on the way into a gesture, and the grid never
                // draws the picture wider than half the screen anyway.
                grabToImage(function (result) {
                    if (result.saveToFile(path)) {
                        TabModel.updateThumbnail(tabId, path)
                    }
                }, Qt.size(width / 2, height / 2))
            }

            onUrlChanged: TabModel.updateUrl(tabId, url)
            onTitleChanged: TabModel.updateTitle(tabId, title)
            onLoadingChanged: {
                if (!loading) {
                    fetchFavicon()
                    captureThumbnail()
                }
            }
            Component.onCompleted: url = initialUrl
        }
    }
}
