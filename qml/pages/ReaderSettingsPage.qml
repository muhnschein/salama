// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// How the reader view sets an article, as Firefox's reader view offers it: its
// colours, typeface and text size, each index or value the stored one
// (docs/DECISIONS/0024-reader-view.md, 0028-settings-pages.md). Under them, a few lines
// of an article as the reader view will set them follow each choice as it is made; a
// reader view on the screen follows them at once too. The picture is below the choices
// rather than above, so a text size growing it never moves the slider from under the
// finger dragging it.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

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
                currentIndex: ReaderSettings.colors
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
                onCurrentIndexChanged: ReaderSettings.colors = currentIndex
            }

            ComboBox {
                objectName: "readerTypefaceCombo"
                width: parent.width
                label: qsTr("Typeface")
                currentIndex: ReaderSettings.typeface
                menu: ContextMenu {
                    MenuItem {
                        text: qsTr("Sans serif")
                    }

                    MenuItem {
                        text: qsTr("Serif")
                    }
                }
                onCurrentIndexChanged: ReaderSettings.typeface = currentIndex
            }

            // Firefox's nine steps, the middle one its default, written as a share of it.
            Slider {
                objectName: "readerTextSizeSlider"
                width: parent.width
                label: qsTr("Text size")
                minimumValue: ReaderSettings.TextSizeMin
                maximumValue: ReaderSettings.TextSizeMax
                stepSize: 1
                value: ReaderSettings.textSize
                valueText: qsTr("%1 %").arg(Math.round(100 * (10 + 2 * value)
                                                       / (10 + 2 * ReaderSettings.TextSizeDefault)))
                onValueChanged: ReaderSettings.textSize = Math.round(value)
            }

            ReaderPreview {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
            }
        }

        VerticalScrollDecorator {}
    }
}
