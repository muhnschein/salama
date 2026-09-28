// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// A switch on the main Settings page with its subject's icon before it, so it stands in
// the column of icons with the ways to pages of their own (components/SettingsEntry.qml)
// and the choices made in place (components/SettingsComboBox.qml): a Silica TextSwitch
// moved in from the edge by the icon and a gap, as sailfish-browser moves its combo boxes
// in for theirs, and the icon at the page's margin
// (docs/DECISIONS/0028-settings-pages.md).
import QtQuick 2.6
import Sailfish.Silica 1.0

TextSwitch {
    id: toggle

    property alias iconSource: icon.source

    leftMargin: Theme.horizontalPageMargin + icon.width + Theme.paddingMedium

    Icon {
        id: icon

        objectName: "settingsSwitchIcon"
        anchors.verticalCenter: parent.verticalCenter
        x: Theme.horizontalPageMargin
        highlighted: toggle.highlighted
    }
}
