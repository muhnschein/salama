// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// A tab group's picture in the list of groups: the preview of the tab last in front in
// it, filling a square cut from the top of the page, as the Gallery's list of albums
// shows each album by a picture from it -- square, uncut at the corners, and touching
// the pictures of the rows above and below. The previews are the ones the grid already
// has (docs/DECISIONS/0008-tab-previews.md). A tab with no preview is the grid's
// placeholder ground, and a group with no tabs a fainter one
// (docs/DECISIONS/0015-tab-groups.md).
import QtQuick 2.6
import Sailfish.Silica 1.0

Rectangle {
    id: picture

    // The pictures' paths as TabGroups' previews role hands them out: the most recent
    // tab's alone, an empty string for a tab with none, and nothing for a group with no
    // tabs.
    property var previews: []
    readonly property bool holdsTab: previews ? previews.length > 0 : false
    readonly property string path: holdsTab ? previews[0] : ""

    color: holdsTab ? Theme.rgba(Theme.highlightBackgroundColor, Theme.highlightBackgroundOpacity)
                    : Theme.rgba(Theme.primaryColor, Theme.opacityFaint / 2)

    // Cropped to the top of the page, as the grid shows the top of what was last on the
    // screen rather than the middle of the page. Decoded at the size it is drawn: a
    // preview is half the screen.
    Image {
        objectName: "tabGroupPictureImage"
        anchors.fill: parent
        fillMode: Image.PreserveAspectCrop
        verticalAlignment: Image.AlignTop
        sourceSize.width: width
        asynchronous: true
        source: picture.path.length > 0 ? "file://" + picture.path : ""
        visible: status === Image.Ready
    }
}
