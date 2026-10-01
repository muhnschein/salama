// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// One search engine in Settings > Search: the TextSwitch that does not check itself, lit
// while the engine is the one in use (docs/DECISIONS/0036-settings-choose-in-place.md),
// with the site an added engine came from under its name. An added engine can be
// removed from a context menu, opened by pressing and holding it; Silica's TextSwitch
// takes the press for itself, so the row passes it on.
import QtQuick 2.6
import Sailfish.Silica 1.0

ListItem {
    id: row

    property alias text: choice.text
    property alias description: choice.description
    property alias checked: choice.checked
    // An added engine, not a built-in one.
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
