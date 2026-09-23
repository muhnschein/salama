// Stub: Silica's fades its source item out towards one edge or both with a shader,
// drawn in place of the item. These tests draw nothing; the stub keeps what it was
// told, over the item it would be drawn over.
import QtQuick 2.6

Item {
    property Item sourceItem
    property int direction: 0
    property real slope: 2.0
    property real offset: 0.5

    x: sourceItem ? sourceItem.x : 0
    y: sourceItem ? sourceItem.y : 0
    width: sourceItem ? sourceItem.width : 0
    height: sourceItem ? sourceItem.height : 0
}
