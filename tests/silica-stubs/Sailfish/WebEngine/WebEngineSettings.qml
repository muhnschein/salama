pragma Singleton
import QtQuick 2.6

QtObject {
    property bool javascriptEnabled: true
    property bool autoLoadImages: true
    property bool popupEnabled: true
    property bool useDownloadDir: false
    // QMozEngineSettings: how many device pixels the engine lays a css pixel out in.
    property real pixelRatio: 1.0
    property string downloadDir
    property var preferences: []

    function setPreference(key, value, type) {
        var list = preferences
        list.push({ "key": key, "value": value })
        preferences = list
    }
}
