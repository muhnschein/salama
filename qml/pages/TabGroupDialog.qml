// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// A tab group's name: for a new group, or for one being renamed.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Dialog {
    id: dialog

    // The group being renamed, or 0 for a new one.
    property int groupId: 0
    property alias name: nameField.text

    objectName: "tabGroupDialog"
    allowedOrientations: Orientation.Portrait
    onAccepted: {
        if (groupId > 0) {
            TabGroups.renameGroup(groupId, nameField.text)
        } else {
            TabGroups.addGroup(nameField.text)
        }
    }

    Column {
        width: parent.width

        DialogHeader {
            title: dialog.groupId > 0 ? qsTr("Rename tab group") : qsTr("New tab group")
            acceptText: qsTr("Save")
        }

        TextField {
            id: nameField

            objectName: "groupNameField"
            width: parent.width
            label: qsTr("Name")
            placeholderText: label
            EnterKey.iconSource: "image://theme/icon-m-enter-accept"
            EnterKey.onClicked: dialog.accept()
        }
    }
}
