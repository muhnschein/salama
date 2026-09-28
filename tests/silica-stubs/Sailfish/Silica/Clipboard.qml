// Stub: the system clipboard, as Silica hands it to QML. What was put on it stays for a
// test to read.
pragma Singleton
import QtQuick 2.6

QtObject {
    property string text
    readonly property bool hasText: text.length > 0
}
