// Stub: Silica's TextField is an editor with a label above it and an underline below.
// Text rather than Item, so that `font` and `text` are the real ones -- a field the
// application sizes its own text on is a field whose font has to exist.
import QtQuick 2.6

Text {
    property string label
    property string placeholderText
    property int inputMethodHints: 0
    property bool readOnly: false
    property int selectAllCount: 0
    // Silica's TextBase publishes where its text sits relative to the item's own
    // centre, so a field can be lined up with a label beside it.
    property real textVerticalCenterOffset: 0

    function selectAll() {
        selectAllCount += 1
    }
}
