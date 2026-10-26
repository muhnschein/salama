// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Go or search in landscape field row. Engine name only, no own text: rows' titles
// don't fit beside field.
import QtQuick 2.6
import Sailfish.Silica 1.0

BackgroundItem {
    id: pill

    property string iconSource
    property string label
    property bool accent: false

    width: glyph.width + (label.length > 0 ? name.width + Theme.paddingSmall : 0)
           + 2 * Theme.paddingLarge
    height: Theme.itemSizeExtraSmall * 0.75
    highlightedColor: "transparent"

    Rectangle {
        anchors.fill: parent
        radius: height / 2
        color: Theme.rgba(pill.accent || pill.highlighted ? Theme.highlightBackgroundColor
                                                          : Theme.primaryColor,
                          pill.highlighted ? Theme.highlightBackgroundOpacity
                                           : Theme.opacityFaint)
    }

    Icon {
        id: glyph

        anchors {
            left: parent.left
            leftMargin: Theme.paddingLarge
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeSmallPlus
        height: width
        source: pill.iconSource
        highlighted: pill.highlighted
    }

    Label {
        id: name

        anchors {
            left: glyph.right
            leftMargin: Theme.paddingSmall
            verticalCenter: parent.verticalCenter
        }
        visible: pill.label.length > 0
        text: pill.label
        textFormat: Text.PlainText
        color: pill.highlighted ? Theme.highlightColor : Theme.primaryColor
    }
}
