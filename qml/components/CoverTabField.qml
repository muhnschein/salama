// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The cover's field of page previews: the open tabs, drawn small, grey and half
// there, arriving from under the heading rather than on a hard line.
//
// It is texture, not a view. The pictures are the ones the tab grid already has
// on disk (docs/DECISIONS/0008-tab-previews.md) and nothing here is tapped,
// scrolled or carried; a cover is drawn while the app is not the active window
// and has no gestures of its own beyond its cover actions. What the cover has to
// say is the number above the field. The field is what makes that number read as
// a browser's rather than a counter's.
//
// Monochrome on purpose. A cover belongs to the phone's ambience rather than to
// the pages inside the app: a dozen screenshots at a glance, each in its own
// site's colours, is noise; the same dozen in grey is a browser with tabs open.
// postivene's cover draws its field of faces the same way, and for the same
// reason.
import QtQuick 2.6
import QtGraphicalEffects 1.0
import Sailfish.Silica 1.0

Item {
    id: field

    /// The previews to draw: one path per open tab, most recently in front first,
    /// and an empty string for a tab that has no picture. TabModel.recentThumbnails
    /// is what the cover hands over; the order is the whole point, so the field
    /// does not sort and does not know what a tab is.
    property alias model: cells.model

    /// How long the previews take to arrive, measured down from the top of the
    /// field. The heading sits above it; the cover sets this to a tenth of its
    /// own height, which is what vuo's texture fades over under the same heading.
    property real fadeHeight: 0

    /// The field is always full, whatever the count. A fixed cell size cannot be:
    /// one tab leaves a stamp in the corner of an empty cover, and two leave a
    /// row with a hole under it. The grid is shaped to what there is instead --
    /// at most two across, at most three down, and the last row widened to take
    /// up what its missing neighbours would have had.
    ///
    /// Six cells is the ceiling. Past that the field says "a lot of tabs" as well
    /// as it is ever going to at cover size, and the number above it is what says
    /// how many there actually are.
    readonly property int maxRows: 3
    readonly property int columns: cells.count <= 2 ? 1 : 2
    readonly property int rows: Math.max(1, Math.min(field.maxRows,
                                                     Math.ceil(cells.count / field.columns)))
    readonly property int shown: Math.min(cells.count, field.columns * field.rows)
    readonly property real rowHeight: field.height / field.rows

    objectName: "coverTabField"
    // The last row reaches the foot of the cover and is cut by it.
    clip: true

    Item {
        id: sheet

        anchors.fill: parent
        // The colour comes out of the whole field in one pass, rather than once
        // per cell: the pictures are drawn as they are and go grey on the way to
        // the screen.
        layer.enabled: true
        layer.effect: Desaturate {
            desaturation: 1.0
        }

        Repeater {
            id: cells

            delegate: Item {
                id: cell

                readonly property int row: Math.floor(index / field.columns)
                readonly property int column: index % field.columns
                // How many cells this row ends up holding: a full row, or what is
                // left of the count on the last one.
                readonly property int rowCells: Math.max(1, Math.min(field.columns,
                                                                     field.shown
                                                                     - field.columns * cell.row))

                objectName: "coverTabCell"
                x: cell.column * cell.width
                y: cell.row * field.rowHeight
                width: field.width / cell.rowCells
                height: field.rowHeight
                // The tabs past the sixth are not drawn; the number says they are
                // there.
                visible: index < field.shown

                Item {
                    id: paper

                    anchors {
                        fill: parent
                        margins: Theme.paddingSmall / 2
                    }
                    // Rounded at the corners, picture and ground alike, as the
                    // tab grid's own cells are. Clipping is rectangular whatever
                    // shape the item has, so the corners are cut by a mask
                    // instead (components/TabPreview.qml).
                    layer.enabled: true
                    layer.effect: OpacityMask {
                        maskSource: Rectangle {
                            width: paper.width
                            height: paper.height
                            radius: Theme.paddingMedium
                            visible: false
                        }
                    }

                    // The cell's own ground, which is the whole of it for a tab
                    // that has no picture: one never displayed this session. The
                    // field keeps its shape either way -- a gap where a tab
                    // is would say there are fewer than the number says.
                    Rectangle {
                        objectName: "coverTabGround"
                        anchors.fill: parent
                        radius: Theme.paddingMedium
                        color: Theme.rgba(Theme.primaryColor, Theme.opacityFaint)
                    }

                    // As wide as the cell and anchored to its top, at the
                    // picture's own aspect, so what shows is the top of what was
                    // last on the screen -- the same reading TabPreview.qml
                    // takes, and for the reason written there.
                    Image {
                        objectName: "coverTabShot"
                        anchors {
                            left: parent.left
                            right: parent.right
                            top: parent.top
                        }
                        height: sourceSize.width > 0 ? width * sourceSize.height / sourceSize.width
                                                     : parent.height
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                        source: modelData.length > 0 ? "file://" + modelData : ""
                        visible: status === Image.Ready
                    }
                }
            }
        }
    }

    // What puts the field under the heading rather than beside it: nothing at the
    // top, everything from fadeHeight down. vuo cuts the same band out of its
    // texture inside its own shader; here the field is finished first and one
    // gradient over it is the whole of it.
    layer.enabled: true
    layer.effect: OpacityMask {
        maskSource: LinearGradient {
            width: field.width
            height: field.height
            start: Qt.point(0, 0)
            end: Qt.point(0, field.height)
            gradient: Gradient {
                GradientStop {
                    position: 0.0
                    color: "transparent"
                }
                // Kept just short of the foot so the two stops below cannot meet
                // and leave the gradient with a pair out of order.
                GradientStop {
                    position: field.height > 0 ? Math.min(0.99, field.fadeHeight / field.height)
                                               : 0.0
                    color: "white"
                }
                GradientStop {
                    position: 1.0
                    color: "white"
                }
            }
        }
    }
}
