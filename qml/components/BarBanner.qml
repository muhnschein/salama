// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: banner

    property bool shown: false
    property string title
    property string detail
    property color detailColor: Theme.secondaryColor
    default property alias badge: badgeSlot.data
    readonly property bool pressed: touch.pressed
    // Portrait's swipe in landscape too: third of wide bar is long reach. Screen is portrait.
    readonly property real dismissDistance: Math.min(bar.width, Screen.width) / 3

    signal activated()
    signal dismissed()

    height: Theme.itemSizeMedium
    visible: opacity > 0
    opacity: shown ? 1.0 : 0.0

    Behavior on opacity {
        FadeAnimation {}
    }

    Item {
        id: handle

        objectName: "bannerHandle"
    }

    Item {
        id: bar

        objectName: "bannerBar"
        width: parent.width
        height: parent.height
        opacity: 1.0 - Math.min(1.0, Math.abs(handle.x) / width)
        transform: Translate {
            x: handle.x
        }

        SheetBackground {
            anchors.fill: parent
        }

        Rectangle {
            anchors.fill: parent
            visible: touch.pressed && !touch.drag.active
            color: Theme.rgba(Theme.highlightBackgroundColor, Theme.highlightBackgroundOpacity)
        }

        MouseArea {
            id: touch

            objectName: "bannerTouch"
            anchors.fill: parent
            drag {
                target: handle
                axis: Drag.XAxis
                minimumX: -bar.width
                maximumX: bar.width
            }
            onClicked: banner.activated()
            onReleased: {
                if (Math.abs(handle.x) > banner.dismissDistance) {
                    banner.dismissed()
                }
                handle.x = 0
            }
        }

        Item {
            id: badgeSlot

            objectName: "bannerBadge"
            anchors {
                left: parent.left
                leftMargin: Theme.horizontalPageMargin
                verticalCenter: parent.verticalCenter
            }
            width: Theme.iconSizeMedium
            height: width
        }

        Column {
            anchors {
                left: badgeSlot.right
                leftMargin: Theme.paddingMedium
                right: action.left
                verticalCenter: parent.verticalCenter
            }

            Label {
                objectName: "bannerTitle"
                width: parent.width
                text: banner.title
                textFormat: Text.PlainText
                truncationMode: TruncationMode.Fade
                font.pixelSize: Theme.fontSizeSmall
                color: touch.pressed ? Theme.highlightColor : Theme.primaryColor
            }

            Label {
                objectName: "bannerDetail"
                width: parent.width
                visible: text.length > 0
                text: banner.detail
                textFormat: Text.PlainText
                truncationMode: TruncationMode.Fade
                font.pixelSize: Theme.fontSizeExtraSmall
                color: banner.detailColor
            }
        }

        BackgroundItem {
            id: action

            objectName: "bannerAction"
            anchors {
                right: parent.right
                rightMargin: Theme.horizontalPageMargin - Theme.paddingLarge
            }
            width: actionLabel.width + 2 * Theme.paddingLarge
            height: parent.height
            onClicked: banner.activated()

            Label {
                id: actionLabel

                objectName: "bannerActionLabel"
                anchors.centerIn: parent
                //: The banner over the navigation bar: what it is about, brought up
                text: qsTr("Show")
                font.pixelSize: Theme.fontSizeSmall
                color: action.highlighted ? Theme.primaryColor : Theme.highlightColor
            }
        }
    }
}
