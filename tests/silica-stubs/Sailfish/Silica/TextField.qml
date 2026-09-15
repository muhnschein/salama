import QtQuick 2.6

Item {
    property string text
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
