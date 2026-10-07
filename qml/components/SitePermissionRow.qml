// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// No choices -> tap navigates instead of opening menu.
import QtQuick 2.6
import Sailfish.Silica 1.0

ListItem {
    id: row

    property string iconSource
    property string text
    property string value
    property string description
    property var choices: []

    signal chosen(int index)
    signal opened()

    contentHeight: Math.max(Theme.itemSizeMedium, texts.height + 2 * Theme.paddingSmall)
    menu: ContextMenu {
        Repeater {
            model: row.choices

            MenuItem {
                objectName: "sitePermissionChoice"
                text: modelData
                onClicked: row.chosen(index)
            }
        }

        MenuItem {
            objectName: "sitePermissionShowExceptions"
            text: qsTr("Show exceptions")
            onClicked: row.opened()
        }
    }
    onClicked: {
        if (choices.length > 0) {
            openMenu()
        } else {
            opened()
        }
    }

    Icon {
        id: icon

        objectName: "sitePermissionIcon"
        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeMedium
        height: width
        source: row.iconSource
        highlighted: row.highlighted
    }

    Column {
        id: texts

        anchors {
            left: icon.right
            right: parent.right
            leftMargin: Theme.paddingMedium
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }

        Row {
            width: parent.width
            spacing: Theme.paddingMedium

            Label {
                id: name

                objectName: "sitePermissionName"
                text: row.text
                truncationMode: TruncationMode.Fade
                color: row.highlighted ? Theme.highlightColor : Theme.primaryColor
            }

            Label {
                objectName: "sitePermissionValue"
                width: Math.max(0, parent.width - name.width - parent.spacing)
                text: row.value
                textFormat: Text.PlainText
                truncationMode: TruncationMode.Fade
                color: Theme.highlightColor
            }
        }

        Label {
            objectName: "sitePermissionDescription"
            width: parent.width
            visible: text.length > 0
            text: row.description
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeExtraSmall
            color: row.highlighted ? Theme.highlightColor : Theme.secondaryHighlightColor
        }
    }
}
