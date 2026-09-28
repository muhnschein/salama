// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// One tab group in the list of them, laid out as the Gallery lists its albums: how many
// tabs it has, a square picture of the one last in front, and its name, large, with a
// grip at its end to carry it to another place in the list, and rename, ungroup and
// delete in its menu. The default group has neither grip nor menu: it is first, and none
// of the three applies to it (docs/DECISIONS/0015-tab-groups.md).
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

ListItem {
    id: delegate

    signal renameRequested()

    // The default group is what it is: neither renamed, ungrouped nor removed, and
    // first whatever else moves.
    readonly property bool fixed: model.defaultGroup
    // While the grip has a finger, the row is carried. Where the finger went down and
    // where it is now are in the list's own coordinates, which do not move with the
    // row, and the row it is carried to is counted from the one it started at.
    property bool carried: false
    property int startRow: -1
    property int carriedRow: -1
    property real startY: 0
    property real grabY: 0
    property real fingerY: 0
    // Made for every group, and handed to the list item only for one that has a menu:
    // on the default group a hold opens nothing, rather than a menu of nothing to do.
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
    // A carried row passes over the others, not under them.
    z: carried ? 1 : 0
    // The list has laid the carried row out at its new place: what it shows stays under
    // the finger.
    onYChanged: {
        if (carried) {
            follow()
        }
    }

    // The finger is down on the grip, this far down the list.
    function pickUp(y) {
        startRow = index
        carriedRow = index
        startY = delegate.y
        grabY = y
        fingerY = y
        carried = true
    }

    // What the row shows, moved from the row's place to where the finger has it.
    function follow() {
        body.y = fingerY - grabY - (delegate.y - startY)
    }

    // The finger has moved. The row trades places with the one its middle has been
    // carried into, as a cell of the grid trades with the one it is carried over (0010).
    // Counted from where it started rather than asked of the list, which lays its rows
    // out again only on its next frame: a second move before then would go by where
    // the rows were.
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

    // The finger is off the grip: the row settles into its place. In that order, so the
    // spring below is on before its property moves; switched by a binding on the same
    // change, it would not be ordered against the move
    // (docs/DECISIONS/0010-tab-grid-deck.md).
    function drop() {
        carried = false
        body.y = 0
    }

    // What the finger carries: all the row shows, lifted off its place in the list while
    // the rows it passes make way for it, and back into place once the finger lifts.
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

        // The wash of a pressed row, for as long as the row is held.
        Rectangle {
            objectName: "tabGroupCarryWash"
            anchors.fill: parent
            color: Theme.rgba(Theme.highlightBackgroundColor, Theme.highlightBackgroundOpacity)
            visible: delegate.carried
        }

        // The count, at the end of the room before the picture, as the Gallery counts
        // an album's photos.
        Label {
            objectName: "tabGroupCount"
            anchors {
                right: picture.left
                rightMargin: Theme.paddingLarge
                verticalCenter: parent.verticalCenter
            }
            text: model.tabCount
            font.pixelSize: Theme.fontSizeLarge
            color: delegate.highlighted || delegate.carried || model.currentGroup
                   ? Theme.secondaryHighlightColor : Theme.secondaryColor
        }

        TabGroupPicture {
            id: picture

            objectName: "tabGroupPicture"
            // As far in as it is wide, which leaves the count its room.
            x: width
            width: height
            height: parent.height
            previews: model.previews
        }

        // The group the grid shows is named in the highlight colour, as Silica lights
        // the item that is chosen.
        Label {
            objectName: "tabGroupName"
            anchors {
                left: picture.right
                right: grip.left
                leftMargin: Theme.paddingLarge
                verticalCenter: parent.verticalCenter
            }
            //: The name of the tab group that holds the tabs in no group of their own
            text: model.name.length > 0 ? model.name : qsTr("Tabs")
            font.pixelSize: Theme.fontSizeLarge
            truncationMode: TruncationMode.Fade
            color: delegate.highlighted || delegate.carried || model.currentGroup
                   ? Theme.highlightColor : Theme.primaryColor
        }

        // The grip: three short bars at the row's end, with room round them for a thumb.
        // A finger on it carries the row at once, with no hold first -- the grip is there
        // for nothing else -- and keeps the touch from the list, which would take a drag
        // up or down to scroll. The default group keeps the room and not the grip, so
        // every name has the same width.
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
