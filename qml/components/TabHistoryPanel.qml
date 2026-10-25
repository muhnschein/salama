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
    height: Math.min(content.height, Math.round(parent.height * 0.6))
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

    // TabHistory caps rows: all made, sheet sized to them.
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
                objectName: "tabHistoryTitle"
                //: Heading of the list of pages visited before this one in the tab
                text: qsTr("Tab history")
            }

            Repeater {
                model: panel.entries

                TabRow {
                    objectName: "tabHistoryRow"
                    width: content.width
                    title: modelData.title
                    subtitle: modelData.url
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
