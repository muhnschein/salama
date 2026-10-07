// Stub of Sailfish.WebView WebView: records calls for tests. Names follow qtmozembed
// qmozview_defined_wrapper.h and sailfish-components-webview WebView.qml.
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: webView

    property url url
    property string title
    property bool loading: false
    property int loadProgress: 0
    property bool canGoBack: false
    property bool canGoForward: false
    property bool active: false
    property bool privateMode: false
    property bool desktopMode: false
    property bool downloadsEnabled: false
    property bool domContentLoaded: false
    property string httpUserAgent
    property var popupProvider: QtObject {
        property var contextMenu: ({ "type": "item", "component": "ContextMenu.qml" })
    }
    property real footerMargin: 0
    // QuickMozView chrome gesture: engine drops chrome on scroll down, wants it back on scroll up.
    property bool chrome: true
    property bool chromeGestureEnabled: true
    property real chromeGestureThreshold: 0
    // Display cutout inset handed to engine. Binding like platform's: Binding release restores
    // binding (Qt 5.15: only binding).
    property real cutoutSafeAreaTop: 90
    property real safeAreaTop: cutoutSafeAreaTop
    property real safeAreaRight: 0
    property real safeAreaBottom: 0
    property real safeAreaLeft: 0
    property bool hasThemeColor: false
    property color themeColor: "black"
    // QuickMozView QMozSecurity. Assign null = engine build without it.
    property QtObject security: QtObject {
        property bool validState: true
        property bool allGood: true
        property bool domainMismatch: false
        property bool notValidAtThisTime: false
        property bool untrusted: false
        property bool blockedTrackingContent: false
        property string subjectDisplayName: ""
        property string issuerDisplayName: ""
        property var expiryDate: null
        // QMozSecurity::TLS_VERSION: -1 none, 4 TLS 1.3.
        property int protocolVersion: -1
        property string cipherName: ""
    }

    property var calls: []
    property string lastScript
    // Every script run: page asked several things on load, lastScript keeps only last.
    property var scripts: []
    property var activeWhenRun: []
    // Script answer: answer(script) if set, else scriptResult.
    property var scriptResult: ""
    property var answer: null
    property bool scriptFails: false
    property string lastGrabPath: ""
    property var lastGrabSize
    property int grabCount: 0
    property bool grabFails: false
    property bool grabSaveFails: false

    signal linkClicked(string url)
    signal viewInitialized()
    signal recvAsyncMessage(string message, var data)
    signal aboutToOpenPopup(var topic, var data)

    function record(name) {
        var list = calls
        list.push(name)
        calls = list
    }

    function goBack() {
        record("goBack")
    }

    function goForward() {
        record("goForward")
    }

    function reload() {
        record("reload")
    }

    function stop() {
        record("stop")
    }

    function load(target, fromExternal) {
        record("load")
        url = target
    }

    property string lastHtml

    function loadHtml(html, baseUrl) {
        record("loadHtml")
        lastHtml = html
        url = "data:text/html;charset=utf-8," + encodeURIComponent(html)
    }

    property var messages: []
    property var messageListeners: []

    function sendAsyncMessage(name, data) {
        var list = messages
        list.push({ "name": name, "data": data })
        messages = list
    }

    function addMessageListener(name) {
        var list = messageListeners
        list.push(name)
        messageListeners = list
    }

    property var frameScripts: []

    function loadFrameScript(name) {
        var list = frameScripts
        list.push(name)
        frameScripts = list
    }

    property var touches: []

    function touch(phase, points) {
        var list = touches
        list.push({ "phase": phase, "x": points[0].x, "y": points[0].y })
        touches = list
    }

    function synthTouchBegin(points) {
        touch("begin", points)
    }

    function synthTouchMove(points) {
        touch("move", points)
    }

    function synthTouchEnd(points) {
        touch("end", points)
    }

    // QuickMozView suspend/resume. Record only: real ones set active from C++, keeping QML
    // binding; assignment here would break page's binding.
    function suspendView() {
        record("suspendView")
    }

    function resumeView() {
        record("resumeView")
    }

    // Replaces QQuickItem::grabToImage: offscreen platform has no scene graph. Sync callback,
    // blank image of asked size; null image when grabSaveFails.
    function grabToImage(callback, targetSize) {
        grabCount += 1
        lastGrabSize = targetSize
        if (grabFails) {
            return false
        }
        callback({
                     "image": GrabStub.image(webView.grabSaveFails ? 0 : targetSize.width,
                                             webView.grabSaveFails ? 0 : targetSize.height),
                     "url": "image://grab/" + grabCount,
                     "saveToFile": function (path) {
                         webView.lastGrabPath = path
                         return !webView.grabSaveFails
                     }
                 })
        return true
    }

    function runJavaScript(script, callback, errorCallback) {
        lastScript = script
        var list = scripts
        list.push(script)
        scripts = list
        var states = activeWhenRun
        states.push(active)
        activeWhenRun = states
        if (scriptFails) {
            if (errorCallback) {
                errorCallback("stub failure")
            }
        } else if (callback) {
            callback(answer ? answer(script) : scriptResult)
        }
    }
}
