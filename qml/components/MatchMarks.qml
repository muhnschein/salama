// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
import QtQuick 2.6
import Sailfish.Silica 1.0

// Search marks as vuo draws them: a model's <b> runs in the ambience's highlight colour too,
// bold alone being hard to make out. Model escaped the text, so every <b> is a mark.
QtObject {
    function marked(styled) {
        return String(styled).replace(/<b>/g, "<b><font color=\"" + Theme.highlightColor + "\">")
                             .replace(/<\/b>/g, "</font></b>")
    }
}
