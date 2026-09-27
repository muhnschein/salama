// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// A way from Settings to a page of its own: the subject's icon and its name, and under
// the name how it is set now, in the secondary highlight colour, as Silica writes a
// value under a label (docs/DECISIONS/0028-settings-pages.md). The shape is
// sailfish-browser's own for its Passwords and Clear browsing data rows
// (apps/browser/qml/pages/SettingsPage.qml): a BackgroundItem a medium item tall, lit
// while it is pressed, as Silica's rows are. A way in with no value, the tutorial's, is
// its name alone, centred.
import QtQuick 2.6
import Sailfish.Silica 1.0

BackgroundItem {
    id: entry

    property string iconSource
    property string text
    // How the subject is set, or empty.
    property string value

    width: parent.width
    height: Theme.itemSizeMedium
    // Dimmed while there is nothing to do, as Silica dims a button.
    opacity: enabled ? 1.0 : Theme.opacityLow

    Icon {
        id: icon

        objectName: "settingsEntryIcon"
        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeMedium
        height: width
        source: entry.iconSource
        highlighted: entry.highlighted
    }

    Column {
        anchors {
            left: icon.right
            right: parent.right
            leftMargin: Theme.paddingMedium
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }

        Label {
            objectName: "settingsEntryName"
            width: parent.width
            text: entry.text
            truncationMode: TruncationMode.Fade
            color: entry.highlighted ? Theme.highlightColor : Theme.primaryColor
        }

        Label {
            objectName: "settingsEntryValue"
            width: parent.width
            visible: text.length > 0
            text: entry.value
            textFormat: Text.PlainText
            truncationMode: TruncationMode.Fade
            font.pixelSize: Theme.fontSizeExtraSmall
            color: entry.highlighted ? Theme.highlightColor : Theme.secondaryHighlightColor
        }
    }
}
