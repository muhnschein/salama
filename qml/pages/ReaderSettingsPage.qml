// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
// Preview below choices so text-size growth never moves slider under finger.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

Page {
    id: readerSettingsPage

    property SettingNames names: SettingNames {}

    objectName: "readerSettingsPage"
    allowedOrientations: Orientation.Portrait | Orientation.LandscapeMask

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column

            width: parent.width

            PageHeader {
                title: qsTr("Reader view")
            }

            SectionHeader {
                text: qsTr("Colours")
            }

            // Display order differs from stored (ReaderSettings::Colors): each square carries own.
            Row {
                id: swatches

                readonly property real cellWidth: (width - 4 * spacing) / 5

                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                spacing: Theme.paddingSmall

                Repeater {
                    model: [ReaderSettings.Automatic, ReaderSettings.Ambience,
                            ReaderSettings.Light, ReaderSettings.Sepia, ReaderSettings.Dark]

                    ReaderSwatch {
                        objectName: "readerColorsChoice"
                        width: swatches.cellWidth
                        colors: modelData
                        text: readerSettingsPage.names.readerColors(modelData)
                        selected: ReaderSettings.colors === modelData
                        onClicked: ReaderSettings.colors = modelData
                    }
                }
            }

            SectionHeader {
                text: qsTr("Typeface")
            }

            Row {
                id: typefaces

                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                spacing: Theme.paddingMedium

                Repeater {
                    model: [ReaderSettings.SansSerif, ReaderSettings.Serif]

                    BackgroundItem {
                        id: typeface

                        readonly property bool selected: ReaderSettings.typeface === modelData

                        objectName: "readerTypefaceChoice"
                        width: (typefaces.width - typefaces.spacing) / 2
                        height: Theme.itemSizeLarge
                        onClicked: ReaderSettings.typeface = modelData

                        Rectangle {
                            anchors.fill: parent
                            radius: Theme.paddingMedium
                            color: typeface.selected
                                   ? Theme.rgba(Theme.highlightBackgroundColor,
                                                Theme.highlightBackgroundOpacity)
                                   : Theme.rgba(Theme.primaryColor, Theme.opacityFaint / 2)
                            border.width: typeface.selected ? 2 * Theme._lineWidth : 0
                            border.color: Theme.highlightColor
                        }

                        Column {
                            anchors.centerIn: parent

                            Label {
                                anchors.horizontalCenter: parent.horizontalCenter
                                //: A sample of text in each of the reader view's typefaces
                                text: qsTr("Aa")
                                font.family: modelData === ReaderSettings.Serif ? "serif"
                                                                                : "sans-serif"
                                font.pixelSize: Theme.fontSizeLarge
                                color: typeface.selected ? Theme.highlightColor
                                                         : Theme.primaryColor
                            }

                            Label {
                                objectName: "readerTypefaceName"
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: readerSettingsPage.names.typeface(modelData)
                                font.pixelSize: Theme.fontSizeExtraSmall
                                color: typeface.selected ? Theme.highlightColor
                                                         : Theme.primaryColor
                            }
                        }
                    }
                }
            }

            Item {
                width: parent.width
                height: Theme.paddingLarge
            }

            // Nine steps, middle = default, shown as share of it.
            Slider {
                objectName: "readerTextSizeSlider"
                width: parent.width
                label: qsTr("Text size")
                minimumValue: ReaderSettings.TextSizeMin
                maximumValue: ReaderSettings.TextSizeMax
                stepSize: 1
                value: ReaderSettings.textSize
                valueText: readerSettingsPage.names.textSize(value)
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
