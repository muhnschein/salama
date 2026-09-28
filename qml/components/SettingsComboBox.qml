// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// A choice made where it is, on the main Settings page, with its subject's icon before
// it, so it stands in the column of icons with the ways to pages of their own
// (components/SettingsEntry.qml). The shape is sailfish-browser's own for its colour
// scheme and notch guard (apps/browser/qml/pages/components/BrowserComboBox.qml): a
// Silica ComboBox moved in from the edge by the icon and a gap, and the icon at the
// page's margin (docs/DECISIONS/0028-settings-pages.md).
import QtQuick 2.6
import Sailfish.Silica 1.0

ComboBox {
    id: combo

    property alias iconSource: icon.source

    width: parent.width
    leftMargin: Theme.horizontalPageMargin + icon.width + Theme.paddingMedium

    Icon {
        id: icon

        objectName: "settingsComboBoxIcon"
        anchors.verticalCenter: parent.verticalCenter
        x: Theme.horizontalPageMargin
        highlighted: combo.highlighted
    }
}
