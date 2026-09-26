// Stub: values exist only so bindings resolve; nothing here imitates layout.
pragma Singleton
import QtQuick 2.6

QtObject {
    readonly property real paddingSmall: 6
    readonly property real paddingMedium: 12
    readonly property real paddingLarge: 24
    readonly property real horizontalPageMargin: 24
    readonly property real fontSizeTiny: 20
    readonly property real fontSizeExtraSmall: 24
    readonly property real fontSizeSmall: 28
    readonly property real fontSizeMedium: 32
    readonly property real fontSizeLarge: 40
    readonly property real fontSizeExtraLarge: 50
    readonly property real fontSizeHuge: 90
    readonly property string fontFamily: "Sans"
    readonly property string fontFamilyHeading: "Sans"
    readonly property real iconSizeSmall: 32
    readonly property real iconSizeSmallPlus: 48
    readonly property real iconSizeMedium: 64
    readonly property real iconSizeLarge: 96
    readonly property real itemSizeSmall: 80
    readonly property real itemSizeMedium: 100
    readonly property real itemSizeLarge: 110
    readonly property real itemSizeExtraLarge: 135
    readonly property color primaryColor: "#ffffff"
    readonly property color secondaryColor: "#b0ffffff"
    readonly property color highlightColor: "#aaccff"
    readonly property color secondaryHighlightColor: "#b0aaccff"
    readonly property color errorColor: "#ff4d4d"
    readonly property color highlightBackgroundColor: "#aaccff"
    readonly property color highlightDimmerColor: "#22447f"
    // A dark ambience's: black, as white is a light one's.
    readonly property color overlayBackgroundColor: "#000000"
    readonly property real highlightBackgroundOpacity: 0.3
    readonly property real opacityFaint: 0.2
    readonly property real opacityLow: 0.4
    readonly property real opacityHigh: 0.6
    readonly property real opacityOverlay: 0.8
    readonly property real startDragDistance: 20
    readonly property real itemSizeExtraSmall: 60
    // The size of a cover on the home screen, which the cover's settings draw theirs to.
    readonly property size coverSizeLarge: Qt.size(234, 374)
    readonly property real pixelRatio: 2.0
    readonly property real _lineWidth: 2
    // Silica's names the pattern the ambience lays over its glass; the stub's names an
    // image its own theme provider draws.
    readonly property url _patternImage: "image://theme/glass-pattern"

    function rgba(color, opacity) {
        return Qt.rgba(color.r, color.g, color.b, opacity)
    }
}
