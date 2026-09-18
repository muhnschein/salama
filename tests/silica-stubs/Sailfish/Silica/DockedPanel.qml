// Stub: a panel that slides in from an edge. open is the state; show/hide flip it
// without any animation.
import QtQuick 2.6

Item {
    property bool open: false
    property int dock: 0
    property bool modal: false
    property bool moving: false
    property int animationDuration: 0
    property bool expanded: open

    function show() {
        open = true
    }

    function hide() {
        open = false
    }
}
