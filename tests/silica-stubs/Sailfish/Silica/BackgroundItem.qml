import QtQuick 2.6

Item {
    id: backgroundItem

    property bool down: false
    property bool highlighted: down
    property int remorseCount: 0

    signal clicked()

    function remorseAction(text, callback) {
        remorseCount += 1
        callback()
    }
}
