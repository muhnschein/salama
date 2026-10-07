pragma Singleton
import QtQuick 2.6

QtObject {
    readonly property int width: 1080
    readonly property int height: 2520
    readonly property int sizeCategory: 2
    // Portrait top cutout. Non-zero so tests can assert clearing it.
    readonly property QtObject topCutout: QtObject {
        readonly property int x: 390
        readonly property int y: 0
        readonly property int width: 300
        readonly property int height: 90
    }
}
