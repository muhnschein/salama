// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Bottom sheet's handle: on its top edge, as on nav bar. Lives beside sheet, not in it:
// DockedPanel clips, which cut top half off.
import QtQuick 2.6
import Sailfish.Silica 1.0

DragHandle {
    property Item edgeOf

    parent: edgeOf ? edgeOf.parent : null
    x: edgeOf.x + (edgeOf.width - width) / 2
    y: edgeOf.y - height / 2
    z: edgeOf.z + 1
    // Shut sheet lies past parent's bottom edge; top half would peek out.
    visible: parent !== null && edgeOf.visible && edgeOf.y < parent.height
}
