// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Keeps OpenSearch offers (<link rel="search">) for Settings > Search; never adds unasked.
import QtQuick 2.6
import harbour.salama 1.0

QtObject {
    id: link

    property Item view

    // Connections claims all handlers for its target, so held, not this object.
    property Connections page: Connections {
        target: link.view
        onRecvAsyncMessage: {
            if (message === EngineMessages.searchOfferedMessage) {
                var offer = EngineMessages.searchOffered(data)
                SearchEngines.offerEngine(offer.title, offer.href, offer.host)
            }
        }
    }

    // Messages heard only once listened for.
    Component.onCompleted: view.addMessageListener(EngineMessages.searchOfferedMessage)
}
