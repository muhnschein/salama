// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Download start, arrival and failure said for a moment, Silica's way. Progress lives in menu's
// Downloads row and on Downloads page.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Notice {
    id: notice

    function say(message) {
        text = message
        show()
    }

    objectName: "downloadNotice"
    duration: Notice.Short
    // Over nav bar.
    verticalOffset: -Theme.itemSizeLarge

    Connections {
        target: DownloadModel
        //: Shown for a moment as a download starts: "Downloading report.pdf"
        onStarted: notice.say(qsTr("Downloading %1").arg(name))
        //: Shown for a moment as a download arrives: "Downloaded report.pdf"
        onFinished: notice.say(qsTr("Downloaded %1").arg(name))
        //: Shown for a moment as a download fails: "Download failed: report.pdf"
        onFailed: notice.say(qsTr("Download failed: %1").arg(name))
    }
}
