// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Share 1.0
import harbour.salama 1.0

DockedPanel {
    id: menu

    // EngineMessages.linkTarget() map, all keys.
    property var target: ({
                              "link": "",
                              "kind": "",
                              "scheme": "",
                              "title": "",
                              "image": "",
                              "contentType": "",
                              "address": ""
                          })
    property Item view: null
    // Only browsing page can make preview view.
    property Component previewView
    property real topInset: 0

    readonly property bool hasLink: target.link.length > 0
    readonly property bool opensPage: target.kind === "page"
    readonly property bool forApp: target.kind === "app"
    readonly property bool hasImage: target.image.length > 0

    // Not while front page plays: page hidden during preview and engine pauses hidden media.
    readonly property bool previewOffered: opensPage && !hasImage
                                           && TabModel.activeMediaState !== TabModel.MediaPlaying
    readonly property bool previewWanted: open && previewOffered && Settings.linkPreview
                                          && view !== null
    property var pageStill: null
    readonly property bool previewShown: previewWanted && pageStill !== null

    readonly property real pull: 2 * sheet.overscroll
    readonly property real closeDistance: Math.min(height / 3, Theme.itemSizeLarge)

    signal openRequested(string url, bool inNewTab)
    signal openedBehind(int tabId, string title)

    objectName: "linkMenu"
    width: parent.width
    height: content.height
    dock: Dock.Bottom
    modal: true
    z: 2

    function openFor(pressed, page) {
        pageStill = null
        target = pressed
        view = page
        show()
    }

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

    // Page hidden while preview draws. Result dropped if page changed meanwhile.
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

            SheetGrip {
                width: parent.width
                handleName: "linkMenuDragHandle"
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
