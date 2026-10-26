// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Bottom sheet's handle: centred on sheet's edge line, as on nav bar. Lives beside sheet, not
// in it: DockedPanel clips, which cut top half off. Upright: side sheet's, on its left edge.
import QtQuick 2.6
import Sailfish.Silica 1.0

DragHandle {
    property Item edgeOf
    property bool upright: false

    parent: edgeOf ? edgeOf.parent : null
    width: upright ? Theme.paddingSmall : Theme.itemSizeMedium
    height: upright ? Theme.itemSizeMedium : Theme.paddingSmall
    radius: Math.min(width, height) / 2
    x: upright ? edgeOf.x + Theme._lineWidth / 2 - width / 2
               : edgeOf.x + (edgeOf.width - width) / 2
    y: upright ? edgeOf.y + (edgeOf.height - height) / 2
               : edgeOf.y + Theme._lineWidth / 2 - height / 2
    z: edgeOf.z + 1
    // Shut sheet lies past parent's edge; half would peek out.
    visible: parent !== null && edgeOf.visible
             && (upright ? edgeOf.x < parent.width : edgeOf.y < parent.height)
}
