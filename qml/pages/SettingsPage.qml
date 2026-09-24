// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Settings: the few that take a line each, and a way to each subject that takes more --
// search, the reader view, the cover, privacy -- on a page of its own, in headed
// groups, as Firefox for Android arranges its settings and Jolla's own browser reaches
// its privacy settings (docs/DECISIONS/0028-settings-pages.md). Under each way in, a
// line says how that subject is set now, so this page is also where to read it.
//
// Every control writes its setting as it changes; nothing waits on a Save.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

Page {
    id: settingsPage

    objectName: "settingsPage"
    allowedOrientations: Orientation.Portrait

    function open(page) {
        pageStack.push(Qt.resolvedUrl(page))
    }

    // The summaries are in the words the subjects' pages offer their choices in, and
    // each is a binding on the settings it names: a change made on the subject's page
    // is already written here when that page is popped.
    function readerSummary(colors, typeface, textSize) {
        var colorNames = [qsTr("Ambience"), qsTr("Light"), qsTr("Sepia"), qsTr("Dark")]
        var typefaceNames = [qsTr("Sans serif"), qsTr("Serif")]
        // As the reader page's slider writes it: Firefox's middle size is the whole.
        var share = qsTr("%1 %").arg(Math.round(100 * (10 + 2 * textSize)
                                                / (10 + 2 * Settings.ReaderTextSizeDefault)))
        //: The reader view's look in one line: its colours, typeface and text size,
        //: e.g. "Ambience · Sans serif · 100 %"
        return qsTr("%1 · %2 · %3").arg(colorNames[colors]).arg(typefaceNames[typeface]).arg(share)
    }

    function privacySummary(level) {
        //: A tracking protection level
        var levelNames = [qsTr("Off"), qsTr("Standard"), qsTr("Strict")]
        return qsTr("Tracking protection: %1").arg(levelNames[level])
    }

    // What the cover shows, then its quick action (docs/DECISIONS/0029-quick-action.md).
    function coverSummary(style, action) {
        var styleNames = [qsTr("The icon alone"), qsTr("The tab count and the last tab"),
                          qsTr("The tab count and the most recent tabs")]
        var actionNames = [
            //: The cover has no quick action
            qsTr("No quick action"),
            // The Search entry's own word, which a translator sees once.
            qsTr("Search"),
            //: A quick action on the cover: the list of bookmarks
            qsTr("Bookmarks"),
            //: A quick action on the cover: one bookmark's page
            qsTr("Open a bookmark"),
            //: A quick action on the cover: the list of downloads
            qsTr("Downloads"),
            //: A quick action on the cover: the history
            qsTr("History")
        ]
        //: The cover's settings in one line: what it shows, then its quick action,
        //: e.g. "The icon alone · Search"
        return qsTr("%1 · %2").arg(styleNames[style]).arg(actionNames[action])
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column

            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: qsTr("Settings")
            }

            SectionHeader {
                text: qsTr("General")
            }

            // First, as Firefox for Android puts it: what the address bar searches with
            // and suggests from is what a browser is used for most.
            SettingsEntry {
                objectName: "searchSettingsEntry"
                // sailfish-browser's for its search engine
                // (apps/browser/qml/pages/SettingsPage.qml:117).
                iconSource: "image://theme/icon-m-search"
                text: qsTr("Search")
                summary: Settings.searchEngineNames[Settings.searchEngineIndex]
                onClicked: settingsPage.open("SearchSettingsPage.qml")
            }

            TextField {
                id: homePageField

                objectName: "homePageField"
                width: parent.width
                label: qsTr("Home page")
                placeholderText: label
                text: Settings.homePage
                inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase | Qt.ImhUrlCharactersOnly
                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: {
                    Settings.homePage = Settings.urlForInput(text)
                    focus = false
                }
            }

            TextSwitch {
                objectName: "desktopModeSwitch"
                text: qsTr("Request desktop sites")
                description: qsTr("Identify as a desktop browser to web sites")
                checked: Settings.desktopMode
                onCheckedChanged: Settings.desktopMode = checked
            }

            TextSwitch {
                objectName: "cutoutGuardSwitch"
                text: qsTr("Avoid the screen cutout")
                description: qsTr("Keep pages and the tab grid out from under the camera cutout")
                checked: Settings.cutoutGuard
                onCheckedChanged: Settings.cutoutGuard = checked
            }

            // The choices are Settings.liveTabLimitChoices, 0 standing for all of them.
            // Five is the default, and what the platform browser keeps.
            ComboBox {
                objectName: "liveTabLimitCombo"
                width: parent.width
                label: qsTr("Pages kept loaded")
                description: qsTr("Tabs beyond this many reload their page when opened again")
                currentIndex: Settings.liveTabLimitIndex
                menu: ContextMenu {
                    Repeater {
                        model: Settings.liveTabLimitChoices

                        MenuItem {
                            text: modelData > 0 ? modelData : qsTr("All")
                        }
                    }
                }
                onCurrentIndexChanged: Settings.liveTabLimitIndex = currentIndex
            }

            SectionHeader {
                text: qsTr("Appearance")
            }

            SettingsEntry {
                objectName: "readerSettingsEntry"
                // The menu's own for the reader view, which is Jolla's Documents' for a
                // text document (docs/DECISIONS/0024-reader-view.md).
                iconSource: "image://theme/icon-m-file-formatted"
                text: qsTr("Reader view")
                summary: settingsPage.readerSummary(Settings.readerColors, Settings.readerTypeface,
                                                    Settings.readerTextSize)
                onClicked: settingsPage.open("ReaderSettingsPage.qml")
            }

            SettingsEntry {
                objectName: "coverSettingsEntry"
                // The one sailfish-browser's toolbar writes the tab count into
                // (apps/browser/qml/pages/components/ToolBar.qml:162), which is what
                // the cover is (docs/DECISIONS/0014-cover-is-the-tab-count.md). Not
                // icon-m-display: sailfish-browser has that for its notch guard, which
                // is the screen cutout here.
                iconSource: "image://theme/icon-m-tabs"
                text: qsTr("Cover")
                summary: settingsPage.coverSummary(Settings.coverStyle, Settings.quickAction)
                onClicked: settingsPage.open("CoverSettingsPage.qml")
            }

            SectionHeader {
                text: qsTr("Privacy")
            }

            SettingsEntry {
                objectName: "privacySettingsEntry"
                // sailfish-browser's for a secure connection
                // (apps/browser/qml/pages/components/CertificateInfo.qml:65).
                iconSource: "image://theme/icon-m-device-lock"
                text: qsTr("Privacy")
                summary: settingsPage.privacySummary(Settings.trackingProtection)
                onClicked: settingsPage.open("PrivacySettingsPage.qml")
            }
        }

        VerticalScrollDecorator {}
    }
}
