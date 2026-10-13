// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
// Start page is home page. Sections stay switchable (dimmed) while blank.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

Page {
    id: startPageSettings

    property SettingNames names: SettingNames {}
    // Landscape: the choices on the left, the picture on the right.
    readonly property real columnWidth: isLandscape ? width / 2 : width

    objectName: "startPageSettingsPage"
    allowedOrientations: Orientation.Portrait | Orientation.LandscapeMask

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + 2 * Theme.paddingLarge

        Column {
            id: column

            width: parent.width

            PageHeader {
                title: qsTr("Start page")
            }

            Flow {
                width: parent.width

                Column {
                    width: startPageSettings.columnWidth

                    TextSwitch {
                        objectName: "startPageSitesSwitch"
                        automaticCheck: false
                        text: startPageSettings.names.startPage(false)
                        checked: !StartPageSettings.blank
                        onClicked: StartPageSettings.blank = false
                    }

                    TextSwitch {
                        objectName: "startPageBlankSwitch"
                        automaticCheck: false
                        text: startPageSettings.names.startPage(true)
                        checked: StartPageSettings.blank
                        onClicked: StartPageSettings.blank = true
                    }

                    SectionHeader {
                        //: The parts of the start page, each switched on or off
                        text: qsTr("Sections")
                    }

                    TextSwitch {
                        objectName: "startPageTopSitesSwitch"
                        enabled: !StartPageSettings.blank
                        text: qsTr("Frequently visited")
                        checked: StartPageSettings.topSites
                        onCheckedChanged: StartPageSettings.topSites = checked
                    }

                    // Shown while frequently visited is on. Index = rows - 1.
                    ComboBox {
                        objectName: "startPageTopSiteRowsCombo"
                        width: parent.width
                        visible: StartPageSettings.topSites
                        enabled: !StartPageSettings.blank
                        //: How many rows of frequently visited sites the start page shows
                        label: qsTr("Rows")
                        currentIndex: StartPageSettings.topSiteRows - 1
                        menu: ContextMenu {
                            MenuItem {
                                text: Number(1).toLocaleString(Qt.locale(), "f", 0)
                            }

                            MenuItem {
                                text: Number(2).toLocaleString(Qt.locale(), "f", 0)
                            }
                        }
                        onCurrentIndexChanged: StartPageSettings.topSiteRows = currentIndex + 1
                    }

                    TextSwitch {
                        objectName: "startPageBookmarksSwitch"
                        enabled: !StartPageSettings.blank
                        text: qsTr("Bookmarks")
                        checked: StartPageSettings.bookmarks
                        onCheckedChanged: StartPageSettings.bookmarks = checked
                    }

                    TextSwitch {
                        objectName: "startPageRecentSwitch"
                        enabled: !StartPageSettings.blank
                        text: qsTr("Recently visited")
                        checked: StartPageSettings.recent
                        onCheckedChanged: StartPageSettings.recent = checked
                    }
                }

                Column {
                    width: startPageSettings.columnWidth

                    SectionHeader {
                        //: Over a picture of what a new tab will show
                        text: qsTr("Preview")
                    }

                    Item {
                        width: parent.width
                        height: Theme.paddingLarge
                    }

                    StartPagePreview {
                        width: parent.width
                        screenWidth: startPageSettings.width
                        screenHeight: startPageSettings.height
                    }
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
