// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// What the tutorial says before its lesson and after it, as the platform's own Tutorial
// says it: the screen dimmed under the ambience's darkest highlight, a heading and a few
// lines in the highlight colour, and the buttons that go on from there, one under
// another (docs/DECISIONS/0034-tutorial.md). While it is up, nothing under it takes a
// press.
import QtQuick 2.6
import Sailfish.Silica 1.0

Rectangle {
    id: card

    property string heading
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

    // The sketch under the card is not to be dragged while the card says what it is for.
    MouseArea {
        anchors.fill: parent
    }

    Column {
        anchors.centerIn: parent
        width: parent.width - 2 * Theme.horizontalPageMargin
        spacing: Theme.paddingLarge

        Label {
            objectName: "tutorialCardHeading"
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            color: Theme.highlightColor
            font.pixelSize: Theme.fontSizeExtraLarge
            text: card.heading
        }

        Label {
            objectName: "tutorialCardText"
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
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
