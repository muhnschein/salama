// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The cover: what it shows on the home screen, the lightning or the last tab
// (docs/DECISIONS/0031-cover-is-lightning.md, 0028-settings-pages.md), and the one quick
// action it offers there (0029-quick-action.md).
//
// The action is one choice among six, made in a row drawn as Silica draws a ComboBox,
// under a line saying why there is one and two pictures of the cover with it. Choosing a
// bookmark asks which, and the action is that bookmark's only once one is picked: backing
// out leaves it as it was. The row then names the bookmark as it is called now, and under
// it are the glyphs the action can wear.
//
// Nothing here needs saving: each control writes its setting as it changes, and the
// cover (cover/CoverPage.qml) follows the settings.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

Page {
    id: coverSettingsPage

    // The glyph the action wears, as the cover picks it: what it opens, and for one
    // bookmark the glyph chosen for it here; none for no action.
    readonly property string glyph: ["", "search", "bookmarks", CoverSettings.quickActionIcon,
                                     "downloads", "history"][CoverSettings.quickAction]
    // Whether the ambience is dark, which decides the ink the cover's pictures are drawn
    // in, worked out as the cover works it out.
    readonly property bool onDark: {
        var ink = Theme.primaryColor
        return 0.299 * ink.r + 0.587 * ink.g + 0.114 * ink.b > 0.5
    }
    // The action's bookmark as it is now: by its id, or by its address once the id is
    // gone -- the menu's Bookmark takes one away and adds it back under another, and the
    // window points the setting at that (harbour-salama.qml). 0 when neither finds one.
    // The revision is read so that both are asked again as the bookmarks change.
    readonly property int bookmarkId: (BookmarkModel.revision,
                                       BookmarkModel.hasBookmark(CoverSettings.quickActionBookmark)
                                       ? CoverSettings.quickActionBookmark
                                       : BookmarkModel.idForUrl(CoverSettings.quickActionBookmarkUrl))
    readonly property string bookmarkTitle: (BookmarkModel.revision,
                                             BookmarkModel.titleOf(bookmarkId))

    function kindLabel(action) {
        return [
            //: The cover has no quick action
            qsTr("None"),
            //: A quick action on the cover: the address bar, opened for a new tab
            qsTr("Search"),
            //: A quick action on the cover: the list of bookmarks
            qsTr("Bookmarks"),
            //: A quick action on the cover: one bookmark's page, picked on the next page
            qsTr("Open a bookmark"),
            //: A quick action on the cover: the list of downloads
            qsTr("Downloads"),
            //: A quick action on the cover: the history
            qsTr("History")
        ][action]
    }

    // What the row says the action does now: a bookmark's by its title, as it is called
    // now, and a bookmark gone for good as gone -- the cover keeps offering it, and it
    // opens the bookmarks then.
    function valueText(action, bookmarkId, bookmarkTitle) {
        if (action !== CoverSettings.QuickActionBookmark) {
            return kindLabel(action)
        }
        if (bookmarkId > 0) {
            //: The cover's quick action opens this bookmark
            return qsTr("Bookmark: %1").arg(bookmarkTitle)
        }
        //: The cover's quick action opens a bookmark that has since been deleted
        return qsTr("Deleted bookmark")
    }

    function choose(action) {
        if (action === CoverSettings.QuickActionBookmark) {
            pickBookmark()
        } else {
            CoverSettings.quickAction = action
        }
    }

    // Which bookmark. The choice is written when one is picked, and not before.
    function pickBookmark() {
        var picker = pageStack.push(Qt.resolvedUrl("BookmarkPickerPage.qml"))
        if (!picker) {
            return
        }
        picker.bookmarkPicked.connect(function (bookmarkId, url, title) {
            CoverSettings.setQuickActionBookmark(bookmarkId, url, title)
            CoverSettings.quickAction = CoverSettings.QuickActionBookmark
        })
    }

    // A glyph's picture as the cover hands it to the home screen, as a whole URL.
    function iconSource(name) {
        return Qt.resolvedUrl("../../" + CoverSettings.iconPath(name, Theme.iconSizeSmall,
                                                                 coverSettingsPage.onDark))
    }

    objectName: "coverSettingsPage"
    allowedOrientations: Orientation.Portrait

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column

            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: qsTr("Cover")
            }

            // The default first, and the index is the stored value -- CoverSettings.Lightning,
            // CoverLatestTab (docs/DECISIONS/0031-cover-is-lightning.md). A combo rather
            // than a switch: which of two covers it is, not something turned on or off.
            ComboBox {
                objectName: "coverStyleCombo"
                width: parent.width
                label: qsTr("Shows")
                currentIndex: CoverSettings.style
                menu: ContextMenu {
                    MenuItem {
                        objectName: "coverLightningItem"
                        text: qsTr("Lightning")
                    }

                    MenuItem {
                        objectName: "coverLatestTabItem"
                        text: qsTr("The tab count and the last tab")
                    }
                }
                onCurrentIndexChanged: CoverSettings.style = currentIndex
            }

            SectionHeader {
                //: The one action offered on the cover on the home screen
                text: qsTr("Quick action")
            }

            Label {
                objectName: "quickActionExplained"
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                textFormat: Text.PlainText
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryHighlightColor
                //: The cover is the app's picture on the Sailfish home screen while it runs
                //: in the background; a quick action is an icon on it that a tap does
                //: something with. Use the same word for "quick action" as the section
                //: over this.
                text: qsTr("The cover on the home screen shows one quick action. The place beside it is kept for the media control, which appears there while the tab in front plays something.")
            }

            // The cover twice, side by side and centred: with nothing playing, and while a
            // tab plays. Two thirds of a real cover across, so that the choice under them is
            // on the screen with them and the glyphs are still told apart. By bindings
            // rather than a Row, which would place them only once it had been polished.
            Item {
                id: previews

                readonly property real between: 2 * Theme.paddingLarge
                readonly property real tileWidth: Math.min(Theme.coverSizeLarge.width * 2 / 3,
                                                           (width - between) / 2
                                                           - Theme.horizontalPageMargin)

                objectName: "quickActionPreviews"
                width: parent.width
                height: Math.max(quietPreview.height, playingPreview.height)

                QuickActionPreview {
                    id: quietPreview

                    objectName: "quietCoverPreview"
                    x: previews.width / 2 - previews.between / 2 - width
                    width: previews.tileWidth
                    glyph: coverSettingsPage.glyph
                    onDark: coverSettingsPage.onDark
                    //: Under a picture of the cover and its quick action while nothing
                    //: plays
                    text: qsTr("Nothing playing")
                }

                QuickActionPreview {
                    id: playingPreview

                    objectName: "playingCoverPreview"
                    x: previews.width / 2 + previews.between / 2
                    width: previews.tileWidth
                    glyph: coverSettingsPage.glyph
                    onDark: coverSettingsPage.onDark
                    playing: true
                    //: Under a picture of the cover while a tab plays: its quick action,
                    //: and the tab's mute beside it
                    text: qsTr("While a tab plays")
                }
            }

            // The choice: what the action is called and what it does now, drawn the way
            // Silica draws a ComboBox, with the kinds in a menu that opens under it. Not a
            // ComboBox: its value is the words of the item last tapped, so "Open a
            // bookmark" would read as chosen before a bookmark was, and after the picker
            // was backed out of, where this row names the bookmark once there is one; and
            // Silica puts a ComboBox's choices on a page of their own past a handful,
            // where a row's menu opens where it is.
            ListItem {
                id: choice

                objectName: "quickActionChoice"
                width: parent.width
                contentHeight: Theme.itemSizeSmall
                onClicked: openMenu()
                menu: ContextMenu {
                    MenuItem {
                        objectName: "quickAction-none"
                        text: coverSettingsPage.kindLabel(CoverSettings.QuickActionNone)
                        onClicked: coverSettingsPage.choose(CoverSettings.QuickActionNone)
                    }

                    MenuItem {
                        objectName: "quickAction-search"
                        text: coverSettingsPage.kindLabel(CoverSettings.QuickActionSearch)
                        onClicked: coverSettingsPage.choose(CoverSettings.QuickActionSearch)
                    }

                    MenuItem {
                        objectName: "quickAction-bookmarks"
                        text: coverSettingsPage.kindLabel(CoverSettings.QuickActionBookmarks)
                        onClicked: coverSettingsPage.choose(CoverSettings.QuickActionBookmarks)
                    }

                    MenuItem {
                        objectName: "quickAction-bookmark"
                        text: coverSettingsPage.kindLabel(CoverSettings.QuickActionBookmark)
                        onClicked: coverSettingsPage.choose(CoverSettings.QuickActionBookmark)
                    }

                    MenuItem {
                        objectName: "quickAction-downloads"
                        text: coverSettingsPage.kindLabel(CoverSettings.QuickActionDownloads)
                        onClicked: coverSettingsPage.choose(CoverSettings.QuickActionDownloads)
                    }

                    MenuItem {
                        objectName: "quickAction-history"
                        text: coverSettingsPage.kindLabel(CoverSettings.QuickActionHistory)
                        onClicked: coverSettingsPage.choose(CoverSettings.QuickActionHistory)
                    }
                }

                Label {
                    id: choiceLabel

                    objectName: "quickActionLabel"
                    x: Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter
                    color: choice.highlighted ? Theme.highlightColor : Theme.primaryColor
                    //: What the cover's quick action does, over the choice of it
                    text: qsTr("Action")
                }

                Label {
                    objectName: "quickActionValue"
                    anchors {
                        left: choiceLabel.right
                        leftMargin: Theme.paddingMedium
                        right: parent.right
                        rightMargin: Theme.horizontalPageMargin
                        verticalCenter: parent.verticalCenter
                    }
                    textFormat: Text.PlainText
                    truncationMode: TruncationMode.Fade
                    color: Theme.highlightColor
                    text: coverSettingsPage.valueText(CoverSettings.quickAction,
                                                      coverSettingsPage.bookmarkId,
                                                      coverSettingsPage.bookmarkTitle)
                }
            }

            // The glyphs a bookmark's action can wear, the one it wears lit, in as few rows
            // as the page's width allows, each as full as the next. Drawn from the files
            // the cover hands the home screen, so what is picked here is what is seen
            // there.
            Grid {
                id: icons

                // How many cells a row has room for, and how many glyphs there are.
                readonly property int room: Math.max(1, Math.floor((parent.width
                                                                    - 2 * Theme.horizontalPageMargin)
                                                                   / Theme.itemSizeSmall))
                readonly property int glyphs: CoverSettings.quickActionIcons.length

                objectName: "quickActionIcons"
                anchors.horizontalCenter: parent.horizontalCenter
                visible: CoverSettings.quickAction === CoverSettings.QuickActionBookmark
                columns: Math.ceil(glyphs / Math.ceil(glyphs / room))

                Repeater {
                    model: CoverSettings.quickActionIcons

                    BackgroundItem {
                        objectName: "quickActionIcon-" + modelData
                        width: Theme.itemSizeSmall
                        height: width
                        highlighted: down || CoverSettings.quickActionIcon === modelData
                        onClicked: CoverSettings.quickActionIcon = modelData

                        Image {
                            objectName: "quickActionIconImage"
                            anchors.centerIn: parent
                            width: Theme.iconSizeSmall
                            height: width
                            source: coverSettingsPage.iconSource(modelData)
                        }
                    }
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
