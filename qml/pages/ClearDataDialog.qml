// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
// Only asks. Caller (HistorySettingsPage.qml) clears under one remorse.
// Range reaches history and downloads only; rest cleared whole.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Dialog {
    id: dialog

    // HistoryModel.ClearRange: list index = value.
    property alias range: rangeCombo.currentIndex
    property alias clearTabs: tabsSwitch.checked
    property alias clearHistory: historySwitch.checked
    property alias clearSiteData: siteDataSwitch.checked
    property alias clearCache: cacheSwitch.checked

    objectName: "clearDataDialog"
    allowedOrientations: Orientation.Portrait
    canAccept: clearTabs || clearHistory || clearSiteData || clearCache

    // Closed tabs go only with everything. Counts reread on history change.
    readonly property double since: HistoryModel.rangeStart(range)
    readonly property int pages: (HistoryModel.pageCount, HistoryModel.countSince(since))
    readonly property int downloads: (DownloadModel.count, DownloadModel.countSince(since))
    readonly property int closedTabs: range === HistoryModel.ClearEverything ? ClosedTabs.count : 0

    function historyText(pages, downloads, closedTabs) {
        var parts = []
        if (pages > 0) {
            //: Pages of the history that clearing takes
            parts.push(qsTr("%n page(s)", "", pages))
        }
        if (downloads > 0) {
            //: Rows of the list of downloads that clearing takes
            parts.push(qsTr("%n download(s)", "", downloads))
        }
        if (closedTabs > 0) {
            //: Recently closed tabs that clearing takes
            parts.push(qsTr("%n closed tab(s)", "", closedTabs))
        }
        if (parts.length === 3) {
            //: Three amounts cleared: "342 pages, 18 downloads and 6 closed tabs"
            return qsTr("%1, %2 and %3").arg(parts[0]).arg(parts[1]).arg(parts[2])
        }
        if (parts.length === 2) {
            //: Two amounts cleared: "342 pages and 18 downloads"
            return qsTr("%1 and %2").arg(parts[0]).arg(parts[1])
        }
        //: The history holds nothing from the time range chosen
        return parts.length === 1 ? parts[0] : qsTr("Nothing from this time")
    }

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
                //: How many tabs clearing the open tabs closes
                description: qsTr("%n tab(s), in every group", "", TabModel.count)
            }

            TextSwitch {
                id: historySwitch

                objectName: "clearHistorySwitch"
                text: qsTr("Browsing and download history")
                description: dialog.historyText(dialog.pages, dialog.downloads, dialog.closedTabs)
                checked: true
            }

            TextSwitch {
                id: siteDataSwitch

                objectName: "clearSiteDataSwitch"
                text: qsTr("Cookies and site data")
                //: What clearing the cookies does
                description: qsTr("Signs you out of most sites")
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
