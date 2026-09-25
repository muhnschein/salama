// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// History: whether the pages visited are kept, whether they go as the browser closes,
// and the way to clear what browsing leaves behind -- Firefox's History settings, with
// its Remember browsing and download history, Clear history when Firefox closes and
// Clear History… (docs/DECISIONS/0030-history-settings.md).
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
import "../components"

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
        contentHeight: column.height

        Column {
            id: column

            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: qsTr("History")
            }

            // Off, what is already kept stays until it is cleared, as in Firefox.
            TextSwitch {
                objectName: "rememberHistorySwitch"
                text: qsTr("Remember browsing history")
                description: qsTr("Keep the pages you visit, to find them in the history and the address bar")
                checked: Settings.rememberHistory
                onCheckedChanged: Settings.rememberHistory = checked
            }

            TextSwitch {
                objectName: "clearHistoryOnCloseSwitch"
                text: qsTr("Clear history when closed")
                description: qsTr("The history, the list of downloads and the recently closed tabs go each time the browser is closed")
                checked: Settings.clearHistoryOnClose
                onCheckedChanged: Settings.clearHistoryOnClose = checked
            }

            SettingsEntry {
                objectName: "clearDataEntry"
                // sailfish-browser's for its own Clear browsing data
                // (apps/browser/qml/pages/SettingsPage.qml:274).
                iconSource: "image://theme/icon-m-delete"
                text: qsTr("Clear browsing data")
                onClicked: historyPage.openClearData()
            }
        }

        VerticalScrollDecorator {}
    }
}
