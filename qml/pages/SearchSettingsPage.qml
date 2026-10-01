// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Search: which engine the address bar searches with, and what on the phone it
// suggests from as something is typed -- the two Firefox for Android keeps on its own
// Search page (docs/DECISIONS/0028-settings-pages.md). The engines are few, so every one
// is on the screen at once, one tap to change: TextSwitches that do not check
// themselves, the one chosen lit. Besides the three that come with the browser are the
// ones added from what sites offered while they were browsed, which is what the
// section under them lists, as sailfish-browser does under "Tap to install"
// (apps/browser/qml/pages/SettingsPage.qml, docs/DECISIONS/0041-search-engines-found.md).
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

Page {
    id: searchSettingsPage

    objectName: "searchSettingsPage"
    allowedOrientations: Orientation.Portrait

    SearchEngineInstaller {
        id: installer

        objectName: "searchEngineInstaller"
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        // Every engine added while browsing and every offer, gone at once, after the
        // remorse a list kept for a while deserves; the first built-in engine searches
        // if the one in use was one of them.
        PullDownMenu {
            MenuItem {
                objectName: "removeAddedEnginesMenu"
                visible: SearchEngines.addedCount > 0 || SearchEngines.foundEngines.length > 0
                text: qsTr("Remove added search engines")
                onClicked: Remorse.popupAction(searchSettingsPage,
                                               qsTr("Removing added search engines"),
                                               function () {
                                                   SearchEngines.removeAddedEngines()
                                               })
            }
        }

        Column {
            id: column

            width: parent.width

            PageHeader {
                title: qsTr("Search")
            }

            SectionHeader {
                text: qsTr("Search engine")
            }

            Repeater {
                model: SearchEngines.engineNames

                SearchEngineChoice {
                    // The site an added engine came from; empty for a built-in one.
                    readonly property string host: SearchEngines.engineHosts[index] || ""

                    text: modelData
                    //: Under a search engine that was added while browsing. %1 is the site that offered it
                    description: host.length > 0 ? qsTr("Added from %1").arg(host) : ""
                    removable: host.length > 0
                    checked: SearchSettings.engineIndex === index
                    onChosen: SearchSettings.engineIndex = index
                    onRemoveRequested: SearchEngines.removeAddedEngine(SearchEngines.engineKeys[index])
                }
            }

            // What the sites browsed have offered and nothing has taken up.
            Column {
                objectName: "foundSearchEngines"
                width: parent.width
                visible: SearchEngines.foundEngines.length > 0

                SectionHeader {
                    text: qsTr("Found while browsing")
                }

                Label {
                    objectName: "foundSearchEnginesHint"
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * x
                    bottomPadding: Theme.paddingMedium
                    wrapMode: Text.Wrap
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryHighlightColor
                    text: qsTr("Sites can offer their search. Tap one to add it and search with it.")
                }

                Repeater {
                    model: SearchEngines.foundEngines

                    FoundSearchEngine {
                        title: modelData.title
                        host: modelData.host
                        onAddRequested: installer.install(modelData.title, modelData.href)
                        onForgetRequested: SearchEngines.forgetFoundEngine(modelData.href)
                    }
                }
            }

            // Where the rows the address bar lists come from, each on until it is
            // switched off here; one switched off lists nothing and counts nothing
            // (docs/DECISIONS/0027-omnibar.md). Firefox calls these "Address bar -
            // Firefox Suggest", which says more about Firefox than about the rows.
            SectionHeader {
                text: qsTr("Address bar suggestions")
            }

            TextSwitch {
                objectName: "omnibarTabsSwitch"
                text: qsTr("Open tabs")
                checked: SearchSettings.omnibarTabs
                onCheckedChanged: SearchSettings.omnibarTabs = checked
            }

            TextSwitch {
                objectName: "omnibarBookmarksSwitch"
                text: qsTr("Bookmarks")
                checked: SearchSettings.omnibarBookmarks
                onCheckedChanged: SearchSettings.omnibarBookmarks = checked
            }

            TextSwitch {
                objectName: "omnibarHistorySwitch"
                text: qsTr("History")
                checked: SearchSettings.omnibarHistory
                onCheckedChanged: SearchSettings.omnibarHistory = checked
            }

            TextSwitch {
                objectName: "omnibarDownloadsSwitch"
                text: qsTr("Downloads")
                checked: SearchSettings.omnibarDownloads
                onCheckedChanged: SearchSettings.omnibarDownloads = checked
            }
        }

        VerticalScrollDecorator {}
    }
}
