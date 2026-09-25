// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// A few lines of an article as the reader view will set them: on the reader settings'
// page, under the choices, following each as it is made (pages/ReaderSettingsPage.qml,
// docs/DECISIONS/0024-reader-view.md). The site's name in the colour of a link, a
// heading and a paragraph, in the theme's colours from the style sheet (Reader), in its
// typeface, and at its text size as many screen pixels large as the engine makes a css
// pixel (Settings.pageZoom) -- the reader view's own size, not a smaller picture of it.
// Every measure is the style sheet's, in css pixels. Not a button.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Rectangle {
    id: preview

    readonly property string scheme: Reader.schemeFor(Settings.readerColors,
                                                      Reader.isDarkAmbience(Theme.primaryColor))
    readonly property real cssPixel: Settings.pageZoom(Theme.pixelRatio)
    // The article's --font-size, which every em below is of.
    readonly property real em: Reader.fontSizeFor(Settings.readerTextSize) * cssPixel
    readonly property string family: Settings.readerTypeface === Settings.ReaderSerif ? "serif"
                                                                                      : "sans-serif"

    objectName: "readerPreview"
    height: article.height + 2 * article.y
    radius: Theme.paddingSmall
    color: Reader.backgroundOf(scheme)

    Column {
        id: article

        // The body's padding.
        x: 20 * preview.cssPixel
        y: x
        width: preview.width - 2 * x

        // .domain: the site the article came from, in the sans-serif whatever the
        // typeface.
        Label {
            objectName: "readerPreviewDomain"
            width: parent.width
            bottomPadding: 4 * preview.cssPixel
            //: The made-up site a sample article in the reader view's preview is from
            text: qsTr("example.com")
            textFormat: Text.PlainText
            font.family: "sans-serif"
            font.pixelSize: 0.9 * preview.em
            font.underline: true
            lineHeightMode: Text.FixedHeight
            lineHeight: 1.48 * font.pixelSize
            color: Reader.linkColorOf(preview.scheme)
        }

        // .header > h1, with its margin above and below.
        Label {
            objectName: "readerPreviewHeading"
            width: parent.width
            topPadding: 30 * preview.cssPixel
            bottomPadding: topPadding
            //: The heading of the sample article in the reader view's preview
            text: qsTr("Just the article")
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
            font.family: preview.family
            font.pixelSize: 1.6 * preview.em
            font.weight: Font.DemiBold
            lineHeightMode: Text.FixedHeight
            lineHeight: 1.25 * font.pixelSize
            color: Reader.textColorOf(preview.scheme)
        }

        // A paragraph of the article, a line of it as tall as the style sheet makes one.
        Label {
            objectName: "readerPreviewText"
            width: parent.width
            //: The sample article in the reader view's preview
            text: qsTr("The reader view keeps a page's words and pictures and leaves out everything around them, set in the colours, the typeface and the size chosen above.")
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
            font.family: preview.family
            font.pixelSize: preview.em
            lineHeightMode: Text.FixedHeight
            lineHeight: 1.6 * preview.em
            color: Reader.textColorOf(preview.scheme)
        }
    }
}
