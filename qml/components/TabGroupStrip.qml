// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 tuuli contributors
//
// The head of the tab grid: the groups in a row, the current one in the middle and
// its neighbours to either side, flicked through sideways the way Safari's are. The
// two corners hold the way to edit the groups and the way to search every tab
// (docs/DECISIONS/0015-tab-groups.md).
//
// The list and the model each drive the other: a flick makes the group under the
// middle the current one, and a tab brought to the front from elsewhere -- a search
// result, a tab moved to another group -- scrolls the list to its group. The list's
// own currentIndex is the meeting point, which is why it is set from a Connections
// rather than bound: the view writes to it too, and a binding would not survive that.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.tuuli 1.0

Item {
    id: strip

    signal editRequested()
    signal searchRequested()

    objectName: "tabGroupStrip"

    IconButton {
        id: editButton

        objectName: "editGroupsButton"
        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeMedium
        height: width
        icon.source: "image://theme/icon-m-edit"
        onClicked: strip.editRequested()
    }

    IconButton {
        id: searchButton

        objectName: "searchTabsButton"
        anchors {
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeMedium
        height: width
        icon.source: "image://theme/icon-m-search"
        onClicked: strip.searchRequested()
    }

    ListView {
        id: groupList

        // Half the room: the current group's name in the middle, and half of each
        // neighbour's showing, which is what says there is somewhere to flick to.
        readonly property real itemWidth: Math.floor(width / 2)

        objectName: "tabGroupList"
        anchors {
            left: editButton.right
            right: searchButton.left
            top: parent.top
            bottom: parent.bottom
            leftMargin: Theme.paddingMedium
            rightMargin: Theme.paddingMedium
        }
        clip: true
        orientation: ListView.Horizontal
        model: TabGroups
        currentIndex: TabModel.currentGroupIndex
        // The current item is held in the middle, and a flick lands on a whole item:
        // the position of the list is the choice of group, never something between.
        snapMode: ListView.SnapOneItem
        highlightRangeMode: ListView.StrictlyEnforceRange
        preferredHighlightBegin: (width - itemWidth) / 2
        preferredHighlightEnd: preferredHighlightBegin + itemWidth
        highlightMoveDuration: 200
        onCurrentIndexChanged: TabGroups.activate(currentIndex)

        delegate: Item {
            objectName: "tabGroupItem"
            width: groupList.itemWidth
            height: groupList.height

            Label {
                objectName: "tabGroupLabel"
                anchors.fill: parent
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                // An unnamed group is named by what it holds, as Safari names the
                // tabs outside every group.
                text: model.name.length > 0 ? model.name : qsTr("%n tab(s)", "", model.tabCount)
                truncationMode: TruncationMode.Fade
                font.pixelSize: Theme.fontSizeSmall
                color: model.currentGroup ? Theme.highlightColor : Theme.secondaryColor
            }

            MouseArea {
                anchors.fill: parent
                onClicked: groupList.currentIndex = index
            }
        }
    }

    Connections {
        target: TabModel
        onCurrentGroupChanged: groupList.currentIndex = TabModel.currentGroupIndex
    }
}
