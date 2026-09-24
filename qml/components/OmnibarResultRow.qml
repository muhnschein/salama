// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// One thing the omnibar found -- an open tab, a bookmark, a page of the history, a
// download -- as one row whatever its kind, the way piirit draws what its search finds,
// so that the sections read as one list: a picture, the title, and under it where the
// thing is, with the date on the right for the kinds that have one
// (docs/DECISIONS/0027-omnibar.md). Titles and addresses are what pages and files chose
// to be called, so every line is plain text.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

ListItem {
    id: row

    readonly property bool dated: model.kind === "history" || model.kind === "download"
    // A tab and a bookmark show the site's own icon once there is one that loads. The
    // rest, and those without, show a glyph of the platform's: the ones the menu gives
    // history and downloads, the tabs' own in sailfish-browser's toolbar
    // (apps/browser/qml/pages/components/ToolBar.qml), and the menu's bookmark.
    readonly property bool showsFavicon: model.favicon.length > 0
                                         && siteIcon.status !== Image.Error
    readonly property string glyph: {
        if (model.kind === "tab") {
            return "image://theme/icon-m-tabs"
        }
        if (model.kind === "bookmark") {
            return "image://theme/icon-m-favorite"
        }
        return model.kind === "history" ? "image://theme/icon-m-history"
                                        : "image://theme/icon-m-downloads"
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
    // The second line: where the thing is. A tab in another group than the grid's says
    // which, by the name the strip gives the group; a download, the site it came from
    // and how it is going; the rest, the whole address.
    readonly property string detail: {
        if (model.kind === "tab") {
            if (model.groupId === TabModel.currentGroupId) {
                return model.host
            }
            return joined(model.groupName.length > 0
                          ? model.groupName : qsTr("%n tab(s)", "", model.groupTabCount),
                          model.host)
        }
        if (model.kind === "download") {
            return downloadState.length > 0 ? joined(model.host, downloadState) : model.host
        }
        return model.url
    }

    objectName: "omnibarResult"
    width: ListView.view.width
    contentHeight: Theme.itemSizeMedium

    function joined(first, second) {
        //: Two parts of a line under a suggestion in the address bar: a tab's group and
        //: its site, or a download's site and how it is going
        return qsTr("%1 · %2").arg(first).arg(second)
    }

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
            right: row.dated ? dateLabel.left : parent.right
            rightMargin: row.dated ? Theme.paddingMedium : Theme.horizontalPageMargin
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

    // As the history list draws its date: the last visit, or when a download started.
    Label {
        id: dateLabel

        objectName: "omnibarResultDate"
        anchors {
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        visible: row.dated
        text: row.dated ? Qt.formatDate(model.date, Qt.DefaultLocaleShortDate) : ""
        textFormat: Text.PlainText
        font.pixelSize: Theme.fontSizeExtraSmall
        color: row.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
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
