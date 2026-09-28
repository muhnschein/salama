import QtQuick 2.6

Item {
    id: backgroundItem

    property bool down: false
    property bool highlighted: down
    // The wash drawn across the item while it is highlighted.
    property color highlightedColor: "#4daaccff"
    property int remorseCount: 0

    signal clicked()

    function remorseAction(text, callback) {
        remorseCount += 1
        callback()
    }
}
