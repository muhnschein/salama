// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The cover as a reader who never opens Settings has it: the launcher icon's bolt,
// large, in the ambience's highlight colour, from above the top edge down to just short
// of the actions, and each time the cover comes into view a flash of sheet lightning
// behind it. It says nothing, and is not meant to: what the cover offers is its actions
// (docs/DECISIONS/0031-cover-is-lightning.md).
//
// No name over it. The bolt is the icon's, and the icon is how the home screen already
// tells one application from another.
import QtQuick 2.6
import QtGraphicalEffects 1.0
import Sailfish.Silica 1.0

Item {
    id: lightning

    /// Whether the cover is on the screen. The flash plays as this turns true and is
    /// put out as it turns false, so nothing animates where nobody can see it.
    property bool active

    /// Whether the ambience is a dark one. A glow in the highlight colour lights a dark
    /// cover; on a light one the same glow reads as a smudge, so there it is white, and
    /// the bolt is not whitened as it is lit either.
    property bool onDark: true

    /// How lit the cover is: 0 at rest, 1 at the height of the flash. The strike below
    /// drives it, and everything the flash draws follows it.
    property real flash: 0

    objectName: "coverLightning"

    onActiveChanged: {
        if (active) {
            strike.restart()
        } else {
            strike.stop()
            flash = 0
        }
    }

    // The flash itself: light behind the bolt, brightest just off its upper arm and
    // gone well before the edges. Under the bolt, and declared first so that it is.
    RadialGradient {
        objectName: "coverFlash"
        anchors.fill: parent
        horizontalOffset: lightning.width * 0.14
        verticalOffset: -lightning.height * 0.22
        horizontalRadius: lightning.width * 0.75
        verticalRadius: lightning.height * 0.55
        visible: lightning.flash > 0
        opacity: lightning.flash
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: lightning.onDark ? Theme.rgba(Theme.highlightColor, 0.45)
                                        : Qt.rgba(1, 1, 1, 0.75)
            }
            GradientStop {
                position: 0.72
                color: Theme.rgba(Theme.highlightColor, 0)
            }
        }
    }

    // The picture, white and fading from tip to foot as drawn (icons/cover-bolt.svg),
    // is the bolt's shape only; what the reader sees is the overlay below. Its box is
    // given as parts of the cover so that it keeps its place on a small cover as on a
    // large one: from a quarter of the cover above the top edge, with the tip off the
    // right edge, to the foot at three quarters of the way down, clear of the actions.
    Image {
        id: bolt

        objectName: "coverBolt"
        x: lightning.width * 0.376
        y: -lightning.height * 0.235
        width: lightning.width * 0.646
        height: lightning.height
        visible: false
        smooth: true
        mipmap: true
        source: Qt.resolvedUrl("../../art/cover/bolt.png")
    }

    ColorOverlay {
        objectName: "coverBoltTint"
        anchors.fill: bolt
        source: bolt
        color: Theme.highlightColor
        opacity: Theme.opacityHigh
    }

    // The bolt lit by its own flash: the same picture, white as it is drawn, over the
    // tinted one for as long as the flash lasts. A dark ambience's only.
    Image {
        objectName: "coverBoltLit"
        anchors.fill: bolt
        visible: lightning.onDark && lightning.flash > 0
        opacity: 0.45 * lightning.flash
        smooth: true
        mipmap: true
        source: bolt.source
    }

    // Two strikes, as lightning has: one, a dip, a brighter one, and a slow fade.
    // Two in half a second and no more is far under anything that could be called
    // flicker. The pause lets the home screen finish bringing the cover in first.
    SequentialAnimation {
        id: strike

        objectName: "coverStrike"

        PauseAnimation {
            duration: 250
        }

        NumberAnimation {
            target: lightning
            property: "flash"
            to: 0.85
            duration: 200
            easing.type: Easing.OutQuad
        }

        NumberAnimation {
            target: lightning
            property: "flash"
            to: 0.3
            duration: 120
            easing.type: Easing.InOutQuad
        }

        NumberAnimation {
            target: lightning
            property: "flash"
            to: 1
            duration: 150
            easing.type: Easing.OutQuad
        }

        NumberAnimation {
            target: lightning
            property: "flash"
            to: 0
            duration: 1200
            easing.type: Easing.OutCubic
        }
    }
}
