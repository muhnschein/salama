// Stub: show/hide flip open, no animation. Open: lies on docked edge so finger hits it.
// Shut: hidden, else it would eat presses meant for view below.
import QtQuick 2.6

Item {
    id: panel

    visible: open
    // Silica's clips too: anything over its edge is cut.
    clip: true
    property bool open: false
    property int dock: 0
    property bool modal: false
    property bool moving: false
    property int animationDuration: 0
    property bool expanded: open

    // Last hide skipped Silica's slide out.
    property bool hiddenAtOnce: false

    function show() {
        hiddenAtOnce = false
        open = true
    }

    function hide(immediate) {
        hiddenAtOnce = !!immediate
        open = false
    }

    // Dock.Top = 1, Dock.Bottom = 2, Dock.Right = 8, as stub plugin registers. As Silica: x and
    // y of the axis not docked along are 0 for it.
    function place() {
        if (dock === 2 && parent) {
            y = open ? parent.height - height : parent.height
        } else if (dock === 1) {
            y = open ? 0 : -height
        } else if (dock === 8 && parent) {
            x = open ? parent.width - width : parent.width
        }
    }

    // As Silica: turning to another edge moves both axes at once, so a side panel's y is 0
    // until its own handler places it.
    onDockChanged: {
        if (dock === 8) {
            y = 0
        } else {
            x = 0
        }
        place()
    }
    onOpenChanged: place()
    onHeightChanged: place()
    onWidthChanged: place()
    Component.onCompleted: place()

    // Silica's targets follow parent's size.
    Connections {
        target: panel.parent
        onWidthChanged: place()
        onHeightChanged: place()
    }
}
