// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: menu

    property bool open: false
    // Bar's top: side sheet stands on it.
    property real barTop: height
    // Site's own title: not translated.
    readonly property string pageTitle: "Sailfish OS"
    readonly property real sheetHeight: sheet.height
    readonly property real sheetWidth: sheet.width
    // Landscape: side sheet above bar's right end, as menu.
    readonly property bool wide: width > height

    signal dismissed()

    visible: open

    MouseArea {
        objectName: "tutorialMenuOutside"
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            bottom: menu.wide ? parent.bottom : sheet.top
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
        anchors {
            right: parent.right
            bottom: parent.bottom
            bottomMargin: menu.wide ? parent.height - menu.barTop : 0
        }
        width: menu.wide ? Math.floor(parent.width * 0.42) : parent.width
        height: menu.wide ? Math.min(content.height, menu.barTop) : content.height

        SheetBackground {
            anchors.fill: parent
            headHeight: headGap.y + headGap.height / 2
            side: menu.wide
        }

        MouseArea {
            anchors.fill: parent
        }

        SheetGrip {
            edgeOf: sheet
            upright: menu.wide
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
                objectName: "tutorialMenuPageRow"
                width: parent.width
                columns: 5

                Repeater {
                    model: [
                        { icon: "icon-m-search-on-page", badge: "" },
                        { icon: "icon-m-favorite", badge: "add" },
                        { icon: "icon-m-share", badge: "" },
                        { icon: "icon-m-computer", badge: "" },
                        { icon: "icon-m-file-formatted", badge: "" }
                    ]

                    MenuIconButton {
                        width: sheet.width / 5
                        enabled: false
                        opacity: 1.0
                        iconSource: "image://theme/" + modelData.icon
                        badge: modelData.badge
                    }
                }
            }

            Item {
                id: headGap

                objectName: "tutorialHeadGap"
                width: parent.width
                height: Theme.paddingMedium
            }

            Repeater {
                model: [
                    { icon: "icon-m-favorite", badge: "list", name: qsTr("Bookmarks") },
                    { icon: "icon-m-history", badge: "", name: qsTr("History") },
                    { icon: "icon-m-downloads", badge: "", name: qsTr("Downloads") },
                    { icon: "icon-m-setting", badge: "", name: qsTr("Settings") }
                ]

                MenuListItem {
                    width: sheet.width
                    enabled: false
                    opacity: 1.0
                    iconSource: "image://theme/" + modelData.icon
                    badge: modelData.badge
                    text: modelData.name
                }
            }
        }
    }
}
