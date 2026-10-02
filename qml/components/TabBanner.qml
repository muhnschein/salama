// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Says where a link opened behind the page in front went: the link sheet's Background
// tab leaves the reader where they were, and this says so for a few seconds, on the bar
// every banner is (BarBanner.qml) -- the link's name, and the group it went into when the
// group has a name -- with Show to go to it (docs/DECISIONS/0046-link-menu.md). A swipe
// sideways takes it away sooner.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

BarBanner {
    id: banner

    // Whether the page leaves room for it, as for the downloads' banner.
    property bool allowed: true
    // The tab opened, its name, and the name of its group.
    property int tabId: 0
    property string tabTitle
    property string groupName
    readonly property bool saying: sayTimer.running

    function opened(newTabId, title) {
        tabId = newTabId
        tabTitle = title
        groupName = TabModel.groupNameOf(newTabId)
        sayTimer.restart()
    }

    function activate() {
        sayTimer.stop()
        TabModel.activateTabById(tabId)
    }

    function dismiss() {
        sayTimer.stop()
    }

    objectName: "tabBanner"
    shown: allowed && saying
    //: The banner as a link opens in a tab behind the one in front
    title: qsTr("Opened in a new tab")
    //: Under "Opened in a new tab": the link's name, and the named tab group it went into
    detail: groupName.length > 0 ? qsTr("%1 · in %2").arg(tabTitle).arg(groupName) : tabTitle
    onActivated: activate()
    onDismissed: dismiss()

    Icon {
        objectName: "tabBannerIcon"
        anchors.centerIn: parent
        source: "image://theme/icon-m-tabs"
        sourceSize: Qt.size(Theme.iconSizeSmallPlus, Theme.iconSizeSmallPlus)
        highlighted: banner.pressed
    }

    // The grid takes over from here: a tab that comes to the front, by Show or otherwise,
    // has been found.
    Connections {
        target: TabModel
        onActiveTabChanged: banner.dismiss()
    }

    Timer {
        id: sayTimer

        objectName: "tabBannerTimer"
        interval: 4000
    }
}
