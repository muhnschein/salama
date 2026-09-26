// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// What the tutorial says before its lessons and after them, as the platform's own Tutorial
// says it: the screen dimmed under the ambience's darkest highlight, words in the
// highlight colour, and the buttons that go on from there, one under another
// (docs/DECISIONS/0034-tutorial.md). The first one also carries the application's mark
// over its name, as piirit's first screen does, so that the first thing a first start
// shows and the launcher say the same thing. While it is up, nothing under it takes a
// press.
import QtQuick 2.6
import Sailfish.Silica 1.0

Rectangle {
    id: card

    // The launcher icon over the heading, for the first card.
    property bool showLogo: false
    property string heading
    // A line under the heading, in the primary colour.
    property string subheading
    property string text
    // The buttons, laid out under the words.
    default property alias buttons: buttonColumn.data

    // As the Tutorial's own: the dimmer colour at nine tenths, which is what Silica's
    // InteractionHintLabel lays under its words too.
    color: Theme.rgba(Theme.highlightDimmerColor, 0.9)
    visible: opacity > 0

    Behavior on opacity {
        FadeAnimation {}
    }

    // The sketch under the card is not to be dragged while the card is up.
    MouseArea {
        anchors.fill: parent
    }

    Column {
        anchors {
            horizontalCenter: parent.horizontalCenter
            verticalCenter: parent.verticalCenter
            // A little above the middle, where a title sits.
            verticalCenterOffset: -parent.height * 0.04
        }
        width: parent.width - 2 * Theme.horizontalPageMargin
        spacing: Theme.paddingLarge

        // The launcher icon, drawn larger (art/logo.png is icons/harbour-salama.svg
        // rendered by icons/render.sh), and decoded at the size it is drawn at.
        Image {
            objectName: "tutorialLogo"
            x: (parent.width - width) / 2
            width: Theme.itemSizeExtraLarge
            height: width
            visible: card.showLogo
            sourceSize.width: width
            sourceSize.height: height
            fillMode: Image.PreserveAspectFit
            smooth: true
            source: card.showLogo ? Qt.resolvedUrl("../../art/logo.png") : ""
        }

        Label {
            objectName: "tutorialCardHeading"
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            textFormat: Text.PlainText
            color: Theme.highlightColor
            font.family: Theme.fontFamilyHeading
            font.pixelSize: card.showLogo ? Theme.fontSizeHuge : Theme.fontSizeExtraLarge
            text: card.heading
        }

        Label {
            objectName: "tutorialCardSubheading"
            width: parent.width
            visible: text.length > 0
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            textFormat: Text.PlainText
            color: Theme.primaryColor
            font.pixelSize: Theme.fontSizeLarge
            text: card.subheading
        }

        Label {
            objectName: "tutorialCardText"
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            textFormat: Text.PlainText
            color: Theme.highlightColor
            text: card.text
        }

        Item {
            width: parent.width
            height: Theme.paddingLarge
        }

        Column {
            id: buttonColumn

            anchors.horizontalCenter: parent.horizontalCenter
            spacing: Theme.paddingLarge
        }
    }
}
