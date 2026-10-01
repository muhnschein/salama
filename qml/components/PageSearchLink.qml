// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Between one page and SearchEngines: a page that says it has a search of its own --
// <link rel="search"> to an OpenSearch description -- is heard here, and its offer is
// kept, to be taken up from Settings > Search or never (docs/DECISIONS/0041-search-engines-found.md).
// sailfish-browser does the same for every page that is not private, and adds the engine
// at once (apps/shared/WebView.qml, "Link:AddSearch"); here nothing is added unasked.
//
// Not an Item: it lives inside the engine's view, which draws what it has itself.
import QtQuick 2.6
import harbour.salama 1.0

QtObject {
    id: link

    // The engine's view of the page.
    property Item view

    // A Connections takes every handler in it for its target's, so it is held here
    // rather than being this object.
    property Connections page: Connections {
        target: link.view
        onRecvAsyncMessage: {
            if (message === EngineMessages.searchOfferedMessage) {
                var offer = EngineMessages.searchOffered(data)
                SearchEngines.offerEngine(offer.title, offer.href, offer.host)
            }
        }
    }

    // What the page says is heard only once asked for.
    Component.onCompleted: view.addMessageListener(EngineMessages.searchOfferedMessage)
}
