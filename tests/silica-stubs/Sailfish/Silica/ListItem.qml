// Stub: remorseAction runs callback immediately.
import QtQuick 2.6

Item {
    id: listItem

    property real contentHeight: 0

    // Silica ListItem height = content; load tests sort rows by it.
    implicitHeight: contentHeight
    property Item menu
    property bool down: false
    property bool highlighted: down
    property bool menuOpen: false
    property int remorseCount: 0

    signal clicked()
    signal pressAndHold()

    function openMenu(properties) {
        menuOpen = true
    }

    function closeMenu() {
        menuOpen = false
    }

    function remorseAction(text, callback) {
        remorseCount += 1
        callback()
    }
}
