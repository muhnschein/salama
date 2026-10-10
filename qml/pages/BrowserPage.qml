// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
// Only file importing Sailfish.WebView (SCOPE.md §5): missing engine breaks browsing, not app.
import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.WebView 1.0
import Sailfish.WebEngine 1.0
import harbour.salama 1.0
import "../components"

WebViewPage {
    id: browserPage

    // null while creating or on start page.
    property Item currentView: null
    property Item currentLoader: null
    property NotificationCenter notifications: NotificationCenter { page: browserPage }
    property EnginePreferences preferences: EnginePreferences {}

    property alias tabsOpen: deck.tabsOpen
    property alias dragging: deck.dragging
    property alias tabsOffset: deck.tabsOffset
    property alias fullHeight: deck.fullHeight
    property alias pullThreshold: deck.pullThreshold

    // Mid-animation view sized to slim end: per-frame resize relaid out page (keyboard stretch).
    readonly property real barHeight: navigationBar.height
    readonly property real viewHeight: fullHeight - banners.height
                                       - (navigationBar.compact || navigationBar.resizing
                                          ? navigationBar.slimHeight : barHeight)

    readonly property CutoutInsets cutout: CutoutInsets {
        view: browserPage.currentView
        orientation: browserPage.orientation
    }
    readonly property real cutoutHeight: cutout.height
    readonly property real cutoutInset: cutout.inset
    readonly property real pageCutoutInset: cutout.pageInset

    // Not during deck drag: mid-drag resize relaid out page.
    readonly property bool barCompact: {
        if (!currentView || navigationBar.editing || findBar.active || Settings.fixedToolbar) {
            return false
        }
        // undefined on engine without chrome gesture -> bar unchanged.
        return currentView.chrome === false
    }

    objectName: "browserPage"
    allowedOrientations: Orientation.Portrait | Orientation.LandscapeMask

    function openUrl(url) {
        if (url.length === 0) {
            return
        }
        if (currentView) {
            currentView.url = url
        } else if (startPageLayer.active && currentLoader) {
            currentLoader.openFromStartPage(url)
        } else {
            TabModel.newTab(url)
        }
    }

    function updateCurrentView() {
        var loader = webViews.itemAt(TabModel.activeTabIndex)
        currentLoader = loader ? loader : null
        currentView = loader && loader.item ? loader.item : null
    }

    function ensureTab() {
        if (TabModel.count === 0) {
            TabModel.newTab("")
        }
    }

    readonly property bool canGoBack: currentView ? currentView.canGoBack === true
                                                    || currentLoader !== null
                                                    && currentLoader.startPageBehind : false
    readonly property bool loading: currentView ? currentView.loading === true : false

    function goBack() {
        if (currentView && currentView.canGoBack) {
            currentView.goBack()
        } else if (canGoBack) {
            currentLoader.backToStartPage()
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

    function captureCurrent() {
        if (currentView) {
            currentView.captureThumbnail()
        } else {
            startPageLayer.capture()
        }
    }

    // Capture on leaving so cover is fresh. Named so load tests can call without window manager.
    function applicationStateChanged(state) {
        if (state !== Qt.ApplicationActive) {
            captureCurrent()
        }
        PageActivity.background = state !== Qt.ApplicationActive
    }

    // Notification-allowed pages stay awake.
    function suspendPages() {
        for (var i = 0; i < webViews.count; ++i) {
            var loader = webViews.itemAt(i)
            if (loader && loader.item && !NotificationPermissions.isAllowed(String(loader.item.url))) {
                loader.item.suspend()
            }
        }
    }

    // Named so load tests skip 10 min wait.
    function trimMemory() {
        WebEngine.notifyObservers(EngineMessages.memoryPressureTopic,
                                  EngineMessages.heapMinimizePayload)
    }

    function uncover() {
        browserMenu.hide()
        linkMenu.hide()
        navigationBar.endEditing()
        deck.settle(false)
    }

    function openOmnibar(forNewTab) {
        uncover()
        findBar.close()
        navigationBar.beginEditing(forNewTab)
    }

    function openChosen(url, inNewTab) {
        navigationBar.endEditing()
        if (inNewTab && url.length > 0) {
            captureCurrent()
            TabModel.newTab(url)
        } else {
            openUrl(url)
        }
    }

    function openDownload(downloadId, done) {
        navigationBar.endEditing()
        if (done) {
            Qt.openUrlExternally(DownloadModel.fileUrl(DownloadModel.rowOf(downloadId)))
        } else {
            pageStack.push(Qt.resolvedUrl("DownloadsPage.qml"))
        }
    }

    // Touch taken above bar from page foot, replayed to engine; focus ends address edit.
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

    // Function so load tests can compare with engine value.
    function pageZoom() {
        return Settings.pageZoom(Theme.pixelRatio)
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

    // Playing media keeps page awake.
    Connections {
        target: WebEngine
        onRecvObserve: {
            PageActivity.observe(message, data)
            DownloadModel.observe(message, data)
        }
    }

    Connections {
        target: DownloadModel
        onEngineRequest: WebEngine.notifyObservers(topic, data)
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
        WebEngineSettings.downloadDir = DownloadModel.directory
        WebEngineSettings.useDownloadDir = true
        for (var i = 0; i < PageActivity.topics.length; ++i) {
            WebEngine.addObserver(PageActivity.topics[i])
        }
        WebEngine.addObserver(DownloadModel.topic)
        ensureTab()
        updateCurrentView()
    }

    Connections {
        target: TabModel
        onActiveTabChanged: browserPage.updateCurrentView()
        onCountChanged: browserPage.ensureTab()
    }

    TabDeck {
        id: deck

        objectName: "tabDeck"
        width: parent.width
        pageHeight: browserPage.height
        onTabsOpenChanged: {
            if (tabsOpen) {
                navigationBar.endEditing()
            }
        }

        Rectangle {
            objectName: "cutoutBand"
            width: parent.width
            height: browserPage.pageCutoutInset
            color: browserPage.currentView
                   && browserPage.currentView.pageThemeColor.length > 0
                   ? browserPage.currentView.pageThemeColor : Theme.highlightDimmerColor
        }

        // Engine gets page between cutout and bar only, so page foot stays reachable.
        Item {
            id: viewArea

            objectName: "viewArea"
            anchors {
                left: parent.left
                right: parent.right
            }
            y: browserPage.pageCutoutInset
            height: browserPage.viewHeight - browserPage.pageCutoutInset

            // Restored tabs load on first activation; not recently read ones drop view until in front.
            Repeater {
                id: webViews

                objectName: "webViews"
                model: TabModel
                delegate: TabViewLoader {
                    sourceComponent: webViewComponent
                    onItemChanged: browserPage.updateCurrentView()
                }
            }

            StartPageLayer {
                id: startPageLayer

                anchors.fill: parent
                onOpenRequested: browserPage.openUrl(url)
            }
        }

        NavigationBar {
            id: navigationBar

            objectName: "navigationBar"
            width: parent.width
            // Page height, not layer's: Silica shrinks page for keyboard, field must follow.
            y: browserPage.height - height

            view: browserPage.currentView
            canGoBack: browserPage.canGoBack
            compact: browserPage.barCompact
            onAccepted: browserPage.openChosen(omnibar.enter(text), inNewTab)
            onBack: browserPage.goBack()
            onReloadOrStop: browserPage.reloadOrStop()
            onShowMenu: browserMenu.show()
            // Capture and prime grid while finger down: both in drag's first frame stuttered.
            onDragArmed: {
                browserPage.captureCurrent()
                deck.prime()
            }
            onDragDisarmed: deck.unprime()
            onDragStarted: deck.beginDrag()
            onDragMoved: deck.dragTo(distance)
            onDragFinished: deck.settle(distance > deck.pullThreshold)
            onPageTouchStarted: browserPage.touchPage(position, "start")
            onPageTouchMoved: browserPage.touchPage(position, "move")
            onPageTouchEnded: browserPage.touchPage(position, "end")
        }

        BarBanners {
            id: banners

            width: parent.width
            y: navigationBar.y - height
            allowed: !navigationBar.editing && !findBar.active && !browserPage.tabsOpen
                     && !browserPage.dragging
        }

        FindBar {
            id: findBar

            anchors.fill: navigationBar
            view: browserPage.currentView
        }

        // After both bars so nothing of theirs draws over it.
        OmnibarView {
            id: omnibar

            width: parent.width
            y: browserPage.cutoutInset
            height: navigationBar.y - y
            active: navigationBar.paneUp
            text: navigationBar.typedText
            forNewTab: navigationBar.forNewTab
            onGoRequested: browserPage.openChosen(url, forNewTab)
            onSearchRequested: browserPage.openChosen(url, forNewTab)
            onUrlChosen: browserPage.openChosen(url, forNewTab)
            onTabChosen: {
                navigationBar.endEditing()
                TabModel.activateTabById(tabId)
            }
            onDownloadChosen: browserPage.openDownload(downloadId, done)
            onDismissed: navigationBar.endEditing()
        }

        grid: TabsView {
            id: tabsView

            anchors.fill: parent
            cutoutHeight: browserPage.cutoutInset
            pageHeight: browserPage.height
            // Drawn once drag may start: first drawn frame uploads all previews, stutters drag.
            visible: deck.tabsOffset > 0 || deck.primed
            onPullStarted: deck.beginDrag()
            onPulled: deck.dragTo(browserPage.height - distance)
            onPullFinished: deck.settle(distance <= deck.pullThreshold)
            onTabActivated: deck.settle(false)
        }
    }

    BrowserMenu {
        id: browserMenu

        view: browserPage.currentView
        onFindRequested: findBar.open()
    }

    LinkMenu {
        id: linkMenu

        topInset: browserPage.pageCutoutInset
        previewView: Component { WebView { objectName: "linkPreviewView"; url: linkMenu.target.link } }
        onOpenRequested: browserPage.openChosen(url, inNewTab)
        onOpenedBehind: banners.tabOpened(tabId, title)
    }

    Component {
        id: webViewComponent

        WebView {
            id: webView

            objectName: "webView"
            // Active until pages sleep: Sailfish Gecko pauses media of inactive (hidden) documents.
            active: isCurrent && !PageActivity.asleep && !linkMenu.previewShown
                    && (browserPage.status === PageStatus.Active
                        || browserPage.status === PageStatus.Deactivating)
            // Link preview holds engine's single picture.
            visible: !linkMenu.previewShown
            downloadsEnabled: true

            property ViewChrome viewChrome: ViewChrome {
                view: webView
                cutoutInset: browserPage.pageCutoutInset
            }

            // Engine doesn't expose it; C++ validates colour.
            property string pageThemeColor: ""

            function fetchThemeColor() {
                runJavaScript(EngineMessages.themeColorScript, function (value) {
                    webView.pageThemeColor = EngineMessages.themeColor(value)
                }, function () {
                    webView.pageThemeColor = ""
                })
            }

            function fetchFavicon() {
                var pageUrl = reader.active ? reader.source : url
                runJavaScript(EngineMessages.faviconScript, function (href) {
                    TabModel.updateFavicon(tabId, EngineMessages.resolveFavicon(pageUrl, href))
                }, function () {
                    TabModel.updateFavicon(tabId, EngineMessages.defaultFavicon(pageUrl))
                })
            }

            // Only out of sight: Gecko draws all views in one window; suspendView() stops it.
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

            function captureThumbnail() {
                if (!isCurrent) {
                    return
                }
                // Half size: GPU readback at gesture start. Encode/write off GUI thread in model.
                grabToImage(function (result) {
                    TabModel.storeThumbnail(tabId, result.image)
                }, Qt.size(width / 2, height / 2))
            }

            property ReaderMode reader: ReaderMode { view: webView }
            property PageMediaLink media: PageMediaLink { view: webView; pageTabId: tabId }
            property PageNotificationLink notices: PageNotificationLink { view: webView; pageTabId: tabId }
            property PageViewport viewport: PageViewport { view: webView }
            property PageSearchLink searches: PageSearchLink { view: webView }
            property PageLinkMenu links: PageLinkMenu { view: webView; onRequested: if (isCurrent) linkMenu.openFor(target, webView) }

            onUrlChanged: TabModel.updateUrl(tabId, reader.follow(url))
            onTitleChanged: TabModel.updateTitle(tabId, title)
            onLoadingChanged: {
                // Doc arriving while asleep arrives awake (redirect, reload): re-suspend. Only
                // while asleep: suspending after return stops shared window drawing.
                if (suspended && PageActivity.asleep) {
                    suspendView()
                }
                if (loading) {
                    // Else bar stays slim from previous page's scroll.
                    chrome = true
                    pageThemeColor = ""
                } else {
                    fetchFavicon()
                    fetchThemeColor()
                    captureThumbnail()
                }
            }
            Component.onCompleted: {
                addMessageListener(EngineMessages.findResultMessage)
                url = initialUrl
            }
        }
    }
}
