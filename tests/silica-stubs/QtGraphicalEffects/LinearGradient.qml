// Stub: the module is not installed on the host, and these tests draw nothing. It
// exists so the import resolves; what the effect renders is a device question.
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
