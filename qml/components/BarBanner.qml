// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// One bar along the top of the navigation bar, saying something that happened away from
// the page in front: the downloads coming (DownloadBanner), a link opened in a tab behind
// it (TabBanner). Both are this bar, so they read as one kind of thing
// (docs/DECISIONS/0038-download-status.md, 0046-link-menu.md): the width of the screen,
// on the sheets' ground with its edge in the highlight colour (SheetBackground), a slot
// at its start for what stands for it, its words, and at its end the one thing to do
// about it, Show. A tap anywhere on it does the same; a swipe sideways takes it away.
//
// It lies on the bar, not over the page: the browsing page ends where the banners begin.
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: banner

    property bool shown: false
    property string title
    property string detail
    property color detailColor: Theme.secondaryColor
    // What stands for it, in the slot at its start.
    default property alias badge: badgeSlot.data
    readonly property bool pressed: touch.pressed

    // A tap, on the bar or on Show.
    signal activated()
    // Swiped away.
    signal dismissed()

    height: Theme.itemSizeMedium
    visible: opacity > 0
    opacity: shown ? 1.0 : 0.0

    Behavior on opacity {
        FadeAnimation {}
    }

    // What a sideways drag moves; the bar follows it, fading as it goes.
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
                if (Math.abs(handle.x) > bar.width / 3) {
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

        // The one thing to do about it, in the highlight colour as a Silica text button is.
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
