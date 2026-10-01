// Stub: Silica's formatting functions. Sizes in the platform's way of writing them,
// near enough for the tests to read: whole bytes, then kB and MB to one decimal.
pragma Singleton
import QtQuick 2.6

QtObject {
    function formatFileSize(bytes) {
        if (bytes < 1000) {
            return bytes + " B"
        }
        if (bytes < 1000 * 1000) {
            return (bytes / 1000).toFixed(1) + " kB"
        }
        return (bytes / (1000 * 1000)).toFixed(1) + " MB"
    }
}
