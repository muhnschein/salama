// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Blank stand-in for platform WebView context menu, else it shows alongside link sheet
// (PopupOpener opens one per Content:ContextMenu).
import QtQuick 2.6

Item {
    // Names opener sets (import/popups/ContextMenuInterface.qml).
    property string linkHref
    property string linkTitle
    property string linkProtocol
    property string imageSrc
    property string contentType
    property int viewId
    property bool downloadsEnabled
    property var pageStack
    property var tabModel
    // Opener reads it.
    readonly property bool active: false

    objectName: "platformMenuStandIn"
    visible: false

    function show() {
    }
}
