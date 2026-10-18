// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Silica's ContextMenu, a ComboBox's too, spans its nearest Flickable from that one's left edge:
// in a column beside another it spread over both and went under the later one (device). This
// unmoving Flickable is that nearest one, so a menu keeps to its column. Drags still reach the
// page's own Flickable.
import QtQuick 2.6

Flickable {
    id: scope

    default property alias content: column.data

    height: column.height
    contentHeight: column.height
    interactive: false

    Column {
        id: column

        width: scope.width
    }
}
