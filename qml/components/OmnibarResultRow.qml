// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// One thing the omnibar found -- an open tab, a bookmark, a page of the history, a
// download -- as one row whatever its kind, so that the list reads as one: the site's
// icon, the title, and under it where a tap leads, no more
// (docs/DECISIONS/0027-omnibar.md). A page shows its site's icon wherever one is known
// (Omnibar's favicon), and without one a tile with the site's initial, as Firefox for
// Android draws a site it has no icon for; a file shows the downloads' glyph. The words
// typed are in bold in the title and the host (Omnibar's markedTitle, markedHost), as
// Firefox's address bar makes them stand out. The line under the title is quieter than
// the title, but for an open tab's "Switch to tab", which is in the ambience's colour:
// a tap on it brings the tab to the front rather than loading the page again.
//
// Titles and addresses are what pages and files chose to be called: the marked ones are
// escaped by the model, and everything else here is plain text.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

ListItem {
    id: row

    readonly property bool isFile: model.kind === "download"
    readonly property bool switches: model.kind === "tab"
    readonly property bool showsFavicon: !isFile && model.favicon.length > 0
                                         && siteIcon.status !== Image.Error
    // The site's initial, for the tile in place of an icon.
    readonly property string initial: (model.host.length > 0 ? model.host : model.title)
                                      .charAt(0).toUpperCase()
    // What a download not yet there is doing, as the list of downloads says it.
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
    // The second line: where a tap leads. A tab in another group than the grid's says
    // which, by the name the strip gives the group; a download, the site it came from
    // and how it is going; any other page, its host. As StyledText but for a tab's, whose
    // group has the name someone gave it.
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

    // A slot as wide as a glyph, so every title starts at the same place whichever
    // picture its row has.
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
            // Quieter than the secondary colour alone, but for "Switch to tab".
            opacity: row.switches ? 1.0 : Theme.opacityOverlay
            color: row.switches ? Theme.highlightColor
                                : row.highlighted ? Theme.secondaryHighlightColor
                                                  : Theme.secondaryColor
        }
    }

    // How far along a download still coming is, at a glance, as its row in the list of
    // downloads says it.
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
