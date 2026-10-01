// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Takes up a search engine a site offered: fetches its OpenSearch description, hands the
// text to SearchSettings, which reads it and adds the engine, and says how it went in a
// Silica Notice, as sailfish-browser says a search was added (apps/browser/qml/pages/
// SettingsPage.qml). The fetch is QML's own XMLHttpRequest, which is Qt's network access
// and needs no module Harbour does not allow; the reading is C++'s (src/search/
// OpenSearch.h). The status line is not asked: a page that is not a description -- an
// error page, a login -- is no description whatever its status says, and reading is
// what decides that (docs/DECISIONS/0041-search-engines-found.md).
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Item {
    id: installer

    // The addresses being fetched: a second tap on an offer waits for the first rather
    // than fetching twice, and then saying that the engine could not be added because
    // the first tap added it.
    property var fetching: ({})

    function install(title, href) {
        if (fetching[href]) {
            return
        }
        fetching[href] = true
        var request = new XMLHttpRequest()
        request.onreadystatechange = function () {
            if (request.readyState !== XMLHttpRequest.DONE) {
                return
            }
            delete installer.fetching[href]
            var added = SearchSettings.addFoundEngine(href, request.responseText)
            notice.text = added ? qsTr("%1 search added").arg(title)
                                : qsTr("Could not add %1").arg(title)
            notice.show()
        }
        request.open("GET", href)
        request.send()
    }

    Notice {
        id: notice

        objectName: "searchEngineNotice"
        duration: Notice.Short
    }
}
