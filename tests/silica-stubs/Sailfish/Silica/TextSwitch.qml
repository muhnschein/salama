import QtQuick 2.6

Item {
    // Sized as Silica sizes it: Column or Flow places no item with empty side.
    width: parent ? parent.width : 0
    height: Theme.itemSizeSmall
    property string text
    property string description
    property real leftMargin
    property bool highlighted: false
    property bool checked: false
    property bool automaticCheck: true
    property bool busy: false

    signal clicked()
    signal pressAndHold()
}
