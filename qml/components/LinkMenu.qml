// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// What a press held on a link or a picture brings up (docs/DECISIONS/0046-link-menu.md):
// the menu's own sheet, coming up from the foot of the screen over a dimmed page, on the
// same ground and pulled down the same way (0021-menu-sheet.md, BrowserMenu.qml). Its
// head names what was pressed and copies it; under the head, for a link to a page, a
// preview of that page, as Safari's; then the link's actions on discs -- New tab,
// Background tab, Share, Save link -- or, for a link another application takes, that
// application's -- Write email, Call, Send message, Show on map -- and Share; and after a
// line, for a picture, Open image, Save image and Copy image link. A picture is lifted
// out of the page above the sheet, where two fingers pinch it closer. A tap on an action
// does it and puts the sheet away; a tap outside it, or a pull down, puts it away alone.
import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Share 1.0
import harbour.salama 1.0

DockedPanel {
    id: menu

    // What was pressed: EngineMessages.linkTarget()'s map, every key there.
    property var target: ({
                              "link": "",
                              "kind": "",
                              "scheme": "",
                              "title": "",
                              "image": "",
                              "contentType": "",
                              "address": ""
                          })
    // The view of the page it was pressed on.
    property Item view: null
    // The browsing page's view for a preview, which only that page can make.
    property Component previewView
    // Where the page begins under the cutout: the room above the sheet starts there.
    property real topInset: 0

    readonly property bool hasLink: target.link.length > 0
    readonly property bool opensPage: target.kind === "page"
    readonly property bool forApp: target.kind === "app"
    readonly property bool hasImage: target.image.length > 0

    // A preview is offered for a link to a page, but not for a picture, which is lifted
    // out of the page instead, and not while the page in front plays: it is hidden while
    // the preview is drawn, and the engine pauses what a hidden page plays. Wanted while
    // it is offered and Settings.linkPreview says so; shown once there is a still of the
    // page to put in its place, which the browsing page then hides (LinkPreview.qml).
    readonly property bool previewOffered: opensPage && !hasImage
                                           && TabModel.activeMediaState !== TabModel.MediaPlaying
    readonly property bool previewWanted: open && previewOffered && Settings.linkPreview
                                          && view !== null
    property var pageStill: null
    readonly property bool previewShown: previewWanted && pageStill !== null

    // How far a pull has the sheet down, and how far puts it away, as the menu's.
    readonly property real pull: 2 * sheet.overscroll
    readonly property real closeDistance: Math.min(height / 3, Theme.itemSizeLarge)

    // Open an address where the page was, or in a new tab in front.
    signal openRequested(string url, bool inNewTab)
    // A tab was opened behind the one in front, under this name.
    signal openedBehind(int tabId, string title)

    objectName: "linkMenu"
    width: parent.width
    height: content.height
    dock: Dock.Bottom
    modal: true
    // Over the overlay it lays above itself.
    z: 2

    // What a press on a page says, for that page.
    function openFor(pressed, page) {
        pageStill = null
        target = pressed
        view = page
        show()
    }

    // The theme's icon for the application a link is for.
    function appIcon(scheme) {
        if (scheme === "mailto") {
            return "image://theme/icon-m-mail"
        }
        if (scheme === "tel") {
            return "image://theme/icon-m-call"
        }
        if (scheme === "sms") {
            return "image://theme/icon-m-sms"
        }
        return scheme === "geo" ? "image://theme/icon-m-location" : ""
    }

    function appAction(scheme) {
        if (scheme === "mailto") {
            //: The link sheet's action for an email address: the mail app, writing to it
            return qsTr("Write email")
        }
        if (scheme === "tel") {
            //: The link sheet's action for a phone number
            return qsTr("Call")
        }
        if (scheme === "sms") {
            //: The link sheet's action for a phone number to text
            return qsTr("Send message")
        }
        //: The link sheet's action for a place: the maps app, showing it
        return qsTr("Show on map")
    }

    function openBehind() {
        hide()
        var title = target.title.length > 0 ? target.title : target.address
        var tabId = TabModel.newTabBehind(target.link, target.title)
        if (tabId > 0) {
            openedBehind(tabId, title)
        }
    }

    function share() {
        hide()
        shareAction.resources = [{
                                     "type": "text/x-url",
                                     "linkTitle": target.title,
                                     "status": target.link
                                 }]
        shareAction.trigger()
    }

    function save(url, contentType) {
        hide()
        DownloadModel.save(url, contentType)
    }

    function copy(text, said) {
        hide()
        Clipboard.text = text
        copiedNotice.text = said
        copiedNotice.show()
    }

    // The head's copy button: a link as it is, the mailbox or the number another
    // application's link is for, or a picture's address.
    function copyPressed() {
        if (forApp) {
            //: Shown for a moment once the link sheet has put an email address, a phone
            //: number or a place on the clipboard
            copy(target.address, qsTr("Copied"))
        } else if (hasLink) {
            //: Shown for a moment once the link sheet has put a link on the clipboard
            copy(target.link, qsTr("Link copied"))
        } else {
            copy(target.image, imageCopied())
        }
    }

    function imageCopied() {
        //: Shown for a moment once the link sheet has put a picture's address on the clipboard
        return qsTr("Image link copied")
    }

    // A still of the page in front, for the preview to stand on: the page is hidden while
    // the preview is drawn. An answer for a page left meanwhile is dropped.
    function takeStill() {
        var page = view
        page.grabToImage(function (result) {
            if (menu.previewWanted && menu.view === page) {
                var at = page.mapToItem(overlay, 0, 0)
                overlay.stillRect = Qt.rect(at.x, at.y, page.width, page.height)
                menu.pageStill = result
            }
        }, Qt.size(page.width, page.height))
    }

    onPreviewWantedChanged: {
        if (previewWanted) {
            takeStill()
        } else {
            pageStill = null
        }
    }

    onPullChanged: {
        if (open) {
            y = parent.height - height + pull
        }
    }

    // What the sheet offers is for the page it was pressed on.
    Connections {
        target: TabModel
        onActiveTabChanged: menu.hide()
    }

    LinkMenuOverlay {
        id: overlay

        parent: menu.parent
        anchors.fill: parent
        z: menu.z - 1
        shown: menu.open
        roomTop: menu.topInset
        roomBottom: parent.height - menu.height
        still: menu.pageStill
        stillShown: menu.previewShown
        picture: menu.target.image
    }

    ShareAction {
        id: shareAction

        objectName: "linkShareAction"
        mimeType: "text/x-url"
    }

    SheetBackground {
        anchors.fill: parent
    }

    // Says what was put on the clipboard, over the navigation bar, as the menu's does.
    Notice {
        id: copiedNotice

        objectName: "linkCopiedNotice"
        duration: Notice.Short
        verticalOffset: -Theme.itemSizeLarge
    }

    SilicaFlickable {
        id: sheet

        readonly property real overscroll: Math.max(0, originY - contentY)

        objectName: "linkMenuSheet"
        width: parent.width
        height: parent.height
        y: -overscroll
        contentHeight: content.height
        flickableDirection: Flickable.VerticalFlick
        boundsBehavior: Flickable.DragOverBounds
        onDragEnded: {
            if (menu.pull > menu.closeDistance) {
                menu.hide()
            }
        }

        Column {
            id: content

            width: parent.width
            bottomPadding: Theme.paddingMedium

            Item {
                width: parent.width
                height: Theme.paddingLarge

                DragHandle {
                    objectName: "linkMenuDragHandle"
                    x: (parent.width - width) / 2
                    y: Theme.paddingSmall
                }
            }

            LinkMenuHeader {
                width: parent.width
                target: menu.target
                appIcon: menu.forApp ? menu.appIcon(menu.target.scheme) : ""
                onCopyRequested: menu.copyPressed()
            }

            LinkPreview {
                width: parent.width
                offered: menu.previewOffered
                shown: menu.previewShown
                pageView: menu.previewView
                frameHeight: Math.round(menu.parent.height / 3)
                onOpenRequested: {
                    menu.hide()
                    menu.openRequested(menu.target.link, false)
                }
            }

            LinkActions {
                width: parent.width
                sheet: menu
            }
        }
    }
}
