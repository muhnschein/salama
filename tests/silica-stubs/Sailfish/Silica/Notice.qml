// Stub: Silica's short message at the foot of the screen. show() counts and says what
// was shown, for a test to read; nothing is drawn and nothing times out.
import QtQuick 2.6

Item {
    // Silica's own durations; Qt 5.15's QML enums stand in for the C++ ones.
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
