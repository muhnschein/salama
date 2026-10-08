// Stub: show() counts and records text for tests; draws nothing, never times out.
import QtQuick 2.6

Item {
    // Silica durations; Qt 5.15 QML enums replace C++ ones.
    enum Duration {
        Short = 3000,
        Long = 4500
    }

    property string text
    property int duration: Notice.Short
    property real verticalOffset: 0
    property int anchor: 0
    property int shownCount: 0
    property string shownText

    function show() {
        shownCount += 1
        shownText = text
    }

    function hide() {
    }
}
