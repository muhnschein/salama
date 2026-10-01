// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// A kind of site permission on Settings > Site permissions: its icon and its name, beside
// the name what it is set to for the sites that have no exception, and under them how many
// there are (docs/DECISIONS/0039-site-permissions.md). The shape is SettingsEntry's, a
// medium item tall with the secondary highlight colour for the line under the name; but
// a tap opens a context menu, as Silica's list items do, with the choices of the default
// and under them the way to the exceptions. A row with no choices of its own -- the
// notifications', whose page is a page of its own, and the sites with tracking protection
// off -- goes where it goes on the tap.
import QtQuick 2.6
import Sailfish.Silica 1.0

ListItem {
    id: row

    property string iconSource
    property string text
    // What it is set to, said after the name.
    property string value
    // The line under them.
    property string description
    // What a tap offers to set it to, each as it is named; none for a row that is only a way
    // on.
    property var choices: []

    // Index into the choices of what was picked.
    signal chosen(int index)
    // A tap on a row with no choices, and the menu's last item.
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
