// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// One entry of the browser menu: an icon with what it does written under it
// (docs/DECISIONS/0021-menu-sheet.md). The page's own actions sit on a disc (round),
// which is lit while the entry is a switch that is on -- this page bookmarked, in its
// desktop version, in its reader view -- and under a finger. The browser's entries have
// no disc, and one standing for something under way -- the downloads still coming --
// has a ring round its icon that fills as it goes (busy, progress).
//
// Nothing is washed across the whole entry when it is pressed, as a BackgroundItem
// would: what lights is the disc, the icon and the name, the way an IconButton lights
// its icon. A square wash round a disc was a second shape for one press.
import QtQuick 2.6
import Sailfish.Silica 1.0

BackgroundItem {
    id: button

    property string iconSource
    property string text
    // A switch that is on.
    property bool checked: false
    // The icon on a disc of its own.
    property bool round: false
    // Something the entry stands for is under way, and how far along it is, from 0 to 1.
    property bool busy: false
    property real progress: 0
    readonly property bool lit: highlighted || checked

    height: column.height + 2 * Theme.paddingMedium
    // Nothing to act on -- no page yet -- and it says so, as a disabled Silica
    // control does.
    opacity: enabled ? 1.0 : Theme.opacityLow
    highlightedColor: "transparent"

    Column {
        id: column

        anchors.centerIn: parent
        width: parent.width
        spacing: Theme.paddingSmall

        // As tall on every entry, disc or not, so the names along a row line up. The
        // icons are a step down from Silica's medium size, as the grid's corner buttons
        // are, so that the disc round one is no bigger than a medium icon.
        Item {
            objectName: "menuButtonIconSlot"
            anchors.horizontalCenter: parent.horizontalCenter
            width: Theme.iconSizeMedium + Theme.paddingSmall
            height: width

            Rectangle {
                objectName: "menuButtonDisc"
                anchors.fill: parent
                radius: width / 2
                visible: button.round
                // Lit in the wash Silica marks a pressed or chosen item with.
                color: button.lit ? Theme.rgba(Theme.highlightBackgroundColor,
                                               Theme.highlightBackgroundOpacity)
                                  : Theme.rgba(Theme.primaryColor, Theme.opacityFaint)
            }

            DownloadRing {
                objectName: "menuButtonProgress"
                anchors.centerIn: parent
                visible: button.busy
                value: button.progress
            }

            Icon {
                objectName: "menuButtonIcon"
                anchors.centerIn: parent
                source: button.iconSource
                sourceSize: Qt.size(Theme.iconSizeSmallPlus, Theme.iconSizeSmallPlus)
                highlighted: button.lit
            }
        }

        // As wide as the entry, with no padding either side: five to a row leave a name a
        // fifth of the screen, and "Desktop site" in tiny type needs about all of it. The
        // fade is for a name that does not fit even so.
        Label {
            objectName: "menuButtonLabel"
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: button.text
            truncationMode: TruncationMode.Fade
            font.pixelSize: Theme.fontSizeTiny
            color: button.lit ? Theme.highlightColor : Theme.primaryColor
        }
    }
}
