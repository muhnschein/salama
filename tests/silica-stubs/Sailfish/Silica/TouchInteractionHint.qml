// Stub: Silica's draws a finger's glow travelling the way a gesture goes, from startX and
// startY, as often as loops says. These tests draw nothing; the stub keeps what it was
// told and whether it runs, as start(), restart() and stop() leave it.
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
