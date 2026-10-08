// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

DockedPanel {
    id: panel

    signal tabReopened()

    objectName: "recentlyClosedPanel"
    dock: Dock.Bottom

    SheetBackground {
        anchors.fill: parent
    }

    SheetShade {
        objectName: "panelShade"
        edgeOf: panel
    }

    SheetGrip {
        objectName: "panelDragHandle"
        edgeOf: panel
    }

    SilicaListView {
        id: closedList

        objectName: "closedTabList"
        anchors.fill: parent
        model: ClosedTabs
        clip: true
        header: Item {
            width: closedList.width
            height: Theme.paddingMedium + heading.height

            SectionHeader {
                id: heading

                objectName: "recentlyClosedTitle"
                y: Theme.paddingMedium
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
