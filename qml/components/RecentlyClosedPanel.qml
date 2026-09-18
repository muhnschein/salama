// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The tabs closed most recently, newest first, in a panel that comes up from under
// the grid's foot when the new-tab button is held. A tap opens the tab again and
// puts the panel away; a tap outside it puts it away alone. Silica's DockedPanel is
// the platform's own way of sliding a sheet in from an edge, and modal, it takes
// the tap outside itself (docs/DECISIONS/0018-recently-closed.md).
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

DockedPanel {
    id: panel

    // A tab was opened again; the page is wanted back with it.
    signal tabReopened()

    objectName: "recentlyClosedPanel"
    dock: Dock.Bottom
    modal: true

    Rectangle {
        anchors.fill: parent
        color: Theme.rgba(Theme.highlightDimmerColor, Theme.opacityOverlay)
    }

    SilicaListView {
        id: closedList

        objectName: "closedTabList"
        anchors.fill: parent
        model: ClosedTabs
        clip: true
        header: Item {
            width: closedList.width
            height: Theme.itemSizeLarge

            DragHandle {
                objectName: "panelDragHandle"
                x: (parent.width - width) / 2
                y: Theme.paddingSmall
            }

            Label {
                objectName: "recentlyClosedTitle"
                anchors {
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                    leftMargin: Theme.horizontalPageMargin
                    rightMargin: Theme.horizontalPageMargin
                    bottomMargin: Theme.paddingMedium
                }
                text: qsTr("Recently closed")
                font.pixelSize: Theme.fontSizeLarge
                color: Theme.highlightColor
                truncationMode: TruncationMode.Fade
            }
        }

        delegate: ClosedTabDelegate {
            onClicked: {
                ClosedTabs.reopen(index)
                panel.hide()
                panel.tabReopened()
            }
        }

        ViewPlaceholder {
            enabled: ClosedTabs.count === 0
            text: qsTr("Nothing closed recently")
        }

        VerticalScrollDecorator {}
    }
}
