// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Marked strings escaped by model; everything else plain text.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

ListItem {
    id: row

    readonly property bool isFile: model.kind === "download"
    readonly property bool switches: model.kind === "tab"
    readonly property bool showsFavicon: !isFile && model.favicon.length > 0
                                         && siteIcon.status !== Image.Error
    readonly property string initial: (model.host.length > 0 ? model.host : model.title)
                                      .charAt(0).toUpperCase()
    readonly property string downloadState: {
        if (model.kind !== "download") {
            return ""
        }
        if (model.downloadStatus === DownloadModel.Running) {
            return qsTr("Downloading, %1%").arg(model.progress)
        }
        if (model.downloadStatus === DownloadModel.Failed) {
            return qsTr("Failed")
        }
        return model.downloadStatus === DownloadModel.Canceled ? qsTr("Cancelled") : ""
    }
    // Plain text for tab rows: group name is user text.
    readonly property string detail: {
        if (switches) {
            if (model.groupId === TabModel.currentGroupId) {
                return qsTr("Switch to tab")
            }
            //: An open tab the address bar found, in another group than the one shown:
            //: %1 is the group's name, or how many tabs it has when it has none
            return qsTr("Switch to tab in %1").arg(model.groupName.length > 0
                                                   ? model.groupName
                                                   : qsTr("%n tab(s)", "", model.groupTabCount))
        }
        if (isFile && downloadState.length > 0) {
            //: Under a download the address bar found: its site, and how it is going
            return qsTr("%1 · %2").arg(model.markedHost).arg(downloadState)
        }
        return model.markedHost
    }

    objectName: "omnibarResult"
    width: ListView.view.width
    contentHeight: Theme.itemSizeMedium

    Item {
        id: pictureSlot

        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeMedium
        height: width

        Image {
            id: siteIcon

            objectName: "omnibarResultFavicon"
            anchors.centerIn: parent
            width: Theme.iconSizeSmall
            height: width
            fillMode: Image.PreserveAspectFit
            visible: row.showsFavicon
            source: row.isFile ? "" : model.favicon
        }

        Rectangle {
            objectName: "omnibarResultLetter"
            anchors.centerIn: parent
            width: Theme.iconSizeSmall
            height: width
            radius: Theme.paddingSmall
            visible: !row.isFile && !row.showsFavicon
            color: Theme.rgba(row.highlighted ? Theme.highlightColor : Theme.primaryColor,
                              Theme.opacityFaint)

            Label {
                anchors.centerIn: parent
                text: row.initial
                textFormat: Text.PlainText
                font.pixelSize: Theme.fontSizeExtraSmall
                color: row.highlighted ? Theme.highlightColor : Theme.secondaryColor
            }
        }

        Icon {
            objectName: "omnibarResultGlyph"
            anchors.fill: parent
            visible: row.isFile
            source: "image://theme/icon-m-downloads"
            highlighted: row.highlighted
        }
    }

    Column {
        anchors {
            left: pictureSlot.right
            leftMargin: Theme.paddingMedium
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }

        Label {
            objectName: "omnibarResultTitle"
            width: parent.width
            text: model.markedTitle
            textFormat: Text.StyledText
            truncationMode: TruncationMode.Fade
            color: row.highlighted ? Theme.highlightColor : Theme.primaryColor
        }

        Label {
            objectName: "omnibarResultDetail"
            width: parent.width
            text: row.detail
            textFormat: row.switches ? Text.PlainText : Text.StyledText
            truncationMode: TruncationMode.Fade
            font.pixelSize: Theme.fontSizeExtraSmall
            opacity: row.switches ? 1.0 : Theme.opacityOverlay
            color: row.switches ? Theme.highlightColor
                                : row.highlighted ? Theme.secondaryHighlightColor
                                                  : Theme.secondaryColor
        }
    }

    Rectangle {
        objectName: "omnibarResultProgress"
        anchors {
            left: parent.left
            bottom: parent.bottom
        }
        height: Theme.paddingSmall
        width: parent.width * model.progress / 100
        color: Theme.highlightColor
        visible: model.kind === "download" && model.downloadStatus === DownloadModel.Running
    }
}
