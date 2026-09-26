// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The strip of groups at the foot of the tutorial's grid: the new-tab and edit icons in
// its corners, and two names between -- the group the sketched tabs are in, underlined,
// named by how many it holds as the real strip names a group without a name, and one
// other group to carry a tab onto (docs/DECISIONS/0015-tab-groups.md, 0034-tutorial.md).
//
// A carried cell asks it what the real strip is asked, through carryOver(), dropTab() and
// endCarry(), so TabPreview carries a tab here exactly as it does over the real one: the
// name under the finger is lit, and a tab dropped on it leaves the grid for that group.
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: strip

    // How many tabs the group in front holds.
    property int tabCount: 0
    // 1 while a carried tab is over the other group's name, -1 otherwise: the group in
    // front is never lit, since the tab is in it already.
    property int dropIndex: -1

    // A tab was dropped on the other group.
    signal tabDropped(int tabId)

    // Where the other group's name is, in the strip's own coordinates: where the
    // tutorial's hint carries a tab to.
    readonly property point otherCentre: Qt.point(names.x + other.x + other.width / 2,
                                                  height / 2)

    function carryOver(item, x, y) {
        var at = mapFromItem(item, x, y)
        if (at.x < 0 || at.x > width || at.y < 0 || at.y > height) {
            dropIndex = -1
            return false
        }
        var inOther = mapToItem(other, at.x, at.y)
        dropIndex = inOther.x >= 0 && inOther.x < other.width ? 1 : -1
        return true
    }

    function dropTab(tabId) {
        var index = dropIndex
        dropIndex = -1
        if (index < 0) {
            return false
        }
        dropMove.tabId = tabId
        dropMove.restart()
        return true
    }

    function endCarry() {
        dropIndex = -1
    }

    // After the finger's release has run its course, as the real strip waits: the cell
    // leaves the grid, and its handler must not find its context gone under it.
    Timer {
        id: dropMove

        property int tabId: 0

        interval: 0
        onTriggered: strip.tabDropped(tabId)
    }

    Icon {
        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeSmallPlus
        height: width
        source: "image://theme/icon-m-add"
    }

    Icon {
        anchors {
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeSmallPlus
        height: width
        source: "image://theme/icon-m-edit"
    }

    Row {
        id: names

        anchors.centerIn: parent
        height: parent.height

        Item {
            width: current.implicitWidth + 2 * Theme.paddingLarge
            height: parent.height

            Label {
                id: current

                objectName: "tutorialCurrentGroup"
                anchors.centerIn: parent
                text: qsTr("%n tab(s)", "", strip.tabCount)
                font.pixelSize: Theme.fontSizeMedium
                color: Theme.highlightColor
            }

            Rectangle {
                x: current.x
                y: current.y + current.height + Theme.paddingSmall
                width: current.width
                height: Theme._lineWidth
                color: Theme.highlightColor
            }
        }

        Item {
            id: other

            objectName: "tutorialOtherGroup"
            width: otherLabel.implicitWidth + 2 * Theme.paddingLarge
            height: parent.height

            Rectangle {
                anchors.centerIn: otherLabel
                width: otherLabel.width + 2 * Theme.paddingMedium
                height: otherLabel.height + 2 * Theme.paddingSmall
                color: Theme.rgba(Theme.highlightBackgroundColor,
                                  Theme.highlightBackgroundOpacity)
                visible: strip.dropIndex === 1
            }

            Label {
                id: otherLabel

                anchors.centerIn: parent
                //: The name of the made-up tab group the tutorial moves a tab into
                text: qsTr("Work")
                font.pixelSize: Theme.fontSizeMedium
                color: strip.dropIndex === 1 ? Theme.highlightColor : Theme.primaryColor
            }
        }
    }
}
