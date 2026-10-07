// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Dialog {
    id: dialog

    // 0 = new group.
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
            objectName: "tabGroupDialogHeader"
            title: dialog.groupId > 0 ? qsTr("Rename tab group") : qsTr("New tab group")
            //: Accepts the dialog that makes a new tab group
            acceptText: dialog.groupId === 0 ? qsTr("Create") : qsTr("Save")
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
