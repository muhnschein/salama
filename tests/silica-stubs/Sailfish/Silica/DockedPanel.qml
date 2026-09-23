// Stub: a panel that slides in from an edge. open is the state; show/hide flip it
// without any animation. Open, it lies along the edge it is docked to, where Silica's
// own settles, so a finger can be put on it; the application may move it from there,
// as Silica's own lets it. Shut, it is not there: Silica's own is off the screen, and a
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

    // Dock.Top is 1 and Dock.Bottom 2, the values the stub plugin registers.
    function place() {
        if (dock === 2 && parent) {
            y = open ? parent.height - height : parent.height
        } else if (dock === 1) {
            y = open ? 0 : -height
        }
    }

    onOpenChanged: place()
    onHeightChanged: place()
    Component.onCompleted: place()
}
