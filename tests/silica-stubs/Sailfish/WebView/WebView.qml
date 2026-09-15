// Stub of Sailfish.WebView's WebView: the properties and slots tuuli uses, recording
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
    // QuickMozView's QMozSecurity. Assign null to stand in for an engine build that
    // has none.
    property QtObject security: QtObject {
        property bool validState: true
        property bool allGood: true
    }

    // Test hooks
    property var calls: []
    property string lastScript
    property string scriptResult: ""
    property bool scriptFails: false
    property string lastGrabPath: ""
    property var lastGrabSize
    property int grabCount: 0
    property bool grabFails: false
    property bool grabSaveFails: false

    signal linkClicked(string url)
    signal viewInitialized()

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
        if (scriptFails) {
            if (errorCallback) {
                errorCallback("stub failure")
            }
        } else if (callback) {
            callback(scriptResult)
        }
    }
}
