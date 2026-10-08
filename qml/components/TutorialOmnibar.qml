// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Item {
    id: pane

    property string typed
    property string address

    Rectangle {
        anchors.fill: parent
        color: Theme.highlightDimmerColor
    }

    GlassTexture {
        anchors.fill: parent
    }

    Column {
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        Repeater {
            model: 2

            Item {
                width: parent.width
                height: Theme.itemSizeMedium

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
                        width: pane.width / (index === 0 ? 2 : 2.5)
                        height: Theme.paddingMedium
                        radius: height / 2
                        color: Theme.rgba(Theme.primaryColor, Theme.opacityLow)
                    }

                    Rectangle {
                        width: pane.width / 3
                        height: Theme.paddingMedium
                        radius: height / 2
                        color: Theme.rgba(Theme.primaryColor, Theme.opacityFaint)
                    }
                }
            }
        }

        OmnibarAction {
            objectName: "tutorialGoAction"
            width: parent.width
            enabled: false
            iconSource: "image://theme/icon-m-right"
            //: The row above the address bar that opens what was typed as an address
            title: qsTr("Go to %1").arg(pane.typed)
            subtitle: pane.address
        }

        OmnibarAction {
            objectName: "tutorialSearchAction"
            width: parent.width
            enabled: false
            iconSource: "image://theme/icon-m-search"
            //: The row above the address bar that searches the web: %1 is the search
            //: engine's name, %2 what was typed
            title: qsTr("Search %1 for “%2”").arg(SearchEngines.engineNames[SearchSettings.engineIndex])
                                           .arg(pane.typed)
        }
    }
}
