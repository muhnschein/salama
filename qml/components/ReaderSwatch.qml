// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// One of the reader view's colours as the reader settings offer it: a square painted as
// the reader view will be, "Aa" on it in its text colour, and its name under it -- the
// one chosen ringed and named in the highlight colour, as Silica marks what is chosen
// (pages/ReaderSettingsPage.qml, docs/DECISIONS/0024-reader-view.md). Automatic is half
// Firefox's light and half its dark, since it is one or the other as the ambience is; the
// ambience's own look runs down from the ambience's dimmer highlight, with its text and
// highlight colours and its typeface. The colours are the style sheet's, from Reader.
// Not a picture of a page: a choice, tapped to make it.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

BackgroundItem {
    id: swatch

    // A ReaderSettings.Colors value.
    property int colors
    property string text
    property bool selected
    readonly property bool ambience: colors === ReaderSettings.Ambience
    readonly property bool automatic: colors === ReaderSettings.Automatic
    // The scheme it paints: for Automatic, its light half's.
    readonly property string scheme: Reader.schemeFor(colors, false)
    //: A sample of text, a capital and a small letter, drawn in each of the reader
    //: view's colours to choose from
    readonly property string sample: qsTr("Aa")
    readonly property color ink: automatic ? Theme.secondaryColor
                                           : ambience ? Theme.primaryColor
                                                      : Reader.textColorOf(scheme)

    height: square.height + Theme.paddingSmall + name.height + Theme.paddingSmall

    Rectangle {
        id: square

        objectName: "readerSwatchSquare"
        anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(parent.width, Theme.itemSizeSmall)
        height: width
        radius: Theme.paddingMedium
        color: swatch.ambience ? Theme.highlightDimmerColor : Reader.backgroundOf(swatch.scheme)
        gradient: swatch.ambience ? ambienceGradient : null

        Gradient {
            id: ambienceGradient

            GradientStop {
                position: 0.0
                color: Theme.highlightDimmerColor
            }

            GradientStop {
                position: 1.0
                color: Reader.ambienceBackground({
                    "highlightDimmerColor": Theme.highlightDimmerColor,
                    "overlayBackgroundColor": Theme.overlayBackgroundColor
                })
            }
        }

        // Automatic's dark half: its outer corners rounded as the square's, its inner
        // edge straight.
        Rectangle {
            visible: swatch.automatic
            x: parent.width / 2
            width: parent.width / 2
            height: parent.height
            radius: parent.radius
            color: Reader.backgroundOf("dark")

            Rectangle {
                width: parent.radius
                height: parent.height
                color: parent.color
            }
        }

        Row {
            anchors.centerIn: parent

            Label {
                text: swatch.sample.charAt(0)
                font.family: swatch.ambience ? Theme.fontFamily : "sans-serif"
                font.pixelSize: Theme.fontSizeLarge
                color: swatch.ink
            }

            Label {
                text: swatch.sample.substring(1)
                font.family: swatch.ambience ? Theme.fontFamily : "sans-serif"
                font.pixelSize: Theme.fontSizeLarge
                color: swatch.ambience ? Theme.highlightColor : swatch.ink
            }
        }

        // The ring, over whatever the square is painted with.
        Rectangle {
            objectName: "readerSwatchRing"
            anchors.fill: parent
            radius: parent.radius
            color: "transparent"
            border.width: swatch.selected ? 2 * Theme._lineWidth : Theme._lineWidth
            border.color: swatch.selected ? Theme.highlightColor
                                          : Theme.rgba(Theme.primaryColor, Theme.opacityFaint)
        }
    }

    Label {
        id: name

        objectName: "readerSwatchName"
        anchors {
            top: square.bottom
            topMargin: Theme.paddingSmall
            horizontalCenter: parent.horizontalCenter
        }
        width: parent.width
        horizontalAlignment: Text.AlignHCenter
        truncationMode: TruncationMode.Fade
        text: swatch.text
        font.pixelSize: Theme.fontSizeTiny
        color: swatch.selected || swatch.highlighted ? Theme.highlightColor : Theme.primaryColor
    }
}
