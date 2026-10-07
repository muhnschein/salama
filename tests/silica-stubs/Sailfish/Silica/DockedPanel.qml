// Stub: show/hide flip open, no animation. Open: lies on docked edge so finger hits it.
// Shut: hidden, else it would eat presses meant for view below.
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

    // Dock.Top = 1, Dock.Bottom = 2, as stub plugin registers.
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
