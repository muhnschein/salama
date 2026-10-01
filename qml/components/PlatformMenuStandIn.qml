// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// What the platform's WebView makes in place of its own menu for a press held on a link
// or a picture. Its popup opener opens one for every Content:ContextMenu the page sends
// (sailfish-components-webview import/popups/PopupOpener.qml), and the link sheet is this
// browser's answer to the same message (docs/DECISIONS/0046-link-menu.md): left alone,
// the two would come up together. PageLinkMenu tells the view's popup provider to make
// this instead, which takes what the opener gives a menu and shows nothing.
import QtQuick 2.6

Item {
    // What the opener sets on its menu, by the names it sets them by
    // (import/popups/ContextMenuInterface.qml).
    property string linkHref
    property string linkTitle
    property string linkProtocol
    property string imageSrc
    property string contentType
    property int viewId
    property bool downloadsEnabled
    property var pageStack
    property var tabModel
    // Whether the menu is up, which the opener reads: never.
    readonly property bool active: false

    objectName: "platformMenuStandIn"
    visible: false

    function show() {
    }
}
