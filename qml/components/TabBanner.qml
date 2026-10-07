// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

BarBanner {
    id: banner

    property bool allowed: true
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

    // Any tab change to front ends banner.
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
