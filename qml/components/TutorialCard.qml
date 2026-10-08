// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
import QtQuick 2.6
import Sailfish.Silica 1.0

Rectangle {
    id: card

    property bool showLogo: false
    property bool showCheck: false
    // Each {icon, text}.
    property var topics: []
    property string heading
    property string subheading
    property string text
    default property alias buttons: buttonColumn.data

    color: Theme.rgba(Theme.highlightDimmerColor, 0.9)
    visible: opacity > 0

    Behavior on opacity {
        FadeAnimation {}
    }

    // Blocks sketch drags below.
    MouseArea {
        anchors.fill: parent
    }

    Column {
        anchors {
            horizontalCenter: parent.horizontalCenter
            verticalCenter: parent.verticalCenter
            verticalCenterOffset: -parent.height * 0.04
        }
        width: parent.width - 2 * Theme.horizontalPageMargin
        spacing: Theme.paddingLarge

        // art/logo.png rendered from icons/harbour-salama.svg by icons/render.sh.
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

        // Drawn, not theme icon, so same on every ambience.
        Canvas {
            id: check

            objectName: "tutorialCheck"
            x: (parent.width - width) / 2
            width: Theme.itemSizeLarge
            height: width
            visible: card.showCheck
            onPaint: {
                var context = getContext("2d")
                var line = Theme._lineWidth * 1.5
                context.reset()
                context.lineWidth = line
                context.strokeStyle = Theme.highlightColor
                context.lineCap = "round"
                context.lineJoin = "round"
                context.beginPath()
                context.arc(width / 2, height / 2, width / 2 - line, 0, 2 * Math.PI)
                context.stroke()
                context.beginPath()
                context.moveTo(width * 0.3, height * 0.52)
                context.lineTo(width * 0.44, height * 0.66)
                context.lineTo(width * 0.71, height * 0.37)
                context.stroke()
            }
        }

        Label {
            objectName: "tutorialCardHeading"
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            textFormat: Text.PlainText
            color: Theme.highlightColor
            font.family: Theme.fontFamilyHeading
            font.weight: Font.Light
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
            font.pixelSize: Theme.fontSizeMedium
            text: card.subheading
        }

        Label {
            objectName: "tutorialCardText"
            width: parent.width
            visible: text.length > 0
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            textFormat: Text.PlainText
            color: Theme.secondaryHighlightColor
            text: card.text
        }

        Row {
            objectName: "tutorialTopics"
            anchors.horizontalCenter: parent.horizontalCenter
            visible: card.topics.length > 0
            topPadding: Theme.paddingLarge

            Repeater {
                model: card.topics

                Column {
                    objectName: "tutorialTopic"
                    width: Theme.itemSizeExtraLarge * 1.2
                    spacing: Theme.paddingSmall

                    Icon {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: Theme.iconSizeMedium
                        height: width
                        source: modelData.icon
                        color: Theme.highlightColor
                    }

                    Label {
                        objectName: "tutorialTopicName"
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap
                        font.pixelSize: Theme.fontSizeExtraSmall
                        color: Theme.secondaryHighlightColor
                        text: modelData.text
                    }
                }
            }
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
