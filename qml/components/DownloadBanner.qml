// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// What the page says of the downloads happening: a strip along the top of the
// navigation bar, the colour of the bar, that comes up while anything is coming and
// goes when nothing is (docs/DECISIONS/0038-download-notice.md).
//
// It is about the downloads together, never about one of them in turn: a ring filled as
// far as they have come together, with the percentage in it, how many are still coming,
// and how many have finished recently. A second download starting does not take the
// first's place, and a first finishing does not take the strip away while a second is
// still coming: what it says is what the browser is doing, which the menu's Downloads
// wears the same ring for (docs/DECISIONS/0021-menu-sheet.md).
//
// Nothing of the list's rows is followed here: DownloadModel answers how many are
// coming, how far along they are together, and how many ended recently, and the strip
// is redrawn when those move. A tap opens the list of them.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Item {
    id: banner

    // Nothing over the page's own furniture: while the address is edited or the find
    // bar is up, the strip waits (BrowserPage.qml sets it).
    property bool suppressed: false

    // The downloads under way, how far along they are together, and the ones that ended
    // while these have been coming, as the model counts them. The one download's name,
    // and how long the batch will take, as the model can say them.
    readonly property int active: DownloadModel.runningCount
    readonly property int progress: DownloadModel.runningProgress
    readonly property int finished: DownloadModel.finishedCount
    readonly property int failed: DownloadModel.failedCount
    readonly property string oneName: DownloadModel.runningName
    readonly property int eta: DownloadModel.etaSeconds

    function timeLeft(seconds) {
        var s = Math.max(0, seconds)
        if (s < 60) {
            return qsTr("Less than a minute left")
        }
        if (s < 3600) {
            return qsTr("%1 minute(s) left").arg(Math.ceil(s / 60))
        }
        return qsTr("%1 hour(s) left").arg(Math.ceil(s / 3600))
    }

    // A tap on it: the list of the downloads, where each is named and opened.
    signal clicked()

    objectName: "downloadBanner"
    height: Theme.itemSizeSmall
    visible: active > 0 && !suppressed
    onClicked: pageStack.push(Qt.resolvedUrl("../pages/DownloadsPage.qml"))

    Rectangle {
        anchors.fill: parent
        color: Theme.highlightDimmerColor
    }

    MouseArea {
        objectName: "downloadBannerArea"
        anchors.fill: parent
        onClicked: banner.clicked()
    }

    // The ring and the count of what is coming, at the left; what has come in, at the
    // right; and between them, on the room the two leave, the one download's name over
    // how long it will take. The middle is anchored to that room rather than sized by
    // its words, so a name runs as long as the strip lets it.
    Row {
        id: left

        objectName: "downloadBannerLeft"
        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        spacing: Theme.paddingMedium

        // How far along the downloads under way have come together, with the figure in
        // the ring; in the error colour if any of them failed.
        Item {
            anchors.verticalCenter: parent.verticalCenter
            width: Theme.iconSizeMedium
            height: width

            ProgressCircle {
                objectName: "downloadBannerRing"
                anchors.fill: parent
                value: banner.progress / 100
                progressColor: banner.failed > 0 ? Theme.errorColor : Theme.highlightColor
                backgroundColor: Theme.rgba(Theme.primaryColor, Theme.opacityFaint)
                borderWidth: Theme.paddingSmall / 2
            }

            Label {
                objectName: "downloadBannerPercent"
                anchors.centerIn: parent
                text: qsTr("%1%").arg(banner.progress)
                font.pixelSize: Theme.fontSizeSmall
                color: banner.failed > 0 ? Theme.errorColor : Theme.primaryColor
            }
        }

        Row {
            objectName: "downloadBannerActive"
            anchors.verticalCenter: parent.verticalCenter
            spacing: Theme.paddingSmall
            visible: banner.active > 1

            Icon {
                objectName: "downloadBannerActiveIcon"
                anchors.verticalCenter: parent.verticalCenter
                source: "image://theme/icon-m-downloads"
                sourceSize: Qt.size(Theme.iconSizeSmall, Theme.iconSizeSmall)
                color: Theme.primaryColor
            }

            Label {
                objectName: "downloadBannerActiveCount"
                anchors.verticalCenter: parent.verticalCenter
                text: banner.active
                color: Theme.primaryColor
            }
        }
    }

    // How many have ended, recently, while these have been coming: a check, or a
    // warning when one of them failed.
    Row {
        id: right

        objectName: "downloadBannerFinished"
        anchors {
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        spacing: Theme.paddingSmall
        visible: banner.finished > 0

        Icon {
            objectName: "downloadBannerFinishedIcon"
            anchors.verticalCenter: parent.verticalCenter
            source: banner.failed > 0 ? "image://theme/icon-s-filled-warning"
                                      : "image://theme/icon-m-enter-accept"
            sourceSize: Qt.size(Theme.iconSizeSmall, Theme.iconSizeSmall)
            color: banner.failed > 0 ? Theme.errorColor : Theme.highlightColor
        }

        Label {
            objectName: "downloadBannerFinishedCount"
            anchors.verticalCenter: parent.verticalCenter
            text: banner.finished
            color: banner.failed > 0 ? Theme.errorColor : Theme.secondaryColor
        }
    }

    Column {
        objectName: "downloadBannerWords"
        anchors {
            left: left.right
            right: right.visible ? right.left : parent.right
            leftMargin: Theme.paddingMedium
            rightMargin: Theme.paddingMedium
            verticalCenter: parent.verticalCenter
        }
        spacing: Theme.paddingSmall / 2

        // Which one it is, when there is one; faded once it is too long for the strip.
        Label {
            objectName: "downloadBannerName"
            width: parent.width
            text: banner.oneName
            visible: banner.oneName.length > 0
            truncationMode: TruncationMode.Fade
            color: Theme.primaryColor
        }

        // How long it will take, at the pace it has been going; nothing until it has
        // moved.
        Label {
            objectName: "downloadBannerEta"
            width: parent.width
            text: banner.timeLeft(banner.eta)
            visible: banner.eta >= 0
            truncationMode: TruncationMode.Fade
            font.pixelSize: Theme.fontSizeExtraSmall
            color: banner.failed > 0 ? Theme.errorColor : Theme.secondaryColor
        }
    }
}
