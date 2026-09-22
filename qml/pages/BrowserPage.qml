// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The only file that imports Sailfish.WebView (SCOPE.md §5): a missing engine package
// breaks browsing, not the application.
//
// The page is the top half of a deck two screens tall: browsing above, the tab grid
// below. Dragging the navigation bar upwards raises the deck and brings the grid up
// from under the page; dragging the grid past its top lowers it again. Nothing is
// pushed onto the page stack, so there is no sideways transition, and nothing to come
// back from.
import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.WebView 1.0
import Sailfish.WebEngine 1.0
import harbour.salama 1.0
import "../components"

WebViewPage {
    id: browserPage

    // The WebView of the active tab, or null while it is being created.
    property Item currentView: null

    // Where the deck is headed and where a finger is holding it: tabsOpen is the
    // settled answer and changes the moment a gesture commits, tabsOffset is the
    // picture and takes the spring below to get there.
    property bool tabsOpen: false
    property bool dragging: false
    property real dragOffset: 0
    property real tabsOffset: dragging ? dragOffset : (tabsOpen ? fullHeight : 0)

    // The tallest this page has been. Silica shrinks a page while the keyboard is up,
    // and resizing the engine's view mid-animation left the content stretched until it
    // finished; the deck keeps its height and lets the keyboard cover it instead.
    property real fullHeight: 0

    onHeightChanged: {
        if (height > fullHeight) {
            fullHeight = height
        }
    }

    // The bar's height, which is height the engine's view does not get: the page ends
    // where the bar begins rather than running on behind it, in either of the two
    // heights the bar has. While the bar is between them the view is sized as it will
    // be at the slim end: a view resized on every frame of that animation is a page
    // relaid out on every frame, which is what stretched pages under the keyboard. One
    // resize, and the bar covers the difference while it moves.
    readonly property real barHeight: navigationBar.height
    readonly property real viewHeight: fullHeight - (navigationBar.compact
                                                     || navigationBar.resizing
                                                     ? navigationBar.slimHeight : barHeight)

    // What the display's own cutout takes at the top of the screen, and how much of
    // it this application keeps out of. Silica reports the cutout's whole rectangle,
    // and it is read as y plus height because a cutout need not start at the very top
    // -- sailfish-browser reads the same pair (docs/DECISIONS/0013-screen-cutout.md).
    readonly property real cutoutHeight: Screen.topCutout
                                         ? Math.max(0, Screen.topCutout.y + Screen.topCutout.height)
                                         : 0
    readonly property real cutoutInset: Settings.cutoutGuard ? cutoutHeight : 0

    // Scrolling down slims the bar to the handle and the host; scrolling back up puts
    // its controls back. The engine's own chrome gesture is the signal -- the same one
    // that used to take the whole bar off the screen -- and the view is resized with
    // the bar, so the foot of a page clears it either way.
    readonly property bool barCompact: {
        if (!currentView || navigationBar.editing || dragging) {
            return false
        }
        // undefined on an engine with no chrome gesture: then the bar stays as it is.
        return currentView.chrome === false
    }

    // Gecko's own verdict on the connection, if this engine build hands one out:
    // validState says it has one for this page, allGood weighs certificate, protocol
    // and mixed content. sailfish-browser reads the same two, and only for https.
    readonly property bool tlsBroken: {
        if (!currentView || TabModel.activeUrl.indexOf("https://") !== 0) {
            return false
        }
        var security = currentView.security
        return !!security && !!security.validState && !security.allGood
    }

    // How far the deck must be dragged for the gesture to commit when the finger lifts.
    // Short, because the movement has already shown what letting go will do.
    readonly property real pullThreshold: Theme.itemSizeLarge

    objectName: "browserPage"
    allowedOrientations: Orientation.Portrait

    // Enabled and disabled from the functions below rather than by a binding: the drag
    // ends by changing what tabsOffset is bound to, and two bindings on one property
    // are not ordered against each other -- the spring has to be on before it moves.
    Behavior on tabsOffset {
        id: deckSpring

        NumberAnimation {
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

    // What the bar asks of the current page.
    readonly property bool canGoBack: currentView ? currentView.canGoBack === true : false
    readonly property bool loading: currentView ? currentView.loading === true : false

    function goBack() {
        if (canGoBack) {
            currentView.goBack()
        }
    }

    function reloadOrStop() {
        if (!currentView) {
            return
        }
        if (currentView.loading) {
            currentView.stop()
        } else {
            currentView.reload()
        }
    }

    // Refresh the grid's picture of this tab before it can be seen.
    function captureCurrent() {
        if (currentView) {
            currentView.captureThumbnail()
        }
    }

    // The cover is made of these pictures, so the last thing this page does on the way
    // out is take one. Until now a preview was only as fresh as the last load or the
    // last time the grid was opened, which left the cover showing a page as it was
    // before it was read: scrolled somewhere else, or a step further into a site that
    // navigates without loading. Out of sight is also when the pages are put to sleep,
    // and PageActivity says when. A named function rather than the handler's body, so
    // the load tests can leave the application without a window manager to do it.
    function applicationStateChanged(state) {
        if (state !== Qt.ApplicationActive) {
            captureCurrent()
        }
        PageActivity.background = state !== Qt.ApplicationActive
    }

    // Every page that is loaded, the one in front and the ones behind it, stops its
    // timers, workers and scripts until its view is next on the screen
    // (docs/DECISIONS/0020-pages-sleep-out-of-sight.md).
    function suspendPages() {
        for (var i = 0; i < webViews.count; ++i) {
            var loader = webViews.itemAt(i)
            if (loader && loader.item) {
                loader.item.suspend()
            }
        }
    }

    // Ten minutes in the background, and the engine is asked to give back what it
    // can -- the words sailfish-browser uses after the same wait
    // (docs/DECISIONS/0016-five-live-pages.md). A named function, so the load tests
    // can ask without waiting ten minutes.
    function trimMemory() {
        WebEngine.notifyObservers(EngineMessages.memoryPressureTopic,
                                  EngineMessages.heapMinimizePayload)
    }

    // What the cover's search action ends at: a new tab, with the address field up and
    // the whole url selected, so the first key typed replaces it.
    function newTabForAddress() {
        captureCurrent()
        settle(false)
        TabModel.newTab(Settings.homePage)
        navigationBar.beginEditing()
    }

    // A finger takes the deck off whatever the spring was doing with it. Disabling the
    // Behavior does not stop an animation already under way, but the next value
    // written through it does -- the switch to dragOffset below. The animation cannot
    // be stopped by hand: it belongs to the Behavior, and Qt logs a warning and
    // ignores the call.
    function beginDrag() {
        deckSpring.enabled = false
        dragOffset = tabsOffset
        dragging = true
    }

    function dragTo(offset) {
        dragOffset = Math.max(0, Math.min(fullHeight, offset))
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

    // A touch the reach above the bar took from the foot of the page and handed back,
    // given to the engine as the touch it would have had, in the view's coordinates.
    // The view takes focus the way a real touch gives it, which ends editing the address.
    function touchPage(position, phase) {
        if (!currentView) {
            return
        }
        var at = currentView.mapFromItem(null, position.x, position.y)
        var touches = [Qt.point(at.x, at.y)]
        if (phase === "start") {
            currentView.forceActiveFocus()
            currentView.synthTouchBegin(touches)
        } else if (phase === "move") {
            currentView.synthTouchMove(touches)
        } else {
            currentView.synthTouchEnd(touches)
        }
    }

    // A tab chosen off the grid -- from the search page -- comes to the front, and the
    // page comes back over the grid with it.
    function showTab(tabId) {
        TabModel.activateTabById(tabId)
        settle(false)
    }

    // How large the engine lays a page out: 1.75 * Theme.pixelRatio is about 360 css
    // pixels across a 1080 wide screen -- the width a phone layout is written for --
    // where the platform's own 1.5 gives 410. Two functions so the load tests can
    // compare them: an expression evaluated from outside has no WebEngine import.
    function pageZoom() {
        return Math.round(Theme.pixelRatio * 1.75 / 0.5) * 0.5
    }

    function engineZoom() {
        return WebEngineSettings.pixelRatio
    }

    Connections {
        target: Qt.application
        onStateChanged: browserPage.applicationStateChanged(Qt.application.state)
    }

    Connections {
        target: PageActivity
        onAsleepChanged: {
            if (PageActivity.asleep) {
                browserPage.suspendPages()
            }
        }
    }

    // What the engine says is playing, for PageActivity to weigh: a page making a
    // sound is not put to sleep.
    Connections {
        target: WebEngine
        onRecvObserve: PageActivity.observe(message, data)
    }

    Timer {
        id: trimTimer

        objectName: "trimTimer"
        interval: 600000
        running: Qt.application.state !== Qt.ApplicationActive
        onTriggered: browserPage.trimMemory()
    }

    Component.onCompleted: {
        WebEngineSettings.pixelRatio = pageZoom()
        for (var i = 0; i < PageActivity.topics.length; ++i) {
            WebEngine.addObserver(PageActivity.topics[i])
        }
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
        height: browserPage.fullHeight * 2
        y: -browserPage.tabsOffset

        Item {
            id: browserLayer

            width: parent.width
            height: browserPage.fullHeight

            // The strip the cutout sits in, in the page's own theme colour when it
            // declares one. sailfish-browser paints the same strip the same way, from
            // a property its own web page item carries; this one asks the page
            // (docs/DECISIONS/0013-screen-cutout.md).
            Rectangle {
                objectName: "cutoutBand"
                width: parent.width
                height: browserPage.cutoutInset
                color: browserPage.currentView
                       && browserPage.currentView.pageThemeColor.length > 0
                       ? browserPage.currentView.pageThemeColor : Theme.highlightDimmerColor
            }

            // The engine gets the page between the cutout and the bar, and no
            // further. Letting it run on behind a bar that scrolled away was the
            // other answer, and on device the foot of a page was still out of reach
            // often enough to be a defect (docs/DECISIONS/0009-navigation-bar-gesture.md).
            Item {
                id: viewArea

                objectName: "viewArea"
                anchors {
                    left: parent.left
                    right: parent.right
                }
                y: browserPage.cutoutInset
                height: browserPage.viewHeight - browserPage.cutoutInset

                // One WebView per tab shown this session; restored tabs stay unloaded
                // until first activated (docs/DECISIONS/0003-one-webview-per-tab.md),
                // and a tab not among the most recently read gives its view up until
                // it is next in front, when it is loaded again from the page it was
                // on (docs/DECISIONS/0016-five-live-pages.md).
                Repeater {
                    id: webViews

                    objectName: "webViews"
                    model: TabModel
                    delegate: Loader {
                        readonly property int tabId: model.tabId
                        readonly property bool isCurrent: model.activeTab
                        readonly property bool liveTab: model.liveTab
                        readonly property string initialUrl: model.url
                        property bool shown: false

                        objectName: "webViewLoader"
                        anchors.fill: parent
                        active: shown && liveTab
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
                width: parent.width
                // The page's own height, not the layer's: Silica shrinks the page for
                // the keyboard, and the field the bar carries has to come up with it.
                y: browserPage.height - height

                url: TabModel.activeUrl
                loading: browserPage.loading
                loadProgress: browserPage.currentView ? browserPage.currentView.loadProgress : 0
                tlsBroken: browserPage.tlsBroken
                compact: browserPage.barCompact
                canGoBack: browserPage.canGoBack
                onAccepted: browserPage.openUrl(Settings.urlForInput(text))
                onBack: browserPage.goBack()
                onReloadOrStop: browserPage.reloadOrStop()
                onShowMenu: pageStack.push(Qt.resolvedUrl("MenuPage.qml"), {
                                               "browserPage": browserPage
                                           })
                // The grid is about to show, so the picture of the tab being left is
                // taken before the first pixel of it does.
                onDragStarted: {
                    browserPage.captureCurrent()
                    browserPage.beginDrag()
                }
                onDragMoved: browserPage.dragTo(distance)
                onDragFinished: browserPage.settle(distance > browserPage.pullThreshold)
                onPageTouchStarted: browserPage.touchPage(position, "start")
                onPageTouchMoved: browserPage.touchPage(position, "move")
                onPageTouchEnded: browserPage.touchPage(position, "end")
            }
        }

        TabsView {
            id: tabsView

            width: parent.width
            height: browserPage.fullHeight
            y: browserPage.fullHeight
            cutoutHeight: browserPage.cutoutInset
            // Nothing to draw while the page covers it: the engine has the screen.
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
            desktopMode: Settings.desktopMode
            downloadsEnabled: true

            // The engine's chrome gesture is what tells the bar which way a page is
            // being scrolled. The threshold is how far it must be scrolled before the
            // engine decides; its default is zero, which flips on the first pixel of
            // every drag. It is a constant rather than the bar's own height, which
            // changes when the bar answers it.
            //
            // Through Binding rather than as properties of their own, because they
            // belong to the engine's view -- a build without them should cost a
            // warning in the log, not a page that fails to load.
            Binding {
                target: webView
                property: "chromeGestureEnabled"
                value: true
            }

            Binding {
                target: webView
                property: "chromeGestureThreshold"
                value: Theme.itemSizeLarge
            }

            // The platform's WebView hands the engine a safe area for the cutout, so
            // that a page written for one can lay itself out around it. With the view
            // already below the cutout there is nothing left for a page to avoid, and
            // a page that did would be avoiding it twice.
            Binding {
                target: webView
                property: "safeAreaTop"
                value: 0
                when: browserPage.cutoutInset > 0
            }

            // What the page asks the browser to dress itself in, or nothing. Read
            // from the page because the engine keeps it to itself; the C++ side is
            // what decides whether the answer is a colour at all.
            property string pageThemeColor: ""

            function fetchThemeColor() {
                runJavaScript(EngineMessages.themeColorScript, function (value) {
                    webView.pageThemeColor = EngineMessages.themeColor(value)
                }, function () {
                    webView.pageThemeColor = ""
                })
            }

            function fetchFavicon() {
                var pageUrl = url
                runJavaScript(EngineMessages.faviconScript, function (href) {
                    TabModel.updateFavicon(tabId, EngineMessages.resolveFavicon(pageUrl, href))
                }, function () {
                    TabModel.updateFavicon(tabId, EngineMessages.defaultFavicon(pageUrl))
                })
            }

            // Asleep: its timers, workers and scripts stopped by suspendView() until
            // the view is next active, which is on the screen. Only ever out of sight:
            // Gecko draws every view into one window, and suspendView() stops that
            // window drawing -- which a view going active again starts, and nothing
            // else does (docs/DECISIONS/0020-pages-sleep-out-of-sight.md).
            property bool suspended: false

            function suspend() {
                suspended = true
                suspendView()
            }

            function resume() {
                if (suspended) {
                    suspended = false
                    resumeView()
                }
            }

            onActiveChanged: {
                if (active) {
                    resume()
                }
            }

            // The model hands out a fresh file name per capture and removes the one it
            // replaces.
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
                // draws it wider than half the screen anyway.
                grabToImage(function (result) {
                    if (result.saveToFile(path)) {
                        TabModel.updateThumbnail(tabId, path)
                    }
                }, Qt.size(width / 2, height / 2))
            }

            onUrlChanged: TabModel.updateUrl(tabId, url)
            onTitleChanged: TabModel.updateTitle(tabId, title)
            onLoadingChanged: {
                // What sleeps is a document, and one that arrives while its view is
                // asleep arrives awake -- a load already under way, a redirect, a page
                // that reloads itself. It is put to sleep with the rest, as
                // sailfish-browser does with a page that finishes loading unseen.
                if (suspended) {
                    suspendView()
                }
                if (loading) {
                    // A new page starts at the top, and the bar starts whole: it would
                    // otherwise stay slim from whatever was scrolled before it. The
                    // colour goes with the page that declared it.
                    chrome = true
                    pageThemeColor = ""
                } else {
                    fetchFavicon()
                    fetchThemeColor()
                    captureThumbnail()
                }
            }
            Component.onCompleted: url = initialUrl
        }
    }
}
