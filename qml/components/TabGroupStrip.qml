// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 tuuli contributors
//
// The head of the tab grid: the groups in a row, each the width of its name, the
// current one underlined and kept in the middle. The two corners hold the way to edit
// the groups and the way to search every tab (docs/DECISIONS/0015-tab-groups.md).
//
// The geometry is Silica's own TabBar's, rebuilt from public API the way vuo rebuilds
// it (ScopeTabBar.qml): a label with Theme.paddingLarge either side, an underline of
// Theme._lineWidth exactly as wide as the current label, the current label in the
// highlight colour, and the first and last tab taking the slack so a row that fits is
// centred. TabBar itself lives in Sailfish.Silica.private and works only inside a
// TabView, neither of which a Harbour application may have. Small type, because the
// strip sits over the grid rather than at the head of a page.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.tuuli 1.0

Item {
    id: strip

    signal editRequested()
    signal searchRequested()

    // The button of the current group, once the Repeater has made it. row.children
    // is in the expression so it re-runs as buttons come and go.
    readonly property Item currentButton: TabModel.currentGroupIndex >= 0
                                          && TabModel.currentGroupIndex < buttons.count
                                          ? (row.children, buttons.itemAt(TabModel.currentGroupIndex))
                                          : null

    objectName: "tabGroupStrip"

    // A tap on a group: what the buttons call, and what the load tests call.
    function select(index) {
        TabGroups.activate(index)
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

    Flickable {
        id: flick

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
                    // Width without the slack, which the row sums to work the slack out.
                    readonly property real contentWidth: label.implicitWidth + 2 * Theme.paddingLarge

                    objectName: "tabGroupItem"
                    width: contentWidth + (index === 0 ? row.extraMargin : 0)
                           + (index === buttons.count - 1 ? row.extraMargin : 0)
                    height: row.height

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
                        // the tabs outside every group; the private one by what it is.
                        text: model.privateGroup ? qsTr("Private")
                                                 : model.name.length > 0 ? model.name
                                                                         : qsTr("%n tab(s)", "", model.tabCount)
                        font.pixelSize: Theme.fontSizeSmall
                        color: button.current || tap.pressed ? Theme.highlightColor
                                                             : Theme.primaryColor
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
