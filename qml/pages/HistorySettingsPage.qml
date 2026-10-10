// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
// Clears here, not in dialog: page stack destroys dialog on pop, so remorse runs here.
// Only settings page importing Sailfish.WebEngine: engine clears cookies/site data/cache.
import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.WebEngine 1.0
import harbour.salama 1.0

Page {
    id: historyPage

    objectName: "historySettingsPage"
    allowedOrientations: Orientation.Portrait | Orientation.LandscapeMask

    // Choice passed in: dialog gone before remorse ends. Engine clears all or nothing.
    function clearData(range, tabs, history, siteData, cache) {
        Remorse.popupAction(historyPage, qsTr("Clearing browsing data"), function () {
            if (tabs) {
                TabModel.closeAllTabs()
            }
            if (history) {
                var since = HistoryModel.rangeStart(range)
                HistoryModel.clearSince(since)
                DownloadModel.clearSince(since)
                if (range === HistoryModel.ClearEverything) {
                    ClosedTabs.clear()
                }
            }
            if (siteData) {
                WebEngine.notifyObservers(EngineMessages.clearPrivateDataTopic,
                                          EngineMessages.cookiesAndSiteDataPayload)
            }
            if (cache) {
                WebEngine.notifyObservers(EngineMessages.clearPrivateDataTopic,
                                          EngineMessages.cachePayload)
            }
        })
    }

    function openClearData() {
        var dialog = pageStack.push(Qt.resolvedUrl("ClearDataDialog.qml"))
        dialog.accepted.connect(function () {
            historyPage.clearData(dialog.range, dialog.clearTabs, dialog.clearHistory,
                                  dialog.clearSiteData, dialog.clearCache)
        })
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column

            width: parent.width

            PageHeader {
                title: qsTr("History")
            }

            // Off keeps existing history until cleared.
            TextSwitch {
                objectName: "rememberHistorySwitch"
                text: qsTr("Remember browsing history")
                checked: PrivacySettings.rememberHistory
                onCheckedChanged: PrivacySettings.rememberHistory = checked
            }

            TextSwitch {
                objectName: "clearHistoryOnCloseSwitch"
                text: qsTr("Clear history when closed")
                description: qsTr("With it, the list of downloads and the recently closed tabs")
                checked: PrivacySettings.clearHistoryOnClose
                onCheckedChanged: PrivacySettings.clearHistoryOnClose = checked
            }

            SectionHeader {
                //: What browsing has left on the phone, counted
                text: qsTr("Kept on this phone")
            }

            DetailItem {
                objectName: "keptHistory"
                label: qsTr("History")
                //: How many pages the history keeps
                value: qsTr("%n page(s)", "", HistoryModel.pageCount)
            }

            DetailItem {
                objectName: "keptDownloads"
                label: qsTr("Downloads")
                //: How many downloads the list of them keeps
                value: qsTr("%n file(s)", "", DownloadModel.count)
            }

            DetailItem {
                objectName: "keptClosedTabs"
                label: qsTr("Recently closed")
                //: How many closed tabs can be opened again
                value: qsTr("%n tab(s)", "", ClosedTabs.count)
            }

            DetailItem {
                objectName: "keptOpenTabs"
                label: qsTr("Open tabs")
                //: How many tabs are open, and in how many groups: "17, in 5 groups"
                value: TabGroups.count > 1 ? qsTr("%1, in %n group(s)", "", TabGroups.count)
                                             .arg(TabModel.count)
                                           : qsTr("%n tab(s)", "", TabModel.count)
            }

            Item {
                width: parent.width
                height: Theme.paddingLarge
            }

            // Button, not row: acts, doesn't navigate.
            Button {
                objectName: "clearDataButton"
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Clear browsing data")
                onClicked: historyPage.openClearData()
            }
        }

        VerticalScrollDecorator {}
    }
}
