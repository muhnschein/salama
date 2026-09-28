// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The cover while something downloads: Silica's ring, filled as far as the downloads
// have come together, the percentage in it and how many files are coming under that
// (docs/DECISIONS/0037-cover-is-where-you-were.md).
//
// In steps of five: the ring and the number move twenty times in a download at most,
// however often the engine reports, so the cover is drawn again no more often than that.
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: downloads

    /// How many downloads are coming, and how far along they are together, 0 to 100
    /// (DownloadModel.runningCount and runningProgress).
    property int count
    property int progress

    /// The progress as the cover shows it, down to the step of five below.
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

    // As large as the room under the heading allows, and centred in it.
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
