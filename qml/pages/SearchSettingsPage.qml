// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

Page {
    id: searchSettingsPage

    objectName: "searchSettingsPage"
    allowedOrientations: Orientation.Portrait | Orientation.LandscapeMask

    SearchEngineInstaller {
        id: installer

        objectName: "searchEngineInstaller"
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        // Falls back to first built-in if engine in use removed. Only shown when something to remove.
        PullDownMenu {
            objectName: "searchSettingsPulley"
            visible: SearchEngines.addedCount > 0 || SearchEngines.foundEngines.length > 0

            MenuItem {
                objectName: "removeAddedEnginesMenu"
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

            // Omnibar row sources; off lists and counts nothing.
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
