// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// One tab in the search results, under a heading naming its group on the first row
// of each group. The heading is part of the row rather than a section of the list,
// so that two unnamed groups holding the same number of tabs stay two headings. What
// was typed is lit wherever it is in the title and the address (TabRow's match).
import QtQuick 2.6
import Sailfish.Silica 1.0

Column {
    id: delegate

    signal chosen()

    // What the search lights in the row, as TabRow takes it.
    property var match: null

    objectName: "tabSearchDelegate"
    width: ListView.view.width

    SectionHeader {
        objectName: "tabSearchGroupHeader"
        visible: model.groupStart
        text: model.groupName.length > 0 ? model.groupName
                                         : qsTr("%n tab(s)", "", model.groupTabCount)
    }

    TabRow {
        objectName: "tabSearchItem"
        width: parent.width
        title: model.title
        subtitle: model.url
        icon: model.favicon
        match: delegate.match
        onClicked: delegate.chosen()
    }
}
