// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Theme icon with small sign at bottom right, as Material's bookmark-add: theme has no star with
// plus, check or list. Icon keeps full size; gap round sign cut out of it, not painted over,
// so it holds on any ground, pressed too.
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: glyph

    property string source
    // "", "add", "check" or "list".
    property string badge
    property bool highlighted: false

    readonly property color ink: highlighted ? Theme.highlightColor : Theme.primaryColor
    // Near theme icons' line weight.
    readonly property real stroke: Math.max(1, Math.round(width / 28))
    readonly property real gap: Math.max(1, Math.round(width / 24))
    // Strokes as [x1, y1, x2, y2], fractions of sign's box.
    readonly property var strokes: badge === "add" ? [[0.5, 0.1, 0.5, 0.9], [0.1, 0.5, 0.9, 0.5]]
                                   : badge === "check" ? [[0.1, 0.55, 0.38, 0.83],
                                                          [0.38, 0.83, 0.9, 0.2]]
                                   : badge === "list" ? [[0.1, 0.2, 0.9, 0.2],
                                                         [0.1, 0.5, 0.9, 0.5],
                                                         [0.1, 0.8, 0.9, 0.8]]
                                   : []

    width: Theme.iconSizeMedium
    height: width

    Icon {
        objectName: "menuButtonGlyph"
        anchors.fill: parent
        source: glyph.source
        highlighted: glyph.highlighted
        layer.enabled: glyph.badge !== ""
        layer.effect: ShaderEffect {
            readonly property variant cut: ShaderEffectSource {
                sourceItem: cutout
                hideSource: true
            }

            objectName: "menuButtonCutEffect"
            fragmentShader: "
                varying highp vec2 qt_TexCoord0;
                uniform lowp float qt_Opacity;
                uniform sampler2D source;
                uniform sampler2D cut;
                void main() {
                    gl_FragColor = texture2D(source, qt_TexCoord0)
                            * (1.0 - texture2D(cut, qt_TexCoord0).a) * qt_Opacity;
                }"
        }
    }

    // Sign's strokes widened by gap each side: what is cut from icon.
    Item {
        id: cutout

        width: glyph.width
        height: glyph.height
        visible: false

        BadgeStrokes {
            objectName: "menuButtonCutout"
            x: sign.x
            y: sign.y
            width: sign.width
            height: sign.height
            strokes: glyph.strokes
            thickness: glyph.stroke + 2 * glyph.gap
            color: "black"
        }
    }

    BadgeStrokes {
        id: sign

        objectName: "menuButtonBadge"
        anchors {
            right: parent.right
            bottom: parent.bottom
        }
        width: Math.round(parent.width * 0.375)
        height: width
        visible: glyph.badge !== ""
        strokes: glyph.strokes
        thickness: glyph.stroke
        color: glyph.ink
    }
}
