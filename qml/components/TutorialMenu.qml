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
    readonly property real gap: Theme.paddingLarge
    readonly property real column: Math.floor((width * 0.42 - gap) / 2)
    readonly property real labelSize: wide ? Theme.fontSizeSmall : Theme.fontSizeTiny

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
        width: menu.wide ? 2 * menu.column + menu.gap : parent.width
        height: menu.wide ? Math.min(content.height, menu.barTop) : content.height

        SheetBackground {
            anchors.fill: parent
            side: menu.wide
        }

        MouseArea {
            anchors.fill: parent
        }

        SheetGrip {
            edgeOf: sheet
            upright: menu.wide
        }

        // As menu's: head over rows; side sheet's rows in two columns under a line.
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

            Item {
                width: parent.width
                height: Math.max(0, Theme.paddingLarge + Theme.paddingMedium - header.inkMargin)
                visible: !menu.wide
            }

            MenuSeparator {
                width: parent.width
                visible: menu.wide
            }

            Grid {
                columns: menu.wide ? 3 : 1

                Grid {
                    id: pageRow

                    readonly property real cell: menu.wide ? menu.column : menu.width / 5

                    objectName: "tutorialMenuPageRow"
                    width: menu.wide ? cell : 5 * cell
                    columns: menu.wide ? 1 : 5

                    Repeater {
                        model: [
                            { icon: "icon-m-search-on-page", name: qsTr("Find in page") },
                            { icon: "icon-m-favorite", name: qsTr("Bookmark") },
                            { icon: "icon-m-share", name: qsTr("Share") },
                            { icon: "icon-m-computer", name: qsTr("Desktop site") },
                            { icon: "icon-m-file-formatted", name: qsTr("Reader view") }
                        ]

                        MenuButton {
                            width: pageRow.cell
                            labelSize: menu.labelSize
                            across: menu.wide
                            round: true
                            enabled: false
                            opacity: 1.0
                            iconSource: "image://theme/" + modelData.icon
                            text: modelData.name
                        }
                    }
                }

                MenuSeparator {
                    width: menu.wide ? menu.gap : menu.width
                    height: menu.wide ? pageRow.height : Theme.paddingLarge
                    vertical: menu.wide
                }

                Grid {
                    id: placeRow

                    readonly property real cell: menu.wide ? menu.column : menu.width / 4

                    objectName: "tutorialMenuPlaceRow"
                    width: menu.wide ? cell : 4 * cell
                    columns: menu.wide ? 1 : 4

                    Repeater {
                        model: [
                            { icon: "icon-m-favorite-selected", name: qsTr("Bookmarks") },
                            { icon: "icon-m-history", name: qsTr("History") },
                            { icon: "icon-m-downloads", name: qsTr("Downloads") },
                            { icon: "icon-m-setting", name: qsTr("Settings") }
                        ]

                        MenuButton {
                            width: placeRow.cell
                            labelSize: menu.labelSize
                            across: menu.wide
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
}
