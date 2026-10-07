// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
import QtQuick 2.6
import Sailfish.Silica 1.0

ListItem {
    id: row

    property string title
    property string host

    signal addRequested()
    signal forgetRequested()

    objectName: "foundSearchEngine"
    width: parent.width
    contentHeight: Theme.itemSizeMedium
    menu: ContextMenu {
        MenuItem {
            objectName: "foundSearchEngineForget"
            //: Drops an offered search engine from the list, without adding it
            text: qsTr("Forget")
            onClicked: row.forgetRequested()
        }
    }
    onClicked: row.addRequested()

    Icon {
        id: icon

        objectName: "foundSearchEngineIcon"
        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        source: "image://theme/icon-m-add"
        highlighted: row.highlighted
    }

    Column {
        anchors {
            left: icon.right
            right: parent.right
            leftMargin: Theme.paddingMedium
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }

        Label {
            objectName: "foundSearchEngineName"
            width: parent.width
            text: row.title
            textFormat: Text.PlainText
            truncationMode: TruncationMode.Fade
            color: row.highlighted ? Theme.highlightColor : Theme.primaryColor
        }

        Label {
            objectName: "foundSearchEngineHost"
            width: parent.width
            //: Under an offered search engine: the site that offered it. %1 is its host
            text: qsTr("%1 · Tap to add").arg(row.host)
            textFormat: Text.PlainText
            truncationMode: TruncationMode.Fade
            font.pixelSize: Theme.fontSizeExtraSmall
            color: row.highlighted ? Theme.highlightColor : Theme.secondaryHighlightColor
        }
    }
}
