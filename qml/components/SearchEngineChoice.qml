// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// TextSwitch eats press-and-hold, so row forwards it to open menu.
import QtQuick 2.6
import Sailfish.Silica 1.0

ListItem {
    id: row

    property alias text: choice.text
    property alias description: choice.description
    property alias checked: choice.checked
    property bool removable: false

    signal chosen()
    signal removeRequested()

    objectName: "searchEngineRow"
    width: parent.width
    contentHeight: choice.height
    menu: ContextMenu {
        MenuItem {
            objectName: "searchEngineRemove"
            //: Takes an engine that was added while browsing out of the list
            text: qsTr("Remove")
            onClicked: row.removeRequested()
        }
    }

    TextSwitch {
        id: choice

        objectName: "searchEngineChoice"
        width: parent.width
        automaticCheck: false
        onClicked: row.chosen()
        onPressAndHold: {
            if (row.removable) {
                row.openMenu()
            }
        }
    }
}
