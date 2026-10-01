// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The downloads, said on the browsing page above the bar, so that nobody has to go to
// the list to see what became of one (docs/DECISIONS/0038-download-status.md). It
// comes up as a download starts, and speaks for the downloads of this run that have not
// arrived -- coming, paused or failed -- as DownloadModel's tray counts them:
//
//  * one: its name, and how far along it is, paused or failed
//  * more: how many, and how far along together
//
// When one arrives it says so for a few seconds. A tap opens the list of downloads,
// where each has its own controls, whatever the banner says; a swipe sideways takes it
// away until a download starts or changes state. It goes once there is nothing left to
// say.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Item {
    id: banner

    // Whether the page leaves room for it: not while the address is edited, a word is
    // looked for, or the grid is out.
    property bool allowed: true
    // The name of the download that arrived last, said for a moment.
    property string flashName
    readonly property bool flashing: flashTimer.running
    readonly property bool shown: allowed && (flashing || DownloadModel.trayCount > 0)
    readonly property bool single: !flashing && DownloadModel.trayCount === 1

    function dismiss() {
        flashTimer.stop()
        DownloadModel.dismissTray()
    }

    // A tap: the list, never a file.
    function activate() {
        pageStack.push(Qt.resolvedUrl("../pages/DownloadsPage.qml"))
    }

    function title() {
        if (flashing) {
            return flashName
        }
        if (single) {
            return DownloadModel.trayNames[0] || ""
        }
        //: The banner over several downloads, and how far along they are together: "3 downloads · 42%"
        return qsTr("%n download(s)", "", DownloadModel.trayCount) + " · "
                + DownloadModel.trayProgress + "%"
    }

    // The line under the title: none for several downloads.
    function detail() {
        if (flashing) {
            //: The banner as a download arrives
            return qsTr("Downloaded")
        }
        if (!single) {
            return ""
        }
        if (DownloadModel.trayStatus === DownloadModel.Failed) {
            return qsTr("Failed")
        }
        if (DownloadModel.trayStatus === DownloadModel.Canceled) {
            return qsTr("Paused · %1%").arg(DownloadModel.trayProgress)
        }
        return says.progress(DownloadModel.traySize, DownloadModel.trayProgress)
    }

    objectName: "downloadBanner"
    height: card.height + 2 * Theme.paddingSmall
    visible: opacity > 0
    opacity: shown ? 1.0 : 0.0

    Behavior on opacity {
        FadeAnimation {}
    }

    DownloadText {
        id: says
    }

    Connections {
        target: DownloadModel
        onFinished: {
            banner.flashName = name
            flashTimer.restart()
        }
    }

    Timer {
        id: flashTimer

        objectName: "downloadBannerFlash"
        interval: 4000
    }

    // What a sideways drag moves; the card follows it, fading as it goes.
    Item {
        id: handle

        objectName: "downloadBannerHandle"
    }

    Rectangle {
        id: card

        objectName: "downloadBannerCard"
        x: Theme.paddingMedium
        y: Theme.paddingSmall
        width: parent.width - 2 * Theme.paddingMedium
        height: Theme.itemSizeMedium
        radius: Theme.paddingMedium
        color: Theme.rgba(Theme.overlayBackgroundColor, Theme.opacityOverlay)
        opacity: 1.0 - Math.min(1.0, Math.abs(handle.x) / width)
        transform: Translate {
            x: handle.x
        }

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            visible: touch.pressed && !touch.drag.active
            color: Theme.rgba(Theme.highlightBackgroundColor, Theme.highlightBackgroundOpacity)
        }

        MouseArea {
            id: touch

            objectName: "downloadBannerTouch"
            anchors.fill: parent
            drag {
                target: handle
                axis: Drag.XAxis
                minimumX: -card.width
                maximumX: card.width
            }
            onClicked: banner.activate()
            onReleased: {
                if (Math.abs(handle.x) > card.width / 3) {
                    banner.dismiss()
                }
                handle.x = 0
            }
        }

        Item {
            id: badge

            anchors {
                left: parent.left
                leftMargin: Theme.paddingMedium
                verticalCenter: parent.verticalCenter
            }
            width: Theme.iconSizeMedium
            height: width

            ProgressCircle {
                objectName: "downloadBannerProgress"
                anchors.fill: parent
                value: banner.flashing ? 1.0 : DownloadModel.trayProgress / 100
                progressColor: !banner.flashing && DownloadModel.trayFailed === DownloadModel.trayCount
                               ? Theme.errorColor : Theme.highlightColor
                backgroundColor: Theme.rgba(Theme.primaryColor, Theme.opacityFaint)
                borderWidth: Theme.paddingSmall / 2
            }

            Icon {
                objectName: "downloadBannerIcon"
                anchors.centerIn: parent
                source: banner.flashing ? "image://theme/icon-m-acknowledge"
                                        : "image://theme/icon-m-downloads"
                sourceSize: Qt.size(Theme.iconSizeSmall, Theme.iconSizeSmall)
                highlighted: touch.pressed
            }
        }

        Column {
            anchors {
                left: badge.right
                leftMargin: Theme.paddingMedium
                right: parent.right
                rightMargin: Theme.paddingMedium
                verticalCenter: parent.verticalCenter
            }

            Label {
                objectName: "downloadBannerTitle"
                width: parent.width
                text: banner.title()
                truncationMode: TruncationMode.Fade
                font.pixelSize: Theme.fontSizeSmall
                color: touch.pressed ? Theme.highlightColor : Theme.primaryColor
            }

            Label {
                objectName: "downloadBannerDetail"
                width: parent.width
                visible: text.length > 0
                text: banner.detail()
                truncationMode: TruncationMode.Fade
                font.pixelSize: Theme.fontSizeExtraSmall
                color: banner.single && DownloadModel.trayStatus === DownloadModel.Failed
                       ? Theme.errorColor : Theme.secondaryColor
            }
        }
    }
}
