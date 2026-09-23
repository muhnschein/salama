// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The foot of the tab grid: the groups in a row, each the width of its name, the
// current one underlined and kept in the middle, with the way to edit the groups in
// the left corner (docs/DECISIONS/0015-tab-groups.md).
//
// The geometry is Silica's own TabBar's, rebuilt from public API the way vuo rebuilds
// it (ScopeTabBar.qml): a label with Theme.paddingLarge either side, an underline of
// Theme._lineWidth exactly as wide as the current label, the current label in the
// highlight colour, and the first and last tab taking the slack so a row that fits is
// centred. TabBar itself lives in Sailfish.Silica.private and works only inside a
// TabView, neither of which a Harbour application may have. Small type, because the
// strip sits over the grid rather than at the head of a page.
//
// The names are also where a tab changes group. A preview carried down over one of
// them lights it, and dropped there the tab moves into that group; the grid's cells
// ask the strip through carryOver(), dropTab() and endCarry().
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Item {
    id: strip

    signal editRequested()

    // The button of the current group, once the Repeater has made it. row.children
    // is in the expression so it re-runs as buttons come and go.
    readonly property Item currentButton: TabModel.currentGroupIndex >= 0
                                          && TabModel.currentGroupIndex < buttons.count
                                          ? (row.children, buttons.itemAt(TabModel.currentGroupIndex))
                                          : null
    // The group a carried tab is over, which is lit and is where the tab goes if the
    // finger lifts now; -1 for none. Never the current group: the tab is in it already.
    property int dropIndex: -1

    objectName: "tabGroupStrip"

    // A tap on a group: what the buttons call, and what the load tests call.
    function select(index) {
        TabGroups.activate(index)
    }

    // The group whose name is under this point of the strip, or -1. Only where the
    // names are shown: past either end of the flickable they are scrolled out of
    // sight, and a name that cannot be seen is not a place to put anything.
    function groupAt(x, y) {
        var inFlick = mapToItem(flick, x, y)
        if (inFlick.x < 0 || inFlick.x > flick.width || inFlick.y < 0 || inFlick.y > flick.height) {
            return -1
        }
        var inRow = mapToItem(row, x, y)
        for (var i = 0; i < buttons.count; ++i) {
            var button = buttons.itemAt(i)
            if (button && inRow.x >= button.x && inRow.x < button.x + button.width) {
                return i
            }
        }
        return -1
    }

    // A cell being carried has the finger at this point of an item. Over the strip,
    // the group under it is lit and the answer is true: the finger is choosing a
    // group now, not a place, and the cells hidden under the strip are not traded
    // with. Anywhere else nothing is lit.
    function carryOver(item, x, y) {
        var at = mapFromItem(item, x, y)
        if (at.x < 0 || at.x > width || at.y < 0 || at.y > height) {
            dropIndex = -1
            return false
        }
        var index = groupAt(at.x, at.y)
        dropIndex = index !== TabModel.currentGroupIndex ? index : -1
        return true
    }

    // The finger lifts off the carried tab: into the lit group, if one is. True if it
    // goes.
    function dropTab(tabId) {
        var index = dropIndex
        dropIndex = -1
        if (index < 0) {
            return false
        }
        dropMove.tabId = tabId
        dropMove.groupId = TabGroups.groupIdAt(index)
        dropMove.restart()
        return true
    }

    // The carry is over, however it ended.
    function endCarry() {
        dropIndex = -1
    }

    // The move itself waits for the finger's release to be over. Made at once, it
    // takes the carried cell out of the grid -- the tab is not in this group any more
    // -- while the cell's own handler is still running, and the rest of the handler
    // finds its context cleared under it.
    Timer {
        id: dropMove

        property int tabId: 0
        property int groupId: 0

        objectName: "groupDropMove"
        interval: 0
        onTriggered: TabGroups.moveTab(tabId, groupId)
    }

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

    Flickable {
        id: flick

        objectName: "tabGroupList"
        // As far from the right edge as the edit button takes from the left, so a row
        // of names that fits sits in the middle of the screen rather than in the
        // middle of what the button leaves.
        anchors {
            left: editButton.right
            right: parent.right
            top: parent.top
            bottom: parent.bottom
            leftMargin: Theme.paddingMedium
            rightMargin: editButton.x + editButton.width + Theme.paddingMedium
        }
        clip: true
        contentWidth: row.width
        contentHeight: height
        boundsBehavior: Flickable.StopAtBounds
        flickableDirection: Flickable.HorizontalFlick
        // Nothing to flick when the names fit; a stray sideways drag then goes to
        // nothing rather than to the grid underneath.
        interactive: contentWidth > width
        // The current group in the middle, as far as the ends allow.
        contentX: {
            var button = strip.currentButton
            if (!button) {
                return 0
            }
            return Math.max(0, Math.min(contentWidth - width,
                                        button.x + (button.width - width) / 2))
        }

        Behavior on contentX {
            enabled: !flick.moving

            NumberAnimation {
                duration: 200
                easing.type: Easing.InOutQuad
            }
        }

        Row {
            id: row

            // Half the slack, folded into the first and last button, so a row that
            // fits the strip is centred in it.
            readonly property real extraMargin: {
                var total = 0
                for (var i = 0; i < row.children.length; ++i) {
                    total += row.children[i].contentWidth || 0
                }
                return Math.max(0, flick.width - total) / 2
            }

            height: flick.height

            Repeater {
                id: buttons

                model: TabGroups

                Item {
                    id: button

                    readonly property bool current: model.currentGroup
                    // A carried tab is over this name, and would go into its group.
                    readonly property bool target: index === strip.dropIndex
                    // Width without the slack, which the row sums to work the slack out.
                    readonly property real contentWidth: label.implicitWidth + 2 * Theme.paddingLarge

                    objectName: "tabGroupItem"
                    width: contentWidth + (index === 0 ? row.extraMargin : 0)
                           + (index === buttons.count - 1 ? row.extraMargin : 0)
                    height: row.height

                    // What marks the group a carried tab would go into: the wash the
                    // grid marks its cells with, round the name.
                    Rectangle {
                        objectName: "tabGroupDropHighlight"
                        anchors.centerIn: label
                        width: label.width + 2 * Theme.paddingMedium
                        height: label.height + 2 * Theme.paddingSmall
                        radius: Theme.paddingMedium
                        color: Theme.rgba(Theme.highlightBackgroundColor,
                                          Theme.highlightBackgroundOpacity)
                        visible: button.target
                    }

                    Label {
                        id: label

                        objectName: "tabGroupLabel"
                        // The outer buttons hug the inside edge, so their slack stays
                        // outside the name and the underline is the name's width.
                        x: {
                            if (buttons.count > 1 && index === 0) {
                                return button.width - width - Theme.paddingLarge
                            }
                            if (buttons.count > 1 && index === buttons.count - 1) {
                                return Theme.paddingLarge
                            }
                            return (button.width - width) / 2
                        }
                        y: (button.height - height) / 2
                        // An unnamed group is named by what it holds, as Safari names
                        // the tabs outside every group.
                        text: model.name.length > 0 ? model.name
                                                    : qsTr("%n tab(s)", "", model.tabCount)
                        font.pixelSize: Theme.fontSizeSmall
                        color: button.current || button.target || tap.pressed
                               ? Theme.highlightColor : Theme.primaryColor
                    }

                    // What marks the current group: the rule under its name.
                    Rectangle {
                        objectName: "tabGroupUnderline"
                        x: label.x
                        y: label.y + label.height + Theme.paddingSmall
                        width: label.width
                        height: Theme._lineWidth
                        color: Theme.highlightColor
                        visible: button.current
                    }

                    MouseArea {
                        id: tap

                        anchors.fill: parent
                        onClicked: strip.select(index)
                    }
                }
            }
        }
    }
}
