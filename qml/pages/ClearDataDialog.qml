// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Clear browsing data: how far back, and a switch for each kind of it, asked in one
// dialog as sailfish-browser asks (apps/browser/qml/pages/PrivacySettingsPage.qml) and
// in the order Firefox for Android's Delete browsing data lists them. The time range is
// Firefox's own dialog's, everything to begin with; it reaches the history and the list
// of downloads, which know when they were made, and the rest is cleared whole, as the
// line under it says. What is cleared to forget where one has been is on to begin
// with; the open tabs are not, since closing them takes away what is being read.
// Clear is dimmed while nothing is on.
//
// The dialog only asks. The history page it is opened from reads it as it is accepted
// and does the clearing, under one remorse of its own
// (docs/DECISIONS/0030-history-settings.md).
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Dialog {
    id: dialog

    // A HistoryModel.ClearRange: its index in the list is its value.
    property alias range: rangeCombo.currentIndex
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

            ComboBox {
                id: rangeCombo

                objectName: "clearRangeCombo"
                width: parent.width
                label: qsTr("Time range")
                description: currentIndex === HistoryModel.ClearEverything
                             ? "" : qsTr("Open tabs, cookies, site data and the cache are cleared whole")
                currentIndex: HistoryModel.ClearEverything
                menu: ContextMenu {
                    MenuItem {
                        text: qsTr("Last hour")
                    }

                    MenuItem {
                        text: qsTr("Last two hours")
                    }

                    MenuItem {
                        text: qsTr("Last four hours")
                    }

                    MenuItem {
                        text: qsTr("Today")
                    }

                    MenuItem {
                        text: qsTr("Everything")
                    }
                }
            }

            TextSwitch {
                id: tabsSwitch

                objectName: "clearTabsSwitch"
                text: qsTr("Open tabs")
            }

            TextSwitch {
                id: historySwitch

                objectName: "clearHistorySwitch"
                text: qsTr("Browsing and download history")
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
