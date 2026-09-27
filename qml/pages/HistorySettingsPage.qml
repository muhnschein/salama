// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// History: whether the pages visited are kept, whether they go as the browser closes,
// what browsing has left on the phone, and the way to clear it -- Firefox's History
// settings, with its Remember browsing and download history, Clear history when Firefox
// closes and Clear History… (docs/DECISIONS/0030-history-settings.md). What is kept is
// counted as Silica lays out details, a label and a value to a line, so that what the
// button under it clears is known before it is pressed.
//
// The clearing is done here, not in its dialog, which only asks: what the dialog is
// accepted with goes under one remorse on this page -- the page on the screen once the
// dialog has gone -- and each kind is then cleared as it always has been. That is why
// this page, and no other settings page, imports Sailfish.WebEngine: the engine is
// told to clear its cookies, site data and cache by its own notification.
import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.WebEngine 1.0
import harbour.salama 1.0

Page {
    id: historyPage

    objectName: "historySettingsPage"
    allowedOrientations: Orientation.Portrait

    // What was chosen is handed in rather than read off the dialog when the remorse
    // runs out: the page stack does away with the dialog as it pops it. The history
    // takes with it what it reaches back to of the list of downloads, and when it goes
    // whole, the recently closed tabs; the range reaches no further than those, the
    // engine clearing all it keeps or nothing.
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

            // Off, what is already kept stays until it is cleared, as in Firefox.
            TextSwitch {
                objectName: "rememberHistorySwitch"
                text: qsTr("Remember browsing history")
                checked: PrivacySettings.rememberHistory
                onCheckedChanged: PrivacySettings.rememberHistory = checked
            }

            // The line says what goes with the history, which the name does not.
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

            // Under what it clears, and a Silica Button rather than a row: it does
            // something, after asking what, rather than going somewhere.
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
