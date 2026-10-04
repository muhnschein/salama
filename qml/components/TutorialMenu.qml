// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The menu as the tutorial draws it: the sheet up from under the bar on the real sheet's
// ground, its head naming the sketched page, then its two rows -- the page's five actions
// on their discs, a line, and where the browser's own pages are -- each entry the real
// sheet's icon and name, MenuButton and MenuPageHeader as the real sheet has them
// (docs/DECISIONS/0021-menu-sheet.md, 0034-tutorial.md). The rest of the screen is
// dimmed, as a modal DockedPanel dims it, and a tap there puts the sheet away, as it does
// the real one. The entries do nothing.
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: menu

    property bool open: false
    // The sketched page's own title, as its site names it: not translated.
    readonly property string pageTitle: "Sailfish OS"
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

    Item {
        id: sheet

        objectName: "tutorialMenuSheet"
        anchors.bottom: parent.bottom
        width: parent.width
        height: content.height

        SheetBackground {
            anchors.fill: parent
        }

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

            // The sketched page, as the bar names it; its copy button does nothing here.
            MenuPageHeader {
                width: parent.width
                url: "https://sailfishos.org"
                title: menu.pageTitle
            }

            Grid {
                width: parent.width
                columns: 5

                Repeater {
                    model: [
                        { icon: "icon-m-search-on-page", name: qsTr("Find in page") },
                        { icon: "icon-m-favorite", name: qsTr("Bookmark") },
                        { icon: "icon-m-share", name: qsTr("Share") },
                        { icon: "icon-m-computer", name: qsTr("Desktop site") },
                        { icon: "icon-m-file-formatted", name: qsTr("Reader view") }
                    ]

                    MenuButton {
                        width: menu.width / 5
                        round: true
                        enabled: false
                        opacity: 1.0
                        iconSource: "image://theme/" + modelData.icon
                        text: modelData.name
                    }
                }
            }

            // The real sheet's line between the page's actions and the browser's.
            Item {
                width: parent.width
                height: Theme.paddingLarge

                Row {
                    anchors.centerIn: parent

                    Separator {
                        width: menu.width / 2 - Theme.horizontalPageMargin
                        color: Theme.primaryColor
                        rotation: 180
                    }

                    Separator {
                        width: menu.width / 2 - Theme.horizontalPageMargin
                        color: Theme.primaryColor
                    }
                }
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
