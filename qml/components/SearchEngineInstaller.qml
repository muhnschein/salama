// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// XMLHttpRequest: Qt network, needs no Harbour-banned module. HTTP status ignored: parse
// decides (error/login pages aren't descriptions anyway).
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Item {
    id: installer

    // In-flight hrefs: second tap waits instead of double fetch and bogus failure notice.
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
            var added = SearchEngines.addFoundEngine(href, request.responseText)
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
