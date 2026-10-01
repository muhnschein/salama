// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Where a download stands, in a circle the size of a medium icon, at the head of its row
// in the list and of the bar over the browsing page
// (docs/DECISIONS/0038-download-controls.md):
//
//  * still coming: the ring the menu's Downloads wears, filled as far as it has come,
//    round the stop it is stopped by -- or, where it cannot be stopped from, round the
//    kind of file it is;
//  * failed or stopped: the arrow it is fetched again by, in the error colour for one
//    that failed, where it can be; the kind of file it is, dimmed, where it cannot;
//  * arrived: the kind of file it is, dimmed once the file is no longer there.
//
// A tap on the stop or the arrow is clicked(); the rest of the circle is the row's.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Item {
    id: indicator

    // A DownloadModel.Status, how far along it is from 0 to 100, and the theme's icon
    // for the kind of file it is.
    property int status: DownloadModel.Running
    property int progress: 0
    property string fileIcon: "image://theme/icon-m-file-other"
    property bool retryable: false
    property bool fileExists: true
    // Whether the stop and the arrow are offered here.
    property bool actionable: false
    property bool highlighted: false

    readonly property bool running: status === DownloadModel.Running
    readonly property bool ended: status === DownloadModel.Failed
                                  || status === DownloadModel.Canceled
    // What a tap here would do: "stop", "retry", or nothing.
    readonly property string action: !actionable ? ""
                                     : running ? "stop"
                                     : ended && retryable ? "retry" : ""

    signal clicked()

    objectName: "downloadIndicator"
    width: Theme.iconSizeMedium
    height: width

    DownloadRing {
        objectName: "downloadIndicatorProgress"
        anchors.fill: parent
        visible: indicator.running
        value: indicator.progress / 100
    }

    Icon {
        objectName: "downloadIndicatorIcon"
        anchors.centerIn: parent
        source: indicator.action === "stop" ? "image://theme/icon-m-clear"
              : indicator.action === "retry" ? "image://theme/icon-m-refresh"
              : indicator.fileIcon
        // Inside the ring, a step down, as the menu's icons are inside theirs.
        sourceSize: indicator.running ? Qt.size(Theme.iconSizeSmallPlus, Theme.iconSizeSmallPlus)
                                      : Qt.size(Theme.iconSizeMedium, Theme.iconSizeMedium)
        color: indicator.action === "retry" && indicator.status === DownloadModel.Failed
               ? Theme.errorColor : (indicator.highlighted ? Theme.highlightColor
                                                           : Theme.primaryColor)
        highlighted: indicator.highlighted || stopArea.pressed
        opacity: (indicator.status === DownloadModel.Done && !indicator.fileExists)
                 || (indicator.ended && indicator.action === "")
                 ? Theme.opacityLow : 1.0
    }

    // A touch area as wide as the row is tall, so the stop is not a fingertip's width.
    MouseArea {
        id: stopArea

        objectName: "downloadIndicatorButton"
        anchors {
            fill: parent
            margins: -Theme.paddingMedium
        }
        enabled: indicator.action !== ""
        onClicked: indicator.clicked()
    }
}
