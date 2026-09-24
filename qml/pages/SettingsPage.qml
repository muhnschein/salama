// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.WebEngine 1.0
import harbour.salama 1.0

Page {
    id: settingsPage

    objectName: "settingsPage"
    allowedOrientations: Orientation.Portrait

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

            ComboBox {
                objectName: "searchEngineCombo"
                width: parent.width
                label: qsTr("Search engine")
                currentIndex: Settings.searchEngineIndex
                menu: ContextMenu {
                    Repeater {
                        model: Settings.searchEngineNames

                        MenuItem {
                            text: modelData
                        }
                    }
                }
                onCurrentIndexChanged: Settings.searchEngineIndex = currentIndex
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

            // How the reader view sets an article, as Firefox's reader view offers it:
            // its colours, typeface and text size, each index or value the stored one
            // (docs/DECISIONS/0024-reader-view.md). A reader view on the screen follows
            // them at once.
            SectionHeader {
                text: qsTr("Reader view")
            }

            ComboBox {
                objectName: "readerColorsCombo"
                width: parent.width
                label: qsTr("Colours")
                currentIndex: Settings.readerColors
                menu: ContextMenu {
                    MenuItem {
                        text: qsTr("Ambience")
                    }

                    MenuItem {
                        text: qsTr("Light")
                    }

                    MenuItem {
                        text: qsTr("Sepia")
                    }

                    MenuItem {
                        text: qsTr("Dark")
                    }
                }
                onCurrentIndexChanged: Settings.readerColors = currentIndex
            }

            ComboBox {
                objectName: "readerTypefaceCombo"
                width: parent.width
                label: qsTr("Typeface")
                currentIndex: Settings.readerTypeface
                menu: ContextMenu {
                    MenuItem {
                        text: qsTr("Sans serif")
                    }

                    MenuItem {
                        text: qsTr("Serif")
                    }
                }
                onCurrentIndexChanged: Settings.readerTypeface = currentIndex
            }

            // Firefox's nine steps, the middle one its default, written as a share of it.
            Slider {
                objectName: "readerTextSizeSlider"
                width: parent.width
                label: qsTr("Text size")
                minimumValue: Settings.ReaderTextSizeMin
                maximumValue: Settings.ReaderTextSizeMax
                stepSize: 1
                value: Settings.readerTextSize
                valueText: qsTr("%1 %").arg(Math.round(100 * (10 + 2 * value)
                                                       / (10 + 2 * Settings.ReaderTextSizeDefault)))
                onValueChanged: Settings.readerTextSize = Math.round(value)
            }

            SectionHeader {
                text: qsTr("Privacy")
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

            SectionHeader {
                text: qsTr("Cover")
            }

            // The order is how much the cover says, least first, and the index is the
            // stored value -- Settings.CoverIconOnly, CoverLatestTab, CoverEveryTab.
            // A combo rather than three switches: these are one choice, not three.
            ComboBox {
                objectName: "coverStyleCombo"
                width: parent.width
                label: qsTr("Shows")
                currentIndex: Settings.coverStyle
                menu: ContextMenu {
                    MenuItem {
                        objectName: "coverIconOnlyItem"
                        text: qsTr("The icon alone")
                    }

                    MenuItem {
                        objectName: "coverLatestTabItem"
                        text: qsTr("The tab count and the last tab")
                    }

                    // "Most recent" rather than "every": the field draws six cells at
                    // most, most recently read first, and the number above it is what
                    // says how many there are (docs/DECISIONS/0014-cover-is-the-tab-count.md).
                    MenuItem {
                        objectName: "coverEveryTabItem"
                        text: qsTr("The tab count and the most recent tabs")
                    }
                }
                onCurrentIndexChanged: Settings.coverStyle = currentIndex
            }

            SectionHeader {
                text: qsTr("Clear data")
            }

            Button {
                objectName: "clearHistoryButton"
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Clear history")
                onClicked: Remorse.popupAction(settingsPage, qsTr("Clearing history"), function () {
                    HistoryModel.clear()
                })
            }

            Button {
                objectName: "clearSiteDataButton"
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Clear cookies and site data")
                onClicked: Remorse.popupAction(settingsPage, qsTr("Clearing site data"), function () {
                    WebEngine.notifyObservers(EngineMessages.clearPrivateDataTopic,
                                              EngineMessages.cookiesAndSiteDataPayload)
                })
            }

            Button {
                objectName: "clearCacheButton"
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Clear cache")
                onClicked: Remorse.popupAction(settingsPage, qsTr("Clearing cache"), function () {
                    WebEngine.notifyObservers(EngineMessages.clearPrivateDataTopic,
                                              EngineMessages.cachePayload)
                })
            }

            Button {
                objectName: "closeAllTabsButton"
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Close all tabs")
                onClicked: Remorse.popupAction(settingsPage, qsTr("Closing all tabs"), function () {
                    TabModel.closeAllTabs()
                })
            }
        }

        VerticalScrollDecorator {}
    }
}
