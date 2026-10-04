// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The tabs closed most recently, newest first, in a panel that comes up from under
// the grid's foot when the new-tab button is held. A tap opens the tab again and
// puts the panel away; a tap outside it puts it away alone. Silica's DockedPanel is
// the platform's own way of sliding a sheet in from an edge, and modal, it takes
// the tap outside itself (docs/DECISIONS/0018-recently-closed.md). It is the same
// sheet as the browser's menu, on the same opaque ground with the same handle, and
// headed as Silica heads a section, so that the two read as one kind of thing.
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

    SheetBackground {
        anchors.fill: parent
    }

    SilicaListView {
        id: closedList

        objectName: "closedTabList"
        anchors.fill: parent
        model: ClosedTabs
        clip: true
        // The handle in a strip as tall as the menu's, and under it the heading, as the
        // grid's search heads each group it finds.
        header: Item {
            width: closedList.width
            height: Theme.paddingLarge + heading.height

            DragHandle {
                objectName: "panelDragHandle"
                x: (parent.width - width) / 2
                // High in its strip, close under the panel's top edge (issue #38).
                y: Theme.paddingSmall / 2
            }

            SectionHeader {
                id: heading

                objectName: "recentlyClosedTitle"
                y: Theme.paddingLarge
                text: qsTr("Recently closed")
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
