pragma Singleton
import QtQuick 2.6

QtObject {
    readonly property int width: 1080
    readonly property int height: 2520
    readonly property int sizeCategory: 2
    // What the display's own cutout takes at the top of the screen in portrait.
    // Non-zero, so that clearing it can be asserted rather than assumed.
    readonly property QtObject topCutout: QtObject {
        readonly property int x: 390
        readonly property int y: 0
        readonly property int width: 300
        readonly property int height: 90
    }
}
