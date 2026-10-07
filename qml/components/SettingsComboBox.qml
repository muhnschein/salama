// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
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
