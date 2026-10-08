// Stub: press outside never arrives by itself; tests emit pressedOutside.
import QtQuick 2.6

Item {
    property bool stealPress: false

    signal pressedOutside(int mouseX, int mouseY)
    signal clickedOutside(int mouseX, int mouseY)
}
