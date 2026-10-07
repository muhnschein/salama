// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Heading in row, not list section, so two unnamed groups with same tab count stay separate.
import QtQuick 2.6
import Sailfish.Silica 1.0

Column {
    id: delegate

    signal chosen()

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
