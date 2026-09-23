// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The tab groups as a list, for editing them: a tap to make one current, rename and
// delete in each row's menu, and a row under the last group that makes a new one
// (docs/DECISIONS/0015-tab-groups.md).
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

Page {
    id: groupsPage

    objectName: "tabGroupsPage"
    allowedOrientations: Orientation.Portrait

    function choose(index) {
        TabGroups.activate(index)
        pageStack.pop()
    }

    SilicaListView {
        id: groupList

        objectName: "tabGroupsList"
        anchors.fill: parent
        model: TabGroups
        header: PageHeader {
            title: qsTr("Tab groups")
        }

        // The way to another group, where the next one would be listed: a row
        // shaped like a group's with a plus where its name would start, under the
        // last row rather than in a pulley, which is where a reader who has just
        // read the list is already looking.
        footer: ListItem {
            id: newGroupRow

            objectName: "newGroupButton"
            width: groupList.width
            contentHeight: Theme.itemSizeMedium
            onClicked: pageStack.push(Qt.resolvedUrl("TabGroupDialog.qml"))

            Icon {
                id: plus

                objectName: "newGroupPlus"
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                source: "image://theme/icon-m-add"
                highlighted: newGroupRow.highlighted
            }

            Label {
                anchors {
                    left: plus.right
                    right: parent.right
                    leftMargin: Theme.paddingMedium
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                text: qsTr("New tab group")
                truncationMode: TruncationMode.Fade
                color: newGroupRow.highlighted ? Theme.highlightColor : Theme.primaryColor
            }
        }

        delegate: TabGroupDelegate {
            onClicked: groupsPage.choose(index)
            onRenameRequested: pageStack.push(Qt.resolvedUrl("TabGroupDialog.qml"), {
                                                  "groupId": model.groupId,
                                                  "name": model.name
                                              })
        }

        VerticalScrollDecorator {}
    }
}
