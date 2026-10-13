import QtQuick 2.6

Item {
    // Sized as Silica sizes it: a Column or Flow places no item with an empty side.
    width: parent ? parent.width : 0
    height: Theme.itemSizeMedium
    property string label
    property string description
    property real leftMargin
    property bool highlighted: false
    property int currentIndex: -1
    property Item menu
    property string value
}
