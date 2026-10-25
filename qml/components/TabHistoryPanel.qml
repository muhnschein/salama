// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Back hold: pages before this one in tab, newest first (Firefox's tab history).
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

DockedPanel {
    id: panel

    property var entries: []

    signal entryChosen(var entry)

    objectName: "tabHistoryPanel"
    width: parent.width
    // From entries, not content.height: Column lays out a frame late, so first show slid toward
    // header-only height then jumped open.
    height: Math.min(2 * Theme.paddingMedium + heading.height
                     + entries.length * Theme.itemSizeSmall,
                     Math.round(parent.height * 0.6))
    dock: Dock.Bottom
    // Shade sits one under: over page, so taps beside sheet reach it.
    z: 2

    function showFor(list) {
        entries = list
        show()
    }

    Connections {
        target: TabModel
        onActiveTabChanged: panel.hide()
    }

    SheetBackground {
        anchors.fill: parent
    }

    SheetShade {
        objectName: "tabHistoryShade"
        edgeOf: panel
    }

    SheetGrip {
        objectName: "tabHistoryDragHandle"
        edgeOf: panel
    }

    // TabHistory caps rows: all made.
    SilicaFlickable {
        objectName: "tabHistoryList"
        anchors.fill: parent
        contentHeight: content.height
        clip: true

        Column {
            id: content

            width: parent.width
            topPadding: Theme.paddingMedium
            bottomPadding: Theme.paddingMedium

            SectionHeader {
                id: heading

                objectName: "tabHistoryTitle"
                //: Heading of the list of pages visited before this one in the tab
                text: qsTr("Tab history")
            }

            Repeater {
                model: panel.entries

                TabRow {
                    objectName: "tabHistoryRow"
                    width: content.width
                    contentHeight: Theme.itemSizeSmall
                    title: modelData.title.length > 0 ? modelData.title : modelData.url
                    icon: modelData.favicon
                    onClicked: {
                        panel.hide()
                        panel.entryChosen(modelData)
                    }
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
