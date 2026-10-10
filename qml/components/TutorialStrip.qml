// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Same carryOver()/dropTab()/endCarry() API as TabGroupStrip, for TabPreview. Vertical
// (landscape rail): one name a row, + and edit at foot.
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: strip

    property int tabCount: 0
    // 1 while carried tab over other group, else -1.
    property int dropIndex: -1
    property bool vertical: false

    signal tabDropped(int tabId)

    readonly property point otherCentre: Qt.point(names.x + other.x + other.width / 2,
                                                  vertical ? names.y + other.y + other.height / 2
                                                           : height / 2)

    function carryOver(item, x, y) {
        var at = mapFromItem(item, x, y)
        if (at.x < 0 || at.x > width || at.y < 0 || at.y > height) {
            dropIndex = -1
            return false
        }
        var inOther = mapToItem(other, at.x, at.y)
        dropIndex = inOther.x >= 0 && inOther.x < other.width
                && inOther.y >= 0 && inOther.y < other.height ? 1 : -1
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

    // Deferred past release: cell's handler must keep its context.
    Timer {
        id: dropMove

        property int tabId: 0

        interval: 0
        onTriggered: strip.tabDropped(tabId)
    }

    Icon {
        id: addIcon

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
        id: editIcon

        anchors {
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeSmallPlus
        height: width
        source: "image://theme/icon-m-edit"
    }

    // One row, or one column when vertical.
    Grid {
        id: names

        anchors {
            horizontalCenter: parent.horizontalCenter
            verticalCenter: parent.verticalCenter
        }
        height: strip.vertical ? implicitHeight : parent.height
        rows: strip.vertical ? 0 : 1
        columns: strip.vertical ? 1 : 0

        Item {
            objectName: "tutorialCurrentRow"
            width: strip.vertical ? strip.width : current.implicitWidth + 2 * Theme.paddingLarge
            height: strip.vertical ? Theme.itemSizeSmall : names.height

            Label {
                id: current

                objectName: "tutorialCurrentGroup"
                x: strip.vertical ? Theme.horizontalPageMargin : (parent.width - width) / 2
                anchors.verticalCenter: parent.verticalCenter
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
                visible: !strip.vertical
            }

            Rectangle {
                objectName: "tutorialCurrentAccent"
                x: parent.width - width
                width: Theme.paddingSmall
                height: parent.height
                color: Theme.highlightColor
                visible: strip.vertical
            }
        }

        Item {
            id: other

            objectName: "tutorialOtherGroup"
            width: strip.vertical ? strip.width : otherLabel.implicitWidth + 2 * Theme.paddingLarge
            height: strip.vertical ? Theme.itemSizeSmall : names.height

            Rectangle {
                anchors.centerIn: otherLabel
                width: otherLabel.width + 2 * Theme.paddingMedium
                height: otherLabel.height + 2 * Theme.paddingSmall
                radius: Theme.paddingSmall
                color: Theme.rgba(Theme.highlightBackgroundColor,
                                  Theme.highlightBackgroundOpacity)
                visible: strip.dropIndex === 1
            }

            Label {
                id: otherLabel

                x: strip.vertical ? Theme.horizontalPageMargin : (parent.width - width) / 2
                anchors.verticalCenter: parent.verticalCenter
                //: The name of the made-up tab group the tutorial moves a tab into
                text: qsTr("Work")
                font.pixelSize: Theme.fontSizeMedium
                color: strip.dropIndex === 1 ? Theme.highlightColor : Theme.primaryColor
            }
        }
    }

    // Names from top, icons in a row at foot.
    states: State {
        when: strip.vertical

        AnchorChanges {
            target: names
            anchors.horizontalCenter: undefined
            anchors.verticalCenter: undefined
            anchors.left: strip.left
            anchors.top: strip.top
        }
        AnchorChanges {
            target: addIcon
            anchors.verticalCenter: undefined
            anchors.bottom: strip.bottom
        }
        AnchorChanges {
            target: editIcon
            anchors.verticalCenter: undefined
            anchors.bottom: strip.bottom
        }
        PropertyChanges {
            target: addIcon
            anchors.bottomMargin: (Theme.itemSizeSmall - addIcon.height) / 2
        }
        PropertyChanges {
            target: editIcon
            anchors.bottomMargin: (Theme.itemSizeSmall - editIcon.height) / 2
        }
    }
}
