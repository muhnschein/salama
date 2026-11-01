// Stub: remorseAction runs callback immediately.
import QtQuick 2.6

Item {
    id: listItem

    property real contentHeight: 0

    // Silica ListItem height = content; load tests sort rows by it.
    implicitHeight: contentHeight
    // Item or Component, as Silica: Component made on first open.
    property var menu
    property Item _menuItem: null
    property bool down: false
    property bool highlighted: down
    property bool menuOpen: false
    property int remorseCount: 0

    signal clicked()
    signal pressAndHold()

    function openMenu(properties) {
        if (menu && menu.createObject !== undefined && _menuItem === null) {
            _menuItem = menu.createObject(listItem, properties || {})
        }
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
