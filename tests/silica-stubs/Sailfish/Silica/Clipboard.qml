// Stub: system clipboard; content kept for tests.
pragma Singleton
import QtQuick 2.6

QtObject {
    property string text
    readonly property bool hasText: text.length > 0
}
