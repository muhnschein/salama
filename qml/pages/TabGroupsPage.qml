// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The tab groups as a list, for editing them: each a picture of its tabs beside its name,
// a tap to make one current, a grip to carry one to another place, rename, ungroup and
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

        // A row carried by its grip trades places with the one it is carried over, and
        // that one makes way for it rather than jump.
        moveDisplaced: Transition {
            NumberAnimation {
                properties: "y"
                duration: 150
                easing.type: Easing.OutQuad
            }
        }

        // The way to another group, where the next one would be listed: a row shaped
        // like a group's, under the last row rather than in a pulley, which is where a
        // reader who has just read the list is already looking. Where a group has its
        // picture, the theme's ringed plus on nothing -- the icon carries its own ring --
        // as Piirit's rows that add something wear it.
        footer: ListItem {
            id: newGroupRow

            objectName: "newGroupButton"
            width: groupList.width
            contentHeight: Theme.itemSizeExtraLarge
            onClicked: pageStack.push(Qt.resolvedUrl("TabGroupDialog.qml"))

            Item {
                id: plusPlace

                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                // A group's picture's size (TabGroupCollage).
                width: Theme.iconSizeLarge
                height: width

                Icon {
                    objectName: "newGroupPlus"
                    anchors.centerIn: parent
                    source: "image://theme/icon-m-add"
                    highlighted: newGroupRow.highlighted
                }
            }

            Label {
                anchors {
                    left: plusPlace.right
                    right: parent.right
                    leftMargin: Theme.paddingLarge
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                text: qsTr("New tab group")
                truncationMode: TruncationMode.Fade
                color: newGroupRow.highlighted ? Theme.highlightColor : Theme.primaryColor
            }
        }

        delegate: TabGroupDelegate {
            width: groupList.width
            onClicked: groupsPage.choose(index)
            onRenameRequested: pageStack.push(Qt.resolvedUrl("TabGroupDialog.qml"), {
                                                  "groupId": model.groupId,
                                                  "name": model.name
                                              })
        }

        VerticalScrollDecorator {}
    }
}
