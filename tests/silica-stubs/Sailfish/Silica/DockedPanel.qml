// Stub: a panel that slides in from an edge. open is the state; show/hide flip it
// without any animation. Shut, it is not there: Silica's own is off the screen, and a
// stub left lying over the view would take the presses meant for what is under it.
import QtQuick 2.6

Item {
    visible: open
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
