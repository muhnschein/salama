// Stub: Silica's is a band as wide as its parent, clear at one edge and darkening to
// the dimmer colour at the other, where its text lies in the highlight colour: the
// bottom edge, or the top one inverted.
import QtQuick 2.6

Rectangle {
    property string text
    property bool invert: false
    property color textColor
    property color backgroundColor

    width: parent ? parent.width : 0
}
