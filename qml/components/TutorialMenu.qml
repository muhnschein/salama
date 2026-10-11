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
    // Landscape: head at left, portrait's rows beside it, as menu.
    readonly property SheetCells cells: SheetCells {
        width: menu.width
        height: menu.height
    }
    readonly property bool wide: cells.wide
    readonly property real rowsWidth: wide ? cells.block : width

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

        // Portrait: one column, head over rows. Landscape: head beside rows.
        Grid {
            id: content

            width: parent.width
            columns: menu.wide ? 3 : 1
            verticalItemAlignment: Grid.AlignVCenter
            topPadding: Theme.paddingMedium
            bottomPadding: Theme.paddingMedium

            MenuPageHeader {
                id: header

                width: menu.wide ? menu.cells.headWidth : parent.width
                height: menu.wide ? rows.height : Theme.itemSizeSmall
                wide: menu.wide
                url: "https://sailfishos.org"
                title: menu.pageTitle
            }

            Item {
                width: parent.width
                height: Math.max(0, Theme.paddingLarge + Theme.paddingMedium - header.inkMargin)
                visible: !menu.wide
            }

            MenuSeparator {
                width: menu.cells.gap
                height: rows.height
                visible: menu.wide
                vertical: true
            }

            Column {
                id: rows

                width: menu.rowsWidth

                Grid {
                    id: pageRow

                    readonly property real cell: menu.rowsWidth / 5

                    objectName: "tutorialMenuPageRow"
                    width: 5 * cell
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
                            width: pageRow.cell
                            labelSize: menu.wide ? Theme.fontSizeExtraSmall : Theme.fontSizeTiny
                            round: true
                            enabled: false
                            opacity: 1.0
                            iconSource: "image://theme/" + modelData.icon
                            text: modelData.name
                        }
                    }
                }

                MenuSeparator {
                    width: parent.width
                }

                Grid {
                    id: placeRow

                    readonly property real cell: menu.rowsWidth / 4

                    objectName: "tutorialMenuPlaceRow"
                    width: 4 * cell
                    columns: 4

                    Repeater {
                        model: [
                            { icon: "icon-m-favorite-selected", name: qsTr("Bookmarks") },
                            { icon: "icon-m-history", name: qsTr("History") },
                            { icon: "icon-m-downloads", name: qsTr("Downloads") },
                            { icon: "icon-m-setting", name: qsTr("Settings") }
                        ]

                        MenuButton {
                            width: placeRow.cell
                            labelSize: menu.wide ? Theme.fontSizeExtraSmall : Theme.fontSizeTiny
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
