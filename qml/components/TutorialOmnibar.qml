// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Landscape, as omnibar: go/search are pills in field row below pane, hits in three columns.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Item {
    id: pane

    property string typed
    property string address
    property bool landscape: false
    // Field row below pane: pills' height and right end there.
    property real rowHeight: 0
    property real pillsRight: width
    readonly property real pillsWidth: landscape ? pills.width : 0
    readonly property real hitWidth: landscape ? Math.floor(width / 3) : width

    Rectangle {
        anchors.fill: parent
        color: Theme.highlightDimmerColor
    }

    GlassTexture {
        anchors.fill: parent
    }

    Component {
        id: hit

        Item {
            width: pane.hitWidth
            height: pane.landscape ? Theme.itemSizeExtraSmall : Theme.itemSizeMedium

            Rectangle {
                id: initial

                x: Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
                width: Theme.iconSizeMedium
                height: width
                radius: Theme.paddingSmall
                color: Theme.rgba(Theme.primaryColor, Theme.opacityFaint)
            }

            Column {
                anchors {
                    left: initial.right
                    leftMargin: Theme.paddingMedium
                    verticalCenter: parent.verticalCenter
                }
                spacing: Theme.paddingSmall

                Rectangle {
                    width: pane.hitWidth / (index === 0 ? 2 : 2.5)
                    height: Theme.paddingMedium
                    radius: height / 2
                    color: Theme.rgba(Theme.primaryColor, Theme.opacityLow)
                }

                Rectangle {
                    width: pane.hitWidth / 3
                    height: Theme.paddingMedium
                    radius: height / 2
                    color: Theme.rgba(Theme.primaryColor, Theme.opacityFaint)
                }
            }
        }
    }

    Column {
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        Repeater {
            model: pane.landscape ? 0 : 2
            delegate: hit
        }

        OmnibarAction {
            objectName: "tutorialGoAction"
            width: parent.width
            visible: !pane.landscape
            enabled: false
            iconSource: "image://theme/icon-m-right"
            //: The row above the address bar that opens what was typed as an address
            title: qsTr("Go to %1").arg(pane.typed)
            subtitle: pane.address
        }

        OmnibarAction {
            objectName: "tutorialSearchAction"
            width: parent.width
            visible: !pane.landscape
            enabled: false
            iconSource: "image://theme/icon-m-search"
            //: The row above the address bar that searches the web: %1 is the search
            //: engine's name, %2 what was typed
            title: qsTr("Search %1 for “%2”").arg(SearchEngines.engineNames[SearchSettings.engineIndex])
                                           .arg(pane.typed)
        }
    }

    // Three columns at foot, as omnibar's.
    Grid {
        id: columns

        objectName: "tutorialHitColumns"
        anchors.bottom: parent.bottom
        visible: pane.landscape
        flow: Grid.TopToBottom
        rows: 2

        Repeater {
            model: pane.landscape ? 6 : 0
            delegate: hit
        }
    }

    // Anchors, not Row: as omnibar's, field and pills move in same frame.
    Item {
        id: pills

        objectName: "tutorialPills"
        x: pane.pillsRight - width
        y: pane.height + (pane.rowHeight - height) / 2
        width: goPill.width + Theme.paddingMedium + searchPill.width
        height: searchPill.height
        visible: pane.landscape

        OmnibarPill {
            id: goPill

            objectName: "tutorialGoPill"
            anchors {
                right: searchPill.left
                rightMargin: Theme.paddingMedium
            }
            enabled: false
            accent: true
            iconSource: "image://theme/icon-m-right"
        }

        OmnibarPill {
            id: searchPill

            objectName: "tutorialSearchPill"
            anchors.right: parent.right
            enabled: false
            iconSource: "image://theme/icon-m-search"
            label: SearchEngines.engineNames[SearchSettings.engineIndex]
        }
    }
}
