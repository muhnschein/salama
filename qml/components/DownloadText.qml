// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// What a download is said to be doing, and the icon its file wears: the same words in
// the list of downloads and on the browsing page's banner
// (docs/DECISIONS/0038-download-status.md).
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

QtObject {
    // How far along one still coming is: how much of how much when the size is known,
    // and the percentage alone when it is not.
    function progress(size, percent) {
        if (size > 0) {
            //: A download's progress: "3.1 MB of 7.4 MB · 42%"
            return qsTr("%1 of %2 · %3%").arg(DownloadModel.formatSize(size * percent / 100))
                                        .arg(DownloadModel.formatSize(size)).arg(percent)
        }
        return qsTr("Downloading, %1%").arg(percent)
    }

    // The line under a download's name, in the list.
    function status(download) {
        if (download.status === DownloadModel.Running) {
            return progress(download.size, download.progress)
        }
        if (download.status === DownloadModel.Failed) {
            return qsTr("Failed")
        }
        if (download.status === DownloadModel.Canceled) {
            //: A download stopped part way, which can go on: "Paused · 42%"
            return download.resumable ? qsTr("Paused · %1%").arg(download.progress)
                                      //: A download stopped in an earlier run, which can only start over
                                      : qsTr("Stopped")
        }
        if (!download.fileExists) {
            //: A download whose file has been deleted or moved since
            return qsTr("File not found")
        }
        var site = SearchSettings.displayAddress(download.url)
        return download.size > 0 ? DownloadModel.formatSize(download.size) + " · " + site : site
    }

    // The theme's icon for a kind of file, by its type, or by its name's ending when the
    // type says nothing more than bytes.
    function fileIcon(mimeType, name) {
        var type = String(mimeType).toLowerCase()
        var ending = String(name).toLowerCase().replace(/^.*\./, "")
        if (type.indexOf("image/") === 0) {
            return "image://theme/icon-m-file-image"
        }
        if (type.indexOf("audio/") === 0) {
            return "image://theme/icon-m-file-audio"
        }
        if (type.indexOf("video/") === 0) {
            return "image://theme/icon-m-file-video"
        }
        if (type === "application/pdf" || ending === "pdf") {
            return "image://theme/icon-m-file-pdf"
        }
        if (type === "application/x-rpm" || ending === "rpm") {
            return "image://theme/icon-m-file-rpm"
        }
        if (/zip|tar|gzip|bzip|xz|7z|rar|compressed/.test(type)
                || ["zip", "tar", "gz", "bz2", "xz", "7z", "rar"].indexOf(ending) >= 0) {
            return "image://theme/icon-m-file-archive-folder"
        }
        if (type.indexOf("text/") === 0 || /document|msword|opendocument/.test(type)) {
            return "image://theme/icon-m-file-document"
        }
        return "image://theme/icon-m-file-other"
    }
}
