// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

// History's and Downloads' section heading, Firefox for Android's words.
SectionHeader {
    /// DateGroup.Group: ListView section, role's int as string.
    property int group

    objectName: "dateSection"
    text: {
        switch (group) {
        case DateGroup.InProgress:
            //: Downloads still running or paused, listed first
            return qsTr("In progress")
        case DateGroup.Today:
            return qsTr("Today")
        case DateGroup.Yesterday:
            return qsTr("Yesterday")
        case DateGroup.LastSevenDays:
            return qsTr("Last 7 days")
        case DateGroup.LastThirtyDays:
            return qsTr("Last 30 days")
        default:
            return qsTr("Older")
        }
    }
}
