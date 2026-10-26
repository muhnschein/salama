// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

ListItem {
    id: delegate

    readonly property bool searching: HistoryModel.searchTerm.length > 0

    signal openInNewTabRequested()
    signal removeRequested()

    objectName: "historyDelegate"
    contentHeight: Theme.itemSizeMedium
    menu: ContextMenu {
        MenuItem {
            objectName: "openInNewTabMenu"
            text: qsTr("Open in new tab")
            onClicked: delegate.openInNewTabRequested()
        }
        MenuItem {
            objectName: "removeHistoryMenu"
            text: qsTr("Remove")
            onClicked: delegate.removeRequested()
        }
    }

    MatchMarks {
        id: marks
    }

    // As bookmark's row.
    Image {
        id: favicon

        objectName: "historyFavicon"
        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeSmall
        height: width
        fillMode: Image.PreserveAspectFit
        source: model.favicon
    }

    Column {
        anchors {
            left: favicon.right
            leftMargin: Theme.paddingMedium
            right: dateLabel.left
            rightMargin: Theme.paddingMedium
            verticalCenter: parent.verticalCenter
        }

        // Page text plain, unless marked (model escaped it).
        Label {
            objectName: "historyTitle"
            width: parent.width
            text: delegate.searching ? marks.marked(model.markedTitle) : model.title
            textFormat: delegate.searching ? Text.StyledText : Text.PlainText
            truncationMode: TruncationMode.Fade
            color: delegate.highlighted ? Theme.highlightColor : Theme.primaryColor
        }

        Label {
            objectName: "historyUrl"
            width: parent.width
            text: delegate.searching ? marks.marked(model.markedUrl) : model.url
            textFormat: delegate.searching ? Text.StyledText : Text.PlainText
            truncationMode: TruncationMode.Fade
            font.pixelSize: Theme.fontSizeExtraSmall
            color: delegate.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
        }
    }

    Label {
        id: dateLabel

        objectName: "historyDate"
        anchors {
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        text: Qt.formatDate(model.date, Qt.DefaultLocaleShortDate)
        font.pixelSize: Theme.fontSizeExtraSmall
        color: Theme.secondaryColor
    }
}
