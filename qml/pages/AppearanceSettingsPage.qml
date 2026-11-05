// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
// How pages and bar look; Firefox's Customize, sailfish-browser's Appearance. Controls write
// live; no Save.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

Page {
    id: appearancePage

    property SettingNames names: SettingNames {}

    objectName: "appearanceSettingsPage"
    allowedOrientations: Orientation.Portrait

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column

            width: parent.width

            PageHeader {
                title: qsTr("Appearance")
            }

            // Index = stored value.
            SettingsComboBox {
                objectName: "websiteColorsCombo"
                iconSource: "image://theme/icon-m-night"
                label: qsTr("Preferred color scheme")
                description: qsTr("The website style to use when available")
                currentIndex: Settings.websiteColors
                menu: ContextMenu {
                    MenuItem {
                        text: appearancePage.names.websiteColors(Settings.WebsiteColorsAutomatic)
                    }

                    MenuItem {
                        text: appearancePage.names.websiteColors(Settings.WebsiteColorsLight)
                    }

                    MenuItem {
                        text: appearancePage.names.websiteColors(Settings.WebsiteColorsDark)
                    }
                }
                onCurrentIndexChanged: Settings.websiteColors = currentIndex
            }

            SettingsSwitch {
                objectName: "fixedToolbarSwitch"
                text: qsTr("Fixed toolbar")
                description: qsTr("Always show the bottom toolbar")
                checked: Settings.fixedToolbar
                onCheckedChanged: Settings.fixedToolbar = checked
            }

            SettingsComboBox {
                objectName: "notchGuardCombo"
                iconSource: "image://theme/icon-m-display"
                //: Setting for the notch at the top of the screen, where the camera sits
                label: qsTr("Screen notch")
                description: qsTr("Keeps page content clear of the notch")
                currentIndex: Settings.notchGuard
                menu: ContextMenu {
                    MenuItem {
                        text: appearancePage.names.notchGuard(Settings.NotchGuardAutomatic)
                    }

                    MenuItem {
                        text: appearancePage.names.notchGuard(Settings.NotchGuardForced)
                    }

                    MenuItem {
                        text: appearancePage.names.notchGuard(Settings.NotchGuardDisabled)
                    }
                }
                onCurrentIndexChanged: Settings.notchGuard = currentIndex
            }
        }

        VerticalScrollDecorator {}
    }
}
