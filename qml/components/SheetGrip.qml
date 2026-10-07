// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Bottom sheet's handle: on its top edge, as on nav bar. Child of sheet itself, never of
// clipped list.
import QtQuick 2.6
import Sailfish.Silica 1.0

DragHandle {
    x: (parent.width - width) / 2
    y: -height / 2
    z: 1
}
