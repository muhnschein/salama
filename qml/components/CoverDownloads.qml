// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Steps of 5: cover redraws at most 20 times per download.
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: downloads

    /// DownloadModel.runningCount and runningProgress (0-100).
    property int count
    property int progress

    readonly property int shown: Math.floor(Math.max(0, Math.min(progress, 100)) / 5) * 5

    objectName: "coverDownloads"

    Label {
        id: heading

        objectName: "coverDownloadsHeading"
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        //: On the cover, over the ring that shows how far the downloads have come
        text: qsTr("Downloading")
        truncationMode: TruncationMode.Fade
        font.pixelSize: Theme.fontSizeExtraSmall
        color: Theme.highlightColor
    }

    Item {
        id: room

        anchors {
            top: heading.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            topMargin: Theme.paddingMedium
        }

        ProgressCircle {
            id: ring

            objectName: "coverDownloadsRing"
            anchors.centerIn: parent
            width: Math.min(room.width, room.height)
            height: width
            value: downloads.shown / 100
            progressColor: Theme.highlightColor
            backgroundColor: Theme.rgba(Theme.highlightColor, Theme.opacityFaint)
            borderWidth: Theme.paddingSmall
        }

        Column {
            anchors.centerIn: ring

            Row {
                anchors.horizontalCenter: parent.horizontalCenter

                Label {
                    id: percent

                    objectName: "coverDownloadsPercent"
                    text: downloads.shown
                    font.pixelSize: Theme.fontSizeHuge
                    color: Theme.primaryColor
                }

                Label {
                    anchors.baseline: percent.baseline
                    //: The unit after the downloads' progress on the cover, set smaller than the number: "64%"
                    text: qsTr("%")
                    font.pixelSize: Theme.fontSizeMedium
                    color: Theme.secondaryColor
                }
            }

            Label {
                objectName: "coverDownloadsCount"
                anchors.horizontalCenter: parent.horizontalCenter
                //: How many downloads are coming, on the cover, under their progress
                text: qsTr("%n file(s)", "", downloads.count)
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
            }
        }
    }
}
