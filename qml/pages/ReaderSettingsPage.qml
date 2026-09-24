// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// How the reader view sets an article, as Firefox's reader view offers it: its
// colours, typeface and text size, each index or value the stored one
// (docs/DECISIONS/0024-reader-view.md, 0028-settings-pages.md). A reader view on the
// screen follows them at once.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Page {
    id: readerSettingsPage

    objectName: "readerSettingsPage"
    allowedOrientations: Orientation.Portrait

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column

            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: qsTr("Reader view")
            }

            ComboBox {
                objectName: "readerColorsCombo"
                width: parent.width
                label: qsTr("Colours")
                currentIndex: Settings.readerColors
                menu: ContextMenu {
                    MenuItem {
                        text: qsTr("Ambience")
                    }

                    MenuItem {
                        text: qsTr("Light")
                    }

                    MenuItem {
                        text: qsTr("Sepia")
                    }

                    MenuItem {
                        text: qsTr("Dark")
                    }
                }
                onCurrentIndexChanged: Settings.readerColors = currentIndex
            }

            ComboBox {
                objectName: "readerTypefaceCombo"
                width: parent.width
                label: qsTr("Typeface")
                currentIndex: Settings.readerTypeface
                menu: ContextMenu {
                    MenuItem {
                        text: qsTr("Sans serif")
                    }

                    MenuItem {
                        text: qsTr("Serif")
                    }
                }
                onCurrentIndexChanged: Settings.readerTypeface = currentIndex
            }

            // Firefox's nine steps, the middle one its default, written as a share of it.
            Slider {
                objectName: "readerTextSizeSlider"
                width: parent.width
                label: qsTr("Text size")
                minimumValue: Settings.ReaderTextSizeMin
                maximumValue: Settings.ReaderTextSizeMax
                stepSize: 1
                value: Settings.readerTextSize
                valueText: qsTr("%1 %").arg(Math.round(100 * (10 + 2 * value)
                                                       / (10 + 2 * Settings.ReaderTextSizeDefault)))
                onValueChanged: Settings.readerTextSize = Math.round(value)
            }
        }

        VerticalScrollDecorator {}
    }
}
