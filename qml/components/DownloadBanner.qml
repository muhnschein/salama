// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The downloads, said on the browsing page on the bar, so that nobody has to go to the
// list to see what became of one (docs/DECISIONS/0038-download-status.md). It comes up
// as a download starts, and speaks for the downloads of this run that have not arrived --
// coming, paused or failed -- as DownloadModel's tray counts them:
//
//  * one: its name, and how far along it is, paused or failed
//  * more: how many, and how far along together
//
// When one arrives it says so for a few seconds. A tap, on it or on its Show, opens the
// list of downloads, where each has its own controls, whatever the banner says; a swipe
// sideways takes it away until a download starts or changes state. It goes once there is
// nothing left to say. It is the bar every banner is (BarBanner.qml), as the one a link
// opened behind the page says it with (docs/DECISIONS/0046-link-menu.md).
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

BarBanner {
    id: banner

    // Whether the page leaves room for it: not while the address is edited, a word is
    // looked for, or the grid is out.
    property bool allowed: true
    // The name of the download that arrived last, said for a moment.
    property string flashName
    readonly property bool flashing: flashTimer.running
    readonly property bool single: !flashing && DownloadModel.trayCount === 1
    readonly property bool failed: single && DownloadModel.trayStatus === DownloadModel.Failed

    function dismiss() {
        flashTimer.stop()
        DownloadModel.dismissTray()
    }

    // A tap: the list, never a file.
    function activate() {
        pageStack.push(Qt.resolvedUrl("../pages/DownloadsPage.qml"))
    }

    function heading() {
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

    // The line under the heading: none for several downloads.
    function line() {
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
    shown: allowed && (flashing || DownloadModel.trayCount > 0)
    title: heading()
    detail: line()
    detailColor: failed ? Theme.errorColor : Theme.secondaryColor
    onActivated: activate()
    onDismissed: dismiss()

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
        highlighted: banner.pressed
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
}
