// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 tuuli contributors
//
// The tab groups as a list, for editing them: new, rename, delete, and a tap to make
// one current. The same page picks a group for a tab to move to when it is given the
// tab (docs/DECISIONS/0015-tab-groups.md).
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.tuuli 1.0
import "../components"

Page {
    id: groupsPage

    // A tab to move into the group that is tapped, or 0 to edit the groups instead.
    property int moveTabId: 0

    objectName: "tabGroupsPage"
    allowedOrientations: Orientation.Portrait

    function choose(index, groupId) {
        if (moveTabId > 0) {
            TabGroups.moveTab(moveTabId, groupId)
        } else {
            TabGroups.activate(index)
        }
        pageStack.pop()
    }

    SilicaListView {
        id: groupList

        objectName: "tabGroupsList"
        anchors.fill: parent
        model: TabGroups
        header: PageHeader {
            title: groupsPage.moveTabId > 0 ? qsTr("Move to tab group") : qsTr("Tab groups")
        }

        PullDownMenu {
            MenuItem {
                objectName: "newGroupMenu"
                text: qsTr("New tab group")
                // Given the tab, the dialog moves it into the group it creates.
                onClicked: pageStack.push(Qt.resolvedUrl("TabGroupDialog.qml"), {
                                              "moveTabId": groupsPage.moveTabId
                                          })
            }
        }

        delegate: TabGroupDelegate {
            onClicked: groupsPage.choose(index, model.groupId)
            onRenameRequested: pageStack.push(Qt.resolvedUrl("TabGroupDialog.qml"), {
                                                  "groupId": model.groupId,
                                                  "name": model.name
                                              })
        }

        VerticalScrollDecorator {}
    }
}
