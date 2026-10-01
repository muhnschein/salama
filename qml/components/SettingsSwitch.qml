// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// A switch on the main Settings page, standing in the column of icons with the ways to
// pages of their own (components/SettingsEntry.qml) and the choices made in place
// (components/SettingsComboBox.qml). A row has an icon or a switch, never both: the
// switch's own light is what stands where the icon would, centred on that column, as
// sailfish-browser centres its TextSwitches on its icons
// (apps/browser/qml/pages/SettingsPage.qml, _textSwitchIconCenter;
// docs/DECISIONS/0028-settings-pages.md).
import QtQuick 2.6
import Sailfish.Silica 1.0

TextSwitch {
    leftMargin: Theme.horizontalPageMargin + Theme.paddingLarge
                + Math.round((Theme.iconSizeMedium - Theme.itemSizeExtraSmall) / 2)
}
