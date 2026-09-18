// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// One tab group in the list of them: its name -- or, unnamed, what it holds -- and
// how many tabs it has, with rename and delete in its menu.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

ListItem {
    id: delegate

    signal renameRequested()

    objectName: "tabGroupDelegate"
    contentHeight: Theme.itemSizeMedium
    // The default group is what it is: neither renamed nor removed.
    readonly property bool fixed: model.defaultGroup

    menu: ContextMenu {
        MenuItem {
            objectName: "renameGroupMenu"
            text: qsTr("Rename")
            enabled: !delegate.fixed
            onClicked: delegate.renameRequested()
        }
        MenuItem {
            objectName: "deleteGroupMenu"
            text: qsTr("Delete")
            enabled: !delegate.fixed
            onClicked: delegate.remorseAction(qsTr("Deleting tab group"), function () {
                TabGroups.removeGroup(model.groupId)
            })
        }
    }

    Column {
        anchors {
            left: parent.left
            right: parent.right
            margins: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }

        Label {
            objectName: "tabGroupName"
            width: parent.width
            text: model.name.length > 0 ? model.name : qsTr("%n tab(s)", "", model.tabCount)
            truncationMode: TruncationMode.Fade
            color: delegate.highlighted || model.currentGroup ? Theme.highlightColor
                                                              : Theme.primaryColor
        }

        Label {
            objectName: "tabGroupCount"
            width: parent.width
            text: qsTr("%n tab(s)", "", model.tabCount)
            // The name already says this for an unnamed group.
            visible: model.name.length > 0
            font.pixelSize: Theme.fontSizeExtraSmall
            color: delegate.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
        }
    }
}
