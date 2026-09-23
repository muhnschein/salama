// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// One page's reader view: whether the page in the view reads as an article, and the
// way into its reader view and back out (docs/DECISIONS/0023-reader-view.md).
//
// As Firefox has it: Readability decides after each load whether the page is worth
// offering, and reading it is a page of its own in the view's history, so back leaves
// it. That page is a document Reader makes from what Readability finds, handed to the
// engine as HTML; the engine reports its address as the data: url it loaded, and the
// page it was made from is read back out of that, so the tab, its history and the bar
// keep the article's own address all the while.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

QtObject {
    id: reader

    // The engine's view whose page this is.
    property Item view: null

    // The page the reader view on the screen was made from, or empty while the view
    // shows anything else.
    property string source: ""
    readonly property bool active: source.length > 0
    // Whether the page in the view reads as an article, by Readability's measure.
    property bool readerable: false
    // An article being read out of the page.
    property bool busy: false

    // The ambience's own colours are light or dark as it is.
    readonly property bool darkAmbience: Reader.isDarkAmbience(Theme.primaryColor)

    onDarkAmbienceChanged: restyle()

    // The view is somewhere new: a reader view, or a page to be looked at afresh once it
    // has loaded. One that changes its address without loading is looked at now.
    function follow(url) {
        source = Reader.sourceUrl(url)
        readerable = false
        busy = false
        if (view && !view.loading) {
            check()
        }
    }

    // Whether the page is worth offering the reader view of, as Firefox asks after every
    // load: not a reader view itself, not a site's front page, and Readability's quick
    // look finding enough text.
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

    // Readability over the page, and what it finds loaded in its place. A page that
    // turns out to hold no article stops being offered.
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
                                   darkAmbience)
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

    // Back to the page, as Firefox goes back: the reader view was opened from it, so it
    // is the entry before. A view with nothing before it loads the page instead. That
    // is for safety: a tab restored, or given up past the limit of loaded pages, comes
    // back as the article's page, not as its reader view.
    function close() {
        if (!active) {
            return
        }
        if (view.canGoBack) {
            view.goBack()
        } else {
            view.url = source
        }
    }

    // A reader view on the screen follows the settings and the ambience as they change,
    // without being loaded again, and the strip beside the cutout with it.
    function restyle() {
        if (!active || !view) {
            return
        }
        view.runJavaScript(Reader.styleScript(darkAmbience), function () {
            view.fetchThemeColor()
        })
    }

    property Connections settings: Connections {
        target: Reader
        onStyleChanged: reader.restyle()
    }
}
