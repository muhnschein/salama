// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 tuuli contributors
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.tuuli 1.0
import "../components"

Page {
    id: tabsPage

    objectName: "tabsPage"
    allowedOrientations: Orientation.Portrait

    SilicaGridView {
        id: tabGrid

        objectName: "tabGrid"
        anchors.fill: parent
        model: TabModel
        cellWidth: width / 2
        cellHeight: cellWidth + Theme.itemSizeSmall

        // The header names the tab the pulley returns to, as the sketch has it.
        header: PageHeader {
            objectName: "tabsHeader"
            title: TabModel.activeTitle.length > 0 ? TabModel.activeTitle : TabModel.activeUrl
            description: qsTr("%n tab(s)", "", TabModel.count)
        }

        PullDownMenu {
            MenuItem {
                objectName: "closeAllTabsMenu"
                text: qsTr("Close all tabs")
                onClicked: Remorse.popupAction(tabsPage, qsTr("Closing all tabs"), function () {
                    TabModel.closeAllTabs()
                    pageStack.pop()
                })
            }
            MenuItem {
                objectName: "newPrivateTabMenu"
                text: qsTr("New private tab")
                onClicked: {
                    TabModel.newTab(Settings.homePage, true)
                    pageStack.pop()
                }
            }
            MenuItem {
                objectName: "newTabMenu"
                text: qsTr("New tab")
                onClicked: {
                    TabModel.newTab(Settings.homePage)
                    pageStack.pop()
                }
            }
            MenuItem {
                objectName: "goToTabMenu"
                text: qsTr("Go to tab")
                enabled: TabModel.count > 0
                onClicked: pageStack.pop()
            }
        }

        delegate: TabPreview {
            onClicked: {
                TabModel.activateTab(index)
                pageStack.pop()
            }
            onCloseRequested: TabModel.closeTab(index)
        }

        ViewPlaceholder {
            enabled: TabModel.count === 0
            text: qsTr("No open tabs")
            hintText: qsTr("Pull down to open one")
        }

        VerticalScrollDecorator {}
    }
}
