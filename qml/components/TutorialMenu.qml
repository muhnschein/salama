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
            color: Theme.highlightDimmerColor
            opacity: Theme.opacityHigh
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

        SheetGrip {
            edgeOf: sheet
        }

        Column {
            id: content

            width: parent.width
            topPadding: Theme.paddingMedium
            bottomPadding: Theme.paddingMedium

            MenuPageHeader {
                id: header

                width: parent.width
                url: "https://sailfishos.org"
                title: menu.pageTitle
            }

            Grid {
                width: parent.width
                columns: 5

                Repeater {
                    model: ["icon-m-search-on-page", "icon-m-favorite", "icon-m-share",
                        "icon-m-computer", "icon-m-file-formatted"]

                    MenuIconButton {
                        width: menu.width / 5
                        enabled: false
                        opacity: 1.0
                        iconSource: "image://theme/" + modelData
                    }
                }
            }

            Item {
                width: parent.width
                height: Theme.paddingMedium
            }

            Repeater {
                model: [
                    { icon: "icon-m-favorite-selected", name: qsTr("Bookmarks") },
                    { icon: "icon-m-history", name: qsTr("History") },
                    { icon: "icon-m-downloads", name: qsTr("Downloads") },
                    { icon: "icon-m-setting", name: qsTr("Settings") }
                ]

                MenuListItem {
                    width: menu.width
                    enabled: false
                    opacity: 1.0
                    iconSource: "image://theme/" + modelData.icon
                    text: modelData.name
                }
            }
        }
    }
}
