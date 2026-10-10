// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

Page {
    id: groupsPage

    objectName: "tabGroupsPage"
    allowedOrientations: Orientation.Portrait | Orientation.LandscapeMask

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

        // Row under dragged one animates aside rather than jumps.
        moveDisplaced: Transition {
            NumberAnimation {
                properties: "y"
                duration: 150
                easing.type: Easing.OutQuad
            }
        }

        // Add row under last (where reader already looks), not pulley. Icon has own ring.
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
