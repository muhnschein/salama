// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Swipe hides until a download starts or changes state.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

BarBanner {
    id: banner

    // False while address edited, find open, or grid out.
    property bool allowed: true
    property string flashName
    readonly property bool flashing: flashTimer.running
    readonly property bool single: !flashing && DownloadModel.trayCount === 1
    readonly property bool failed: single && DownloadModel.trayStatus === DownloadModel.Failed

    function dismiss() {
        flashTimer.stop()
        DownloadModel.dismissTray()
    }

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
