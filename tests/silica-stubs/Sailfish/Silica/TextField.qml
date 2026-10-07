// Stub: Text not Item, so font and text are real: app sizes its text on field font.
import QtQuick 2.6

Text {
    property string label
    property string placeholderText
    property int inputMethodHints: 0
    property bool readOnly: false
    property int selectAllCount: 0
    // TextBase: text offset from item centre, for aligning with label beside.
    property real textVerticalCenterOffset: 0
    // Silica insets text by Theme.horizontalPageMargin each end; stub Theme = 24.
    property real textLeftMargin: 24
    property real textRightMargin: 24
    property bool errorHighlight: false
    // Press outside: FocusBehavior.ClearItemFocus (TextBase default) drops focus.
    property int focusOutBehavior: 0
    property Item leftItem
    property Item rightItem

    function selectAll() {
        selectAllCount += 1
    }
}
