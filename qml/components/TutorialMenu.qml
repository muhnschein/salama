// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: menu

    property bool open: false
    // Site's own title: not translated.
    readonly property string pageTitle: "Sailfish OS"
    readonly property real sheetHeight: sheet.height

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
