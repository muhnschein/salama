// Stub: module absent on host; exists so import resolves. Tests draw nothing.
import QtQuick 2.6

Item {
    default property alias stops: holder.data

    property variant source
    property variant start
    property variant end
    property Gradient gradient
    property bool cached: false

    Item {
        id: holder
    }
}
