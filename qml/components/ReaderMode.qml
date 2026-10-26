// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Reader page loaded as data: url, own history entry; original url recovered from it so
// tab, history and bar keep article address.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

QtObject {
    id: reader

    property Item view: null

    // Empty unless reader view on screen.
    property string source: ""
    readonly property bool active: source.length > 0
    property bool readerable: false
    property bool busy: false
    // Menu asked since document arrived; load end answers again.
    property bool asked: false

    // For Automatic and ambience look.
    readonly property var ambience: ({
        "primaryColor": Theme.primaryColor,
        "secondaryColor": Theme.secondaryColor,
        "highlightColor": Theme.highlightColor,
        "secondaryHighlightColor": Theme.secondaryHighlightColor,
        "highlightBackgroundColor": Theme.highlightBackgroundColor,
        "highlightDimmerColor": Theme.highlightDimmerColor,
        "overlayBackgroundColor": Theme.overlayBackgroundColor,
        "fontFamily": Theme.fontFamily,
        "fontFamilyHeading": Theme.fontFamilyHeading
    })

    onAmbienceChanged: restyle()

    // Returns url tab keeps. No check here: Readability check lays page out on Gecko's main
    // thread, ran on every address change and load end, only for menu's button.
    function follow(url) {
        source = Reader.sourceUrl(url)
        readerable = false
        busy = false
        asked = false
        return active ? source : String(url)
    }

    // Menu opening: once per document. Mid-load answer waits for load end.
    function ask() {
        if (asked) {
            return
        }
        asked = true
        if (view && !view.loading) {
            check()
        }
    }

    // Skips reader views and site front pages.
    function check() {
        var url = view ? String(view.url) : ""
        if (active || !Reader.checksUrl(url)) {
            return
        }
        view.runJavaScript(Reader.readerableScript, function (answer) {
            if (String(view.url) === url) {
                readerable = Reader.readerable(answer)
            }
        })
    }

    function toggle() {
        if (active) {
            close()
        } else {
            open()
        }
    }

    function open() {
        if (!view || busy || active) {
            return
        }
        var url = String(view.url)
        busy = true
        view.runJavaScript(Reader.articleScript, function (article) {
            busy = false
            if (String(view.url) !== url) {
                return
            }
            var html = Reader.page(article, url, TabModel.activeTitle, TabModel.activeFavicon,
                                   ambience)
            if (html.length > 0) {
                view.loadHtml(html)
            } else {
                readerable = false
            }
        }, function () {
            busy = false
            readerable = false
        })
    }

    // No history (restored tab, or unloaded past live-page limit) -> load source.
    function close() {
        if (!active) {
            return
        }
        if (view.canGoBack) {
            view.goBack()
        } else {
            view.open(source)
        }
    }

    // No reload; refetch cutout strip colour.
    function restyle() {
        if (!active || !view) {
            return
        }
        view.runJavaScript(Reader.styleScript(ambience), function () {
            view.fetchThemeColor()
        })
    }

    property Connections settings: Connections {
        target: Reader
        onStyleChanged: reader.restyle()
    }

    property Connections loads: Connections {
        target: reader.view
        onLoadingChanged: {
            if (!reader.view.loading && reader.asked) {
                reader.check()
            }
        }
    }
}
