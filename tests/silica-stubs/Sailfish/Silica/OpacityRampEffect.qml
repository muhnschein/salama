// Stub: Silica shader fade toward edge(s). Draws nothing; keeps settings, sits over source.
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
