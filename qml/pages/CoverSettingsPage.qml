// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The cover: how much of itself it shows on the home screen
// (docs/DECISIONS/0014-cover-is-the-tab-count.md, 0028-settings-pages.md).
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Page {
    id: coverSettingsPage

    objectName: "coverSettingsPage"
    allowedOrientations: Orientation.Portrait

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column

            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: qsTr("Cover")
            }

            // The order is how much the cover says, least first, and the index is the
            // stored value -- Settings.CoverIconOnly, CoverLatestTab, CoverEveryTab.
            // A combo rather than three switches: these are one choice, not three.
            ComboBox {
                objectName: "coverStyleCombo"
                width: parent.width
                label: qsTr("Shows")
                currentIndex: Settings.coverStyle
                menu: ContextMenu {
                    MenuItem {
                        objectName: "coverIconOnlyItem"
                        text: qsTr("The icon alone")
                    }

                    MenuItem {
                        objectName: "coverLatestTabItem"
                        text: qsTr("The tab count and the last tab")
                    }

                    // "Most recent" rather than "every": the field draws six cells at
                    // most, most recently read first, and the number above it is what
                    // says how many there are (docs/DECISIONS/0014-cover-is-the-tab-count.md).
                    MenuItem {
                        objectName: "coverEveryTabItem"
                        text: qsTr("The tab count and the most recent tabs")
                    }
                }
                onCurrentIndexChanged: Settings.coverStyle = currentIndex
            }
        }

        VerticalScrollDecorator {}
    }
}
