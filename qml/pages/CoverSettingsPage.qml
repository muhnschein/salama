// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The cover: the one quick action it offers on the home screen (docs/DECISIONS/
// 0029-quick-action.md, 0028-settings-pages.md), under a picture of the cover with the
// action on it (0037-cover-is-where-you-were.md). What the cover shows is not a choice:
// it says what the browser has to say.
//
// The action is one of six rows under a line saying why there is one, each with the glyph
// it wears on the cover, the one set lit -- every choice on the screen at once, where one
// row's menu hid them. Choosing a bookmark asks which, and the action is that bookmark's
// only once one is picked: backing out leaves it as it was. Its row then names the
// bookmark as it is called now, and under the rows are the glyphs the action can wear.
//
// Nothing here needs saving: each control writes its setting as it changes, and the
// cover (cover/CoverPage.qml) follows the settings.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

Page {
    id: coverSettingsPage

    property SettingNames names: SettingNames {}
    // The glyph the action wears, as the cover picks it: what it opens, and for one
    // bookmark the glyph chosen for it here; none for no action.
    readonly property string glyph: glyphOf(CoverSettings.quickAction)
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
    // The bookmark's row names it once there is one picked, whether or not the action is
    // that bookmark's now.
    readonly property bool bookmarkPicked: CoverSettings.quickActionBookmark > 0
                                           || CoverSettings.quickActionBookmarkUrl.length > 0

    function glyphOf(action) {
        return ["", "search", "bookmarks", CoverSettings.quickActionIcon, "downloads",
                "history"][action] || ""
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

            PageHeader {
                title: qsTr("Cover")
            }

            // The cover, centred, two thirds of a real one across, so the rows under it
            // start on the screen with it and the glyph on it is still told apart.
            QuickActionPreview {
                objectName: "coverPreview"
                x: (parent.width - width) / 2
                width: Math.min(Theme.coverSizeLarge.width * 2 / 3,
                                parent.width - 2 * Theme.horizontalPageMargin)
                glyph: coverSettingsPage.glyph
                onDark: coverSettingsPage.onDark
            }

            SectionHeader {
                //: The one action offered on the cover on the home screen
                text: qsTr("Quick action")
            }

            Label {
                objectName: "quickActionExplained"
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                bottomPadding: Theme.paddingMedium
                wrapMode: Text.Wrap
                textFormat: Text.PlainText
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryHighlightColor
                //: The cover is the app's picture on the Sailfish home screen while it runs
                //: in the background; a quick action is an icon on it that a tap does
                //: something with. The mute button is the playing tab's own.
                text: qsTr("The cover on the home screen offers one action. While a tab plays, its mute button sits beside it.")
            }

            // The six, in the stored order, each with the glyph it wears on the cover; the
            // one set lit, as Silica lights a chosen item.
            Repeater {
                model: [
                    { "action": CoverSettings.QuickActionNone, "key": "none" },
                    { "action": CoverSettings.QuickActionSearch, "key": "search" },
                    { "action": CoverSettings.QuickActionBookmarks, "key": "bookmarks" },
                    { "action": CoverSettings.QuickActionBookmark, "key": "bookmark" },
                    { "action": CoverSettings.QuickActionDownloads, "key": "downloads" },
                    { "action": CoverSettings.QuickActionHistory, "key": "history" }
                ]

                BackgroundItem {
                    id: actionRow

                    readonly property int action: modelData.action
                    readonly property bool chosen: CoverSettings.quickAction === action
                    readonly property bool lit: chosen || highlighted
                    readonly property string rowGlyph: coverSettingsPage.glyphOf(action)
                    readonly property string detail: action === CoverSettings.QuickActionBookmark
                                                     && coverSettingsPage.bookmarkPicked
                                                     ? coverSettingsPage.names.quickActionValue(
                                                           action, coverSettingsPage.bookmarkId,
                                                           coverSettingsPage.bookmarkTitle)
                                                     : ""

                    objectName: "quickAction-" + modelData.key
                    width: column.width
                    height: detail.length > 0 ? Theme.itemSizeMedium : Theme.itemSizeSmall
                    onClicked: coverSettingsPage.choose(action)

                    Item {
                        id: glyphSlot

                        x: Theme.horizontalPageMargin
                        width: Theme.iconSizeMedium
                        height: parent.height

                        Icon {
                            objectName: "quickActionGlyph"
                            anchors.centerIn: parent
                            visible: actionRow.rowGlyph.length > 0
                            width: Theme.iconSizeSmall
                            height: width
                            source: actionRow.rowGlyph.length > 0
                                    ? coverSettingsPage.iconSource(actionRow.rowGlyph) : ""
                            highlighted: actionRow.lit
                        }

                        // No action wears nothing: a dot keeps its place.
                        Rectangle {
                            anchors.centerIn: parent
                            visible: actionRow.rowGlyph.length === 0
                            width: Theme.paddingSmall
                            height: width
                            radius: width / 2
                            color: actionRow.lit ? Theme.highlightColor : Theme.primaryColor
                        }
                    }

                    Column {
                        anchors {
                            left: glyphSlot.right
                            right: parent.right
                            leftMargin: Theme.paddingMedium
                            rightMargin: Theme.horizontalPageMargin
                            verticalCenter: parent.verticalCenter
                        }

                        Label {
                            objectName: "quickActionName"
                            width: parent.width
                            text: coverSettingsPage.names.quickAction(actionRow.action)
                            truncationMode: TruncationMode.Fade
                            color: actionRow.lit ? Theme.highlightColor : Theme.primaryColor
                        }

                        Label {
                            objectName: "quickActionDetail"
                            width: parent.width
                            visible: text.length > 0
                            text: actionRow.detail
                            textFormat: Text.PlainText
                            truncationMode: TruncationMode.Fade
                            font.pixelSize: Theme.fontSizeExtraSmall
                            color: actionRow.lit ? Theme.secondaryHighlightColor
                                                 : Theme.secondaryColor
                        }
                    }
                }
            }

            // The glyphs a bookmark's action can wear, the one it wears lit, in as few rows
            // as the page's width allows, each as full as the next. Drawn from the files
            // the cover hands the home screen, so what is picked here is what is seen
            // there.
            Column {
                objectName: "quickActionIconPicker"
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                visible: CoverSettings.quickAction === CoverSettings.QuickActionBookmark
                topPadding: Theme.paddingMedium
                spacing: Theme.paddingSmall

                Label {
                    width: parent.width
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryColor
                    //: Over the glyphs a bookmark's quick action can wear on the cover
                    text: qsTr("Its icon on the cover")
                }

                Grid {
                    id: icons

                    // How many cells a row has room for, and how many glyphs there are.
                    readonly property int room: Math.max(1, Math.floor(parent.width
                                                                       / Theme.itemSizeSmall))
                    readonly property int glyphs: CoverSettings.quickActionIcons.length

                    objectName: "quickActionIcons"
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
        }

        VerticalScrollDecorator {}
    }
}
