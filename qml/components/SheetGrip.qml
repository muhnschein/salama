// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Top strip of bottom sheets: handle centred in thin strip, half small padding under top edge.
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    property alias handleName: handle.objectName
    property alias active: handle.active

    height: handle.height + Theme.paddingSmall

    DragHandle {
        id: handle

        anchors.centerIn: parent
    }
}
