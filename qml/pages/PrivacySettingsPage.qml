// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Privacy: how much of the engine's own tracking protection is on, and the way to
// clear what browsing leaves behind, the two Firefox for Android keeps under Privacy
// and security (docs/DECISIONS/0028-settings-pages.md).
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
    id: privacyPage

    objectName: "privacySettingsPage"
    allowedOrientations: Orientation.Portrait

    // What was chosen is handed in rather than read off the dialog when the remorse
    // runs out: the page stack does away with the dialog as it pops it.
    function clearData(tabs, history, siteData, cache) {
        Remorse.popupAction(privacyPage, qsTr("Clearing browsing data"), function () {
            if (tabs) {
                TabModel.closeAllTabs()
            }
            if (history) {
                HistoryModel.clear()
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
            privacyPage.clearData(dialog.clearTabs, dialog.clearHistory, dialog.clearSiteData,
                                  dialog.clearCache)
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
                title: qsTr("Privacy")
            }

            // Firefox's tracking protection categories, least first, with Off in place
            // of Custom; the index is the stored value -- Settings.TrackingProtectionOff,
            // TrackingProtectionStandard, TrackingProtectionStrict
            // (docs/DECISIONS/0023-tracking-protection.md). The description promises
            // only what every engine this runs on does.
            ComboBox {
                objectName: "trackingProtectionCombo"
                width: parent.width
                label: qsTr("Tracking protection")
                description: currentIndex === Settings.TrackingProtectionOff
                             ? qsTr("Sites can follow you from one to another")
                             : currentIndex === Settings.TrackingProtectionStrict
                               ? qsTr("Stops more tracking, and can break some sites")
                               : qsTr("Stops sites following you with cookies")
                currentIndex: Settings.trackingProtection
                menu: ContextMenu {
                    MenuItem {
                        text: qsTr("Off")
                    }

                    MenuItem {
                        text: qsTr("Standard")
                    }

                    MenuItem {
                        text: qsTr("Strict")
                    }
                }
                onCurrentIndexChanged: Settings.trackingProtection = currentIndex
            }

            SettingsEntry {
                objectName: "clearDataEntry"
                // sailfish-browser's for its own Clear browsing data
                // (apps/browser/qml/pages/SettingsPage.qml:274).
                iconSource: "image://theme/icon-m-delete"
                text: qsTr("Clear browsing data")
                onClicked: privacyPage.openClearData()
            }
        }

        VerticalScrollDecorator {}
    }
}
