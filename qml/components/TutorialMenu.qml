// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The menu as the tutorial draws it: the sheet up from under the bar, its two rows under
// their headings -- what can be done with the page in front, and where the browser's own
// pages are -- each entry the real sheet's icon and name, MenuButton as the real sheet has
// it (docs/DECISIONS/0021-menu-sheet.md, 0034-tutorial.md). The rest of the screen is
// dimmed, as a modal DockedPanel dims it, and a tap there puts the sheet away, as it does
// the real one. The entries do nothing.
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: menu

    property bool open: false
    // How tall the sheet is, from its top to the foot of the screen.
    readonly property real sheetHeight: sheet.height

    // A tap outside the sheet.
    signal dismissed()

    visible: open

    MouseArea {
        objectName: "tutorialMenuOutside"
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            bottom: sheet.top
        }
        onClicked: menu.dismissed()

        Rectangle {
            anchors.fill: parent
            color: Theme.rgba(Theme.overlayBackgroundColor, Theme.opacityLow)
        }
    }

    Rectangle {
        id: sheet

        objectName: "tutorialMenuSheet"
        anchors.bottom: parent.bottom
        width: parent.width
        height: content.height
        color: Theme.rgba(Theme.highlightDimmerColor, Theme.opacityOverlay)

        // Presses on the sheet stay on it.
        MouseArea {
            anchors.fill: parent
        }

        Column {
            id: content

            width: parent.width
            bottomPadding: Theme.paddingMedium

            Item {
                width: parent.width
                height: Theme.paddingLarge

                DragHandle {
                    x: (parent.width - width) / 2
                    y: Theme.paddingSmall
                }
            }

            SectionHeader {
                text: qsTr("This page")
            }

            Grid {
                width: parent.width
                columns: 4

                Repeater {
                    model: [
                        { icon: "icon-m-search-on-page", name: qsTr("Search on page") },
                        { icon: "icon-m-favorite", name: qsTr("Bookmark") },
                        { icon: "icon-m-share", name: qsTr("Share") },
                        { icon: "icon-m-computer", name: qsTr("Desktop version") },
                        { icon: "icon-m-file-formatted", name: qsTr("Reader view") }
                    ]

                    MenuButton {
                        width: menu.width / 4
                        enabled: false
                        opacity: 1.0
                        iconSource: "image://theme/" + modelData.icon
                        text: modelData.name
                    }
                }
            }

            SectionHeader {
                text: qsTr("Browser")
            }

            Grid {
                width: parent.width
                columns: 4

                Repeater {
                    model: [
                        { icon: "icon-m-favorite-selected", name: qsTr("Bookmarks") },
                        { icon: "icon-m-history", name: qsTr("History") },
                        { icon: "icon-m-downloads", name: qsTr("Downloads") },
                        { icon: "icon-m-setting", name: qsTr("Settings") }
                    ]

                    MenuButton {
                        width: menu.width / 4
                        enabled: false
                        opacity: 1.0
                        iconSource: "image://theme/" + modelData.icon
                        text: modelData.name
                    }
                }
            }
        }
    }
}
