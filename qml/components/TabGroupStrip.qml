// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Silica TabBar geometry rebuilt from public API: TabBar is in Sailfish.Silica.private and
// needs TabView, neither Harbour-allowed. Whole foot is drop zone while carrying, so no
// trades with cells under it. Vertical (landscape rail): one name a row, + and edit at foot.
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
    property bool vertical: false

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
            if (!button) {
                continue
            }
            var along = vertical ? inRow.y - button.y : inRow.x - button.x
            if (along >= 0 && along < (vertical ? button.height : button.width)) {
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
        height: strip.vertical ? Theme.itemSizeSmall : parent.height
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
        height: newTabButton.height
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
        contentWidth: strip.vertical ? width : row.width
        contentHeight: strip.vertical ? row.height : height
        boundsBehavior: Flickable.StopAtBounds
        flickableDirection: strip.vertical ? Flickable.VerticalFlick : Flickable.HorizontalFlick
        // Not interactive when names fit, so stray drag goes nowhere.
        interactive: strip.vertical ? contentHeight > height : contentWidth > width
        contentX: {
            var button = strip.currentButton
            if (!button || strip.vertical) {
                return 0
            }
            return Math.max(0, Math.min(contentWidth - width,
                                        button.x + (button.width - width) / 2))
        }
        contentY: {
            var button = strip.currentButton
            if (!button || !strip.vertical) {
                return 0
            }
            return Math.max(0, Math.min(contentHeight - height,
                                        button.y + (button.height - height) / 2))
        }

        Behavior on contentX {
            enabled: !flick.moving

            NumberAnimation {
                duration: 200
                easing.type: Easing.InOutQuad
            }
        }

        Behavior on contentY {
            enabled: !flick.moving

            NumberAnimation {
                duration: 200
                easing.type: Easing.InOutQuad
            }
        }

        // One row, or one column when vertical.
        Grid {
            id: row

            // Half slack to first and last button centres fitting row.
            readonly property real extraMargin: {
                if (strip.vertical) {
                    return 0
                }
                var total = 0
                for (var i = 0; i < row.children.length; ++i) {
                    total += row.children[i].contentWidth || 0
                }
                return Math.max(0, flick.width - total) / 2
            }

            rows: strip.vertical ? 0 : 1
            columns: strip.vertical ? 1 : 0
            height: strip.vertical ? implicitHeight : flick.height

            Repeater {
                id: buttons

                model: TabGroups

                Item {
                    id: button

                    readonly property bool current: model.currentGroup
                    readonly property bool target: index === strip.dropIndex
                    readonly property real contentWidth: label.implicitWidth + 2 * Theme.paddingLarge

                    objectName: "tabGroupItem"
                    width: strip.vertical ? flick.width
                                          : contentWidth + (index === 0 ? row.extraMargin : 0)
                                            + (index === buttons.count - 1 ? row.extraMargin : 0)
                    height: strip.vertical ? Theme.itemSizeSmall : row.height

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
                            if (strip.vertical) {
                                return Theme.horizontalPageMargin
                            }
                            if (buttons.count > 1 && index === 0) {
                                return button.width - width - Theme.paddingLarge
                            }
                            if (buttons.count > 1 && index === buttons.count - 1) {
                                return Theme.paddingLarge
                            }
                            return (button.width - width) / 2
                        }
                        y: (button.height - height) / 2
                        width: strip.vertical ? countLabel.x - x - Theme.paddingMedium
                                              : implicitWidth
                        truncationMode: strip.vertical ? TruncationMode.Fade
                                                       : TruncationMode.None
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
                        visible: button.current && !strip.vertical
                    }

                    // Unnamed group's name is already its count.
                    Label {
                        id: countLabel

                        objectName: "tabGroupCount"
                        x: button.width - width - Theme.horizontalPageMargin
                        y: (button.height - height) / 2
                        visible: strip.vertical && model.name.length > 0
                        width: visible ? implicitWidth : 0
                        text: model.tabCount
                        font.pixelSize: Theme.fontSizeSmall
                        color: button.current || button.target || tap.pressed
                               ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    }

                    Rectangle {
                        objectName: "tabGroupAccent"
                        x: button.width - width
                        width: Theme.paddingSmall
                        height: button.height
                        color: Theme.highlightColor
                        visible: button.current && strip.vertical
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
        enabled: !strip.vertical && flick.interactive && flick.pastLeft > 0
        direction: OpacityRamp.RightToLeft
        slope: Math.max(1 + 20 * flick.width / Screen.width,
                        flick.width / Math.max(1, flick.pastLeft))
        offset: 1 - 1 / slope
    }

    OpacityRampEffect {
        objectName: "tabGroupRightFade"
        sourceItem: leftFade.enabled ? leftFade : flick
        enabled: !strip.vertical && flick.interactive && flick.pastRight > 0
        direction: OpacityRamp.LeftToRight
        slope: Math.max(1 + 20 * flick.width / Screen.width,
                        flick.width / Math.max(1, flick.pastRight))
        offset: 1 - 1 / slope
    }

    states: State {
        when: strip.vertical

        AnchorChanges {
            target: newTabButton
            anchors.verticalCenter: undefined
            anchors.bottom: strip.bottom
        }
        AnchorChanges {
            target: editButton
            anchors.verticalCenter: undefined
            anchors.bottom: strip.bottom
        }
        AnchorChanges {
            target: flick
            anchors.left: strip.left
            anchors.right: strip.right
            anchors.bottom: newTabButton.top
        }
    }
}
