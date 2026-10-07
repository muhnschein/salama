// Stub: Silica tap hint. Draws nothing; keeps settings and running state per start/restart/stop.
import QtQuick 2.6

Item {
    property int loops: -1
    property int taps: 1
    property bool running: true

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
