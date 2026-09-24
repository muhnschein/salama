// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Clear browsing data: a switch for each kind of it, asked in one dialog as
// sailfish-browser asks (apps/browser/qml/pages/PrivacySettingsPage.qml) and in the
// order Firefox for Android's Delete browsing data lists them. What is cleared to
// forget where one has been is on to begin with; the open tabs are not, since closing
// them takes away what is being read. Clear is dimmed while nothing is on.
//
// The dialog only asks. The privacy page it is opened from reads the four switches as
// it is accepted and does the clearing, under one remorse of its own
// (docs/DECISIONS/0028-settings-pages.md).
import QtQuick 2.6
import Sailfish.Silica 1.0

Dialog {
    id: dialog

    property alias clearTabs: tabsSwitch.checked
    property alias clearHistory: historySwitch.checked
    property alias clearSiteData: siteDataSwitch.checked
    property alias clearCache: cacheSwitch.checked

    objectName: "clearDataDialog"
    allowedOrientations: Orientation.Portrait
    canAccept: clearTabs || clearHistory || clearSiteData || clearCache

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column

            width: parent.width

            DialogHeader {
                title: qsTr("Clear browsing data")
                //: Accepts the dialog, clearing what is switched on
                acceptText: qsTr("Clear")
            }

            TextSwitch {
                id: tabsSwitch

                objectName: "clearTabsSwitch"
                text: qsTr("Open tabs")
            }

            TextSwitch {
                id: historySwitch

                objectName: "clearHistorySwitch"
                text: qsTr("History")
                checked: true
            }

            TextSwitch {
                id: siteDataSwitch

                objectName: "clearSiteDataSwitch"
                text: qsTr("Cookies and site data")
                checked: true
            }

            TextSwitch {
                id: cacheSwitch

                objectName: "clearCacheSwitch"
                text: qsTr("Cache")
                checked: true
            }
        }

        VerticalScrollDecorator {}
    }
}
