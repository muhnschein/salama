// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Silica TabBar geometry rebuilt from public API: TabBar is in Sailfish.Silica.private and
// needs TabView, neither Harbour-allowed. Whole foot is drop zone while carrying, so no
// trades with cells under it.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Item {
    id: strip

    signal newTabRequested()
    signal closedTabsRequested()
    signal editRequested()

    // row.children in expression so it re-evaluates as buttons come and go.
    readonly property Item currentButton: TabModel.currentGroupIndex >= 0
                                          && TabModel.currentGroupIndex < buttons.count
                                          ? (row.children, buttons.itemAt(TabModel.currentGroupIndex))
                                          : null
    // -1 none. Never current group.
    property int dropIndex: -1

    objectName: "tabGroupStrip"

    function select(index) {
        TabGroups.activate(index)
    }

    // Hidden (scrolled-out) names aren't drop targets.
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

    // True while over strip, so cell skips trades.
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

    function endCarry() {
        dropIndex = -1
    }

    // Deferred past release: moving at once removes carried cell while its handler runs,
    // clearing its context mid-handler.
    Timer {
        id: dropMove

        property int tabId: 0
        property int groupId: 0

        objectName: "groupDropMove"
        interval: 0
        onTriggered: TabGroups.moveTab(tabId, groupId)
    }

    IconButton {
        id: newTabButton

        objectName: "newTabButton"
        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin - Theme.paddingMedium
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeSmallPlus + 2 * Theme.paddingMedium
        height: parent.height
        icon.source: "image://theme/icon-m-add"
        icon.sourceSize: Qt.size(Theme.iconSizeSmallPlus, Theme.iconSizeSmallPlus)
        onClicked: strip.newTabRequested()
        onPressAndHold: strip.closedTabsRequested()
    }

    IconButton {
        id: editButton

        objectName: "editGroupsButton"
        anchors {
            right: parent.right
            rightMargin: Theme.horizontalPageMargin - Theme.paddingMedium
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeSmallPlus + 2 * Theme.paddingMedium
        height: parent.height
        icon.source: "image://theme/icon-m-edit"
        icon.sourceSize: Qt.size(Theme.iconSizeSmallPlus, Theme.iconSizeSmallPlus)
        onClicked: strip.editRequested()
    }

    Flickable {
        id: flick

        readonly property real pastLeft: contentX
        readonly property real pastRight: contentWidth - width - contentX

        objectName: "tabGroupList"
        anchors {
            left: newTabButton.right
            right: editButton.left
            top: parent.top
            bottom: parent.bottom
        }
        clip: true
        contentWidth: row.width
        contentHeight: height
        boundsBehavior: Flickable.StopAtBounds
        flickableDirection: Flickable.HorizontalFlick
        // Not interactive when names fit, so stray sideways drag goes nowhere.
        interactive: contentWidth > width
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

            // Half slack to first and last button centres a fitting row.
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
                    readonly property bool target: index === strip.dropIndex
                    readonly property real contentWidth: label.implicitWidth + 2 * Theme.paddingLarge

                    objectName: "tabGroupItem"
                    width: contentWidth + (index === 0 ? row.extraMargin : 0)
                           + (index === buttons.count - 1 ? row.extraMargin : 0)
                    height: row.height

                    Rectangle {
                        objectName: "tabGroupDropHighlight"
                        anchors.centerIn: label
                        width: label.width + 2 * Theme.paddingMedium
                        height: label.height + 2 * Theme.paddingSmall
                        radius: Theme.paddingSmall
                        color: Theme.rgba(Theme.highlightBackgroundColor,
                                          Theme.highlightBackgroundOpacity)
                        visible: button.target
                    }

                    Label {
                        id: label

                        objectName: "tabGroupLabel"
                        // Outer buttons hug inside edge so slack stays outside name.
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
                        text: model.name.length > 0 ? model.name
                                                    : qsTr("%n tab(s)", "", model.tabCount)
                        font.pixelSize: Theme.fontSizeMedium
                        color: button.current || button.target || tap.pressed
                               ? Theme.highlightColor : Theme.primaryColor
                    }

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

    // Ends with hidden names fade, narrowing as row reaches end: TabBar's two ramps, chained.
    OpacityRampEffect {
        id: leftFade

        objectName: "tabGroupLeftFade"
        sourceItem: flick
        enabled: flick.interactive && flick.pastLeft > 0
        direction: OpacityRamp.RightToLeft
        slope: Math.max(1 + 20 * flick.width / Screen.width,
                        flick.width / Math.max(1, flick.pastLeft))
        offset: 1 - 1 / slope
    }

    OpacityRampEffect {
        objectName: "tabGroupRightFade"
        sourceItem: leftFade.enabled ? leftFade : flick
        enabled: flick.interactive && flick.pastRight > 0
        direction: OpacityRamp.LeftToRight
        slope: Math.max(1 + 20 * flick.width / Screen.width,
                        flick.width / Math.max(1, flick.pastRight))
        offset: 1 - 1 / slope
    }
}
