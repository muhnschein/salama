// Stub of Sailfish.WebView's WebView: the properties and slots salama uses, recording
// calls so tests can assert on them. Property names follow qtmozembed's
// qmozview_defined_wrapper.h and sailfish-components-webview's WebView.qml.
import QtQuick 2.6

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
    property var popupProvider
    // RawWebView: what the engine keeps clear at the foot of the viewport.
    property real footerMargin: 0
    // QuickMozView's chrome gesture: the engine drops chrome while a page is scrolled
    // down and asks for it back on the way up.
    property bool chrome: true
    property bool chromeGestureEnabled: true
    property real chromeGestureThreshold: 0
    // What the platform's own WebView hands the engine for the display's cutout, so
    // that a page written for one can lay itself out around it.
    property real safeAreaTop: 90
    property real safeAreaRight: 0
    property real safeAreaBottom: 0
    property real safeAreaLeft: 0
    // The page's own theme colour, when it declares one.
    property bool hasThemeColor: false
    property color themeColor: "black"
    // QuickMozView's QMozSecurity. Assign null to stand in for an engine build that
    // has none.
    property QtObject security: QtObject {
        property bool validState: true
        property bool allGood: true
    }

    // Test hooks
    property var calls: []
    property string lastScript
    // Every script run since the view was made: a page is asked more than one thing
    // when it finishes loading, and only the last of them would be seen otherwise.
    property var scripts: []
    // Whether the view was active as each of them was run.
    property var activeWhenRun: []
    // What a script answers: scriptResult, or what answer(script) says when a test
    // has set it, for a page asked more than one thing.
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
    // QuickMozView: a message from the page's own scripts, on a name listened for.
    signal recvAsyncMessage(string message, var data)

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

    // qtmozembed's QuickMozView::loadHtml: the text loaded as a data: url.
    property string lastHtml

    function loadHtml(html, baseUrl) {
        record("loadHtml")
        lastHtml = html
        url = "data:text/html;charset=utf-8," + encodeURIComponent(html)
    }

    // QuickMozView's messages to the page's own scripts, recorded as {name, data}, and
    // the names of the ones from it that the view has been told to listen for.
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

    // QuickMozView's synthetic touches, in the view's own coordinates: recorded as
    // {phase, x, y}, one point each.
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

    // QuickMozView: inactive with its timers stopped, and back. Recorded only: the
    // real ones set `active` from C++, which leaves a QML binding on it in place, and
    // an assignment here would have removed the page's.
    function suspendView() {
        record("suspendView")
    }

    function resumeView() {
        record("resumeView")
    }

    // Stands in for QQuickItem::grabToImage, which needs a rendering scene graph the
    // offscreen test platform does not provide. Calls back synchronously.
    function grabToImage(callback, targetSize) {
        grabCount += 1
        lastGrabSize = targetSize
        if (grabFails) {
            return false
        }
        callback({
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
