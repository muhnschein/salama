// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// A way from Settings to a page of its own: the subject's icon, its name, and under
// the name a line saying how it is set now, as Firefox for Android writes "Standard"
// under its tracking protection. The line is whatever the page using the entry binds to
// the setting, so it is already true when the subject's page is popped back to
// (docs/DECISIONS/0028-settings-pages.md). The shape is sailfish-browser's own for its
// Passwords and Clear browsing data rows (apps/browser/qml/pages/SettingsPage.qml): a
// BackgroundItem a medium item tall, lit while it is pressed, as Silica's rows are.
import QtQuick 2.6
import Sailfish.Silica 1.0

BackgroundItem {
    id: entry

    property string iconSource
    property string text
    // How the subject is set, in a line; left empty, the name is alone and centred.
    property string summary

    width: parent.width
    height: Theme.itemSizeMedium

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
            objectName: "settingsEntrySummary"
            width: parent.width
            visible: text.length > 0
            text: entry.summary
            truncationMode: TruncationMode.Fade
            font.pixelSize: Theme.fontSizeExtraSmall
            color: entry.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
        }
    }
}
