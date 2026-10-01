import QtQuick 2.6

Item {
    property string text
    property string description
    property real leftMargin
    property bool highlighted: false
    property bool checked: false
    property bool automaticCheck: true
    property bool busy: false

    signal clicked()
    signal pressAndHold()
}
