// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// One entry of the browser menu: an icon with what it does written under it. An entry
// that is a switch -- this page bookmarked, this page in its desktop version -- is
// drawn in the highlight colour while it is on (docs/DECISIONS/0021-menu-sheet.md).
import QtQuick 2.6
import Sailfish.Silica 1.0

BackgroundItem {
    id: button

    property string iconSource
    property string text
    // A switch that is on.
    property bool checked: false

    height: column.height + 2 * Theme.paddingMedium
    // Nothing to act on -- no page yet -- and it says so, as a disabled Silica
    // control does.
    opacity: enabled ? 1.0 : Theme.opacityLow

    Column {
        id: column

        anchors.centerIn: parent
        width: parent.width
        spacing: Theme.paddingSmall

        Icon {
            objectName: "menuButtonIcon"
            anchors.horizontalCenter: parent.horizontalCenter
            source: button.iconSource
            highlighted: button.highlighted || button.checked
        }

        Label {
            objectName: "menuButtonLabel"
            x: Theme.paddingSmall
            width: parent.width - 2 * Theme.paddingSmall
            horizontalAlignment: Text.AlignHCenter
            text: button.text
            truncationMode: TruncationMode.Fade
            font.pixelSize: Theme.fontSizeTiny
            color: button.highlighted || button.checked ? Theme.highlightColor : Theme.primaryColor
        }
    }
}
