import QtQuick 2.6

Item {
    // Pages fill window; size needed so list views instantiate delegates. As Silica: window
    // stays portrait, landscape page swaps sides and loses cutout (AvoidLandscapeCutout).
    width: !parent ? 0 : isPortrait ? parent.width : parent.height - Screen.topCutout.height
    height: !parent ? 0 : isPortrait ? parent.height : parent.width
    // As Silica: landscape page turned a quarter inside portrait window, so window axes are
    // page's only when upright.
    x: !parent || isPortrait ? 0 : (parent.width - width) / 2
    y: !parent || isPortrait ? 0 : (parent.height - height) / 2
    rotation: orientation === 2 ? 90 : orientation === 8 ? 270 : 0

    property int allowedOrientations: 0
    property int status: 0
    // As Silica: device's when page and window allow it, else portrait. Stack carries both.
    readonly property int orientation: {
        var device = parent && parent.deviceOrientation !== undefined ? parent.deviceOrientation : 1
        var window = parent && parent.windowOrientations !== undefined ? parent.windowOrientations
                                                                       : 15
        return (allowedOrientations & window & device) ? device : 1
    }
    readonly property bool isLandscape: (orientation & 10) !== 0
    readonly property bool isPortrait: !isLandscape
    property bool backNavigation: true
    property bool forwardNavigation: false
    property bool showNavigationIndicator: true
    property bool canNavigateForward: false
}
