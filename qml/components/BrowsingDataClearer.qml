// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
// Clear browsing data from History and Privacy and security alike. Clears on calling page,
// not in dialog: page stack destroys dialog on pop, so remorse runs on page that stays.
// Only settings QML importing Sailfish.WebEngine: engine clears cookies/site data/cache.
import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.WebEngine 1.0
import harbour.salama 1.0

QtObject {
    id: clearer

    // Page that stays while remorse runs.
    property Item page

    // Choice passed in: dialog gone before remorse ends. Engine clears all or nothing.
    function clear(range, tabs, history, siteData, cache) {
        Remorse.popupAction(clearer.page, qsTr("Clearing browsing data"), function () {
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

    function open() {
        var dialog = pageStack.push(Qt.resolvedUrl("../pages/ClearDataDialog.qml"))
        dialog.accepted.connect(function () {
            clearer.clear(dialog.range, dialog.clearTabs, dialog.clearHistory,
                          dialog.clearSiteData, dialog.clearCache)
        })
    }
}
