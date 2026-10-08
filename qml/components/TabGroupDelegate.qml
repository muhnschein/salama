// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

ListItem {
    id: delegate

    signal renameRequested()

    readonly property bool fixed: model.defaultGroup
    // Ys in list coords (don't move with row); target row counted from startRow.
    property bool carried: false
    property int startRow: -1
    property int carriedRow: -1
    property real startY: 0
    property real grabY: 0
    property real fingerY: 0
    // Attached only for non-default groups, so hold on default opens nothing.
    property Item groupMenu: ContextMenu {
        MenuItem {
            objectName: "renameGroupMenu"
            text: qsTr("Rename")
            onClicked: delegate.renameRequested()
        }
        MenuItem {
            objectName: "ungroupMenu"
            //: Removes the tab group and keeps its tabs open, in the first group
            text: qsTr("Ungroup")
            onClicked: TabGroups.ungroup(model.groupId)
        }
        MenuItem {
            objectName: "deleteGroupMenu"
            text: qsTr("Delete")
            onClicked: delegate.remorseAction(qsTr("Deleting tab group"), function () {
                TabGroups.removeGroup(model.groupId)
            })
        }
    }

    objectName: "tabGroupDelegate"
    contentHeight: Theme.itemSizeExtraLarge
    menu: fixed ? null : groupMenu
    z: carried ? 1 : 0
    // List relaid carried row; keep content under finger.
    onYChanged: {
        if (carried) {
            follow()
        }
    }

    function pickUp(y) {
        startRow = index
        carriedRow = index
        startY = delegate.y
        grabY = y
        fingerY = y
        carried = true
    }

    function follow() {
        body.y = fingerY - grabY - (delegate.y - startY)
    }

    // Counted from start, not asked of list: list relays out next frame, so second move
    // before then would use stale rows.
    function carryTo(y) {
        if (!carried) {
            return
        }
        fingerY = y
        follow()
        var target = startRow + Math.round((y - grabY) / delegate.contentHeight)
        target = Math.max(1, Math.min(TabGroups.count - 1, target))
        if (target !== carriedRow && TabGroups.moveGroup(carriedRow, target)) {
            carriedRow = target
        }
    }

    // Order matters: spring enabled before y moves.
    function drop() {
        carried = false
        body.y = 0
    }

    Item {
        id: body

        width: parent.width
        height: delegate.contentHeight

        Behavior on y {
            enabled: !delegate.carried

            NumberAnimation {
                duration: 150
                easing.type: Easing.OutQuad
            }
        }

        Rectangle {
            objectName: "tabGroupCarryWash"
            anchors.fill: parent
            color: Theme.rgba(Theme.highlightBackgroundColor, Theme.highlightBackgroundOpacity)
            visible: delegate.carried
        }

        TabGroupCollage {
            id: collage

            objectName: "tabGroupCollage"
            anchors {
                left: parent.left
                leftMargin: Theme.horizontalPageMargin
                verticalCenter: parent.verticalCenter
            }
            previews: model.previews
            current: model.currentGroup
        }

        Column {
            anchors {
                left: collage.right
                right: grip.left
                leftMargin: Theme.paddingLarge
                verticalCenter: parent.verticalCenter
            }

            Label {
                objectName: "tabGroupName"
                width: parent.width
                text: model.name.length > 0 ? model.name : qsTr("%n tab(s)", "", model.tabCount)
                truncationMode: TruncationMode.Fade
                color: delegate.highlighted || delegate.carried || model.currentGroup
                       ? Theme.highlightColor : Theme.primaryColor
            }

            Label {
                objectName: "tabGroupCount"
                width: parent.width
                text: qsTr("%n tab(s)", "", model.tabCount)
                visible: model.name.length > 0
                font.pixelSize: Theme.fontSizeExtraSmall
                color: delegate.highlighted || delegate.carried || model.currentGroup
                       ? Theme.secondaryHighlightColor : Theme.secondaryColor
            }
        }

        // Carries at once, no hold; preventStealing keeps vertical drag from list scroll.
        MouseArea {
            id: grip

            objectName: "tabGroupGrip"
            anchors {
                right: parent.right
                top: parent.top
                bottom: parent.bottom
            }
            width: Theme.itemSizeSmall
            enabled: !delegate.fixed
            visible: enabled
            preventStealing: true
            onPressed: delegate.pickUp(mapToItem(delegate.parent, mouse.x, mouse.y).y)
            onPositionChanged: delegate.carryTo(mapToItem(delegate.parent, mouse.x, mouse.y).y)
            onReleased: delegate.drop()
            onCanceled: delegate.drop()

            Column {
                anchors.centerIn: parent
                spacing: Theme.paddingSmall

                Repeater {
                    model: 3

                    Rectangle {
                        width: Theme.iconSizeSmall - Theme.paddingMedium
                        height: Theme._lineWidth
                        radius: height / 2
                        color: grip.pressed ? Theme.highlightColor : Theme.secondaryColor
                    }
                }
            }
        }
    }
}
