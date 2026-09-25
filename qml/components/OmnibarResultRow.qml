// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// One thing the omnibar found -- an open tab, a bookmark, a page of the history, a
// download -- as one row whatever its kind, so that the list reads as one: a picture,
// the title, and under it where it leads, no more (docs/DECISIONS/0027-omnibar.md). An
// open tab says it is one, "Switch to tab", as Firefox's address bar says it, since a
// tap brings it to the front rather than loading the page again; every other page says
// its host. Titles and addresses are what pages and files chose to be called, so every
// line is plain text.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

ListItem {
    id: row

    // A page shows the site's own icon once there is one that loads. The rest, and
    // those without, show a glyph of the platform's: the tabs' own in sailfish-browser's
    // toolbar (apps/browser/qml/pages/components/ToolBar.qml), and the ones the menu
    // gives the bookmarks, the history and the downloads.
    readonly property bool showsFavicon: model.favicon.length > 0
                                         && siteIcon.status !== Image.Error
    readonly property string glyph: {
        if (model.kind === "tab") {
            return "image://theme/icon-m-tabs"
        }
        if (model.kind === "download") {
            return "image://theme/icon-m-downloads"
        }
        return model.bookmarked ? "image://theme/icon-m-favorite" : "image://theme/icon-m-history"
    }
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
    // and how it is going; any other page, its host.
    readonly property string detail: {
        if (model.kind === "tab") {
            if (model.groupId === TabModel.currentGroupId) {
                return qsTr("Switch to tab")
            }
            //: An open tab the address bar found, in another group than the one shown:
            //: %1 is the group's name, or how many tabs it has when it has none
            return qsTr("Switch to tab in %1").arg(model.groupName.length > 0
                                                   ? model.groupName
                                                   : qsTr("%n tab(s)", "", model.groupTabCount))
        }
        if (model.kind === "download" && downloadState.length > 0) {
            //: Under a download the address bar found: its site, and how it is going
            return qsTr("%1 · %2").arg(model.host).arg(downloadState)
        }
        return model.host
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
            source: model.favicon
        }

        Icon {
            objectName: "omnibarResultGlyph"
            anchors.fill: parent
            visible: !row.showsFavicon
            source: row.glyph
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
            text: model.title
            textFormat: Text.PlainText
            truncationMode: TruncationMode.Fade
            color: row.highlighted ? Theme.highlightColor : Theme.primaryColor
        }

        Label {
            objectName: "omnibarResultDetail"
            width: parent.width
            text: row.detail
            textFormat: Text.PlainText
            truncationMode: TruncationMode.Fade
            font.pixelSize: Theme.fontSizeExtraSmall
            color: row.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
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
