// Stub: Silica gesture glow hint. Draws nothing; keeps settings and running state per
// start/restart/stop.
import QtQuick 2.6

Item {
    property int direction: 2
    property int interactionMode: 0
    property int loops: 3
    property real startX: 0
    property real startY: 0
    property real distance: 0
    property bool alwaysRunToEnd: false
    property bool running: false

    function start() {
        running = true
    }

    function restart() {
        running = true
    }

    function stop() {
        running = false
    }
}
