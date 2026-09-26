// Stub: Silica's blinks a finger's tap where it is placed, as often as loops says, and
// runs from the start unless told not to. These tests draw nothing; the stub keeps what
// it was told and whether it runs, as start(), restart() and stop() leave it.
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
