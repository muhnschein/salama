// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Real size (Settings.pageZoom px per css px), not scaled. Measures in css px, from
// Reader style sheet.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Rectangle {
    id: preview

    readonly property string scheme: Reader.schemeFor(ReaderSettings.colors,
                                                      Reader.isDarkAmbience(Theme.primaryColor))
    readonly property bool ambience: scheme === "ambience"
    readonly property real cssPixel: Settings.pageZoom(Theme.pixelRatio)
    // ems below relative to this.
    readonly property real em: Reader.fontSizeFor(ReaderSettings.textSize) * cssPixel
    readonly property bool serif: ReaderSettings.typeface === ReaderSettings.Serif
    readonly property string family: serif ? "serif" : ambience ? Theme.fontFamily : "sans-serif"
    readonly property color textColor: ambience ? Theme.primaryColor : Reader.textColorOf(scheme)
    readonly property color linkColor: ambience ? Theme.highlightColor : Reader.linkColorOf(scheme)

    objectName: "readerPreview"
    height: article.height + 2 * article.y
    radius: Theme.paddingSmall
    color: ambience ? Theme.highlightDimmerColor : Reader.backgroundOf(scheme)
    gradient: ambience ? ambienceGradient : null

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

    Item {
        id: article

        x: 20 * preview.cssPixel
        y: x
        width: preview.width - 2 * x
        height: paragraph.y + paragraph.height

        Label {
            id: domain

            objectName: "readerPreviewDomain"
            y: preview.ambience ? heading.height : 0
            width: parent.width
            bottomPadding: 4 * preview.cssPixel
            //: The made-up site a sample article in the reader view's preview is from
            text: qsTr("example.com")
            textFormat: Text.PlainText
            horizontalAlignment: preview.ambience ? Text.AlignRight : Text.AlignLeft
            font.family: preview.ambience ? Theme.fontFamily : "sans-serif"
            font.pixelSize: 0.9 * preview.em
            font.underline: !preview.ambience
            lineHeightMode: Text.FixedHeight
            lineHeight: 1.48 * font.pixelSize
            color: preview.ambience ? Theme.secondaryHighlightColor : preview.linkColor
        }

        Label {
            id: heading

            objectName: "readerPreviewHeading"
            y: preview.ambience ? 0 : domain.height
            width: parent.width
            topPadding: (preview.ambience ? 10 : 30) * preview.cssPixel
            bottomPadding: (preview.ambience ? 4 : 30) * preview.cssPixel
            //: The heading of the sample article in the reader view's preview
            text: qsTr("Just the article")
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
            horizontalAlignment: preview.ambience ? Text.AlignRight : Text.AlignLeft
            font.family: preview.ambience && !preview.serif ? Theme.fontFamilyHeading
                                                             : preview.family
            font.pixelSize: 1.6 * preview.em
            font.weight: preview.ambience ? (preview.serif ? Font.Normal : Font.Light)
                                          : Font.DemiBold
            lineHeightMode: Text.FixedHeight
            lineHeight: 1.25 * font.pixelSize
            color: preview.ambience ? Theme.highlightColor : preview.textColor
        }

        Label {
            id: paragraph

            objectName: "readerPreviewText"
            y: domain.height + heading.height + (preview.ambience ? 15 * preview.cssPixel : 0)
            width: parent.width
            //: The sample article in the reader view's preview
            text: qsTr("The reader view keeps a page's words and pictures and leaves out everything around them, set in the colours, the typeface and the size chosen above.")
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
            font.family: preview.family
            font.pixelSize: preview.em
            lineHeightMode: Text.FixedHeight
            lineHeight: 1.6 * preview.em
            color: preview.textColor
        }
    }
}
