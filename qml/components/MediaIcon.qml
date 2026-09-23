// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The glyph of a media control, the same on the grid's previews and on the navigation
// bar: pause while the page plays and play while it is paused; the speaker while the
// tab's sound is on, and the speaker struck through while it is muted. Theme ids that
// are in use elsewhere, none guessed: play and pause are Jolla's media controls', the
// two speakers those of the Harbour players that mute (Jupii, harbour-sailfishconnect).
// There is no small size of any of them, so the medium ones are drawn small
// (docs/DECISIONS/0024-media-controls.md).
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Icon {
    // "playback" or "mute".
    property string control
    property int mediaState: TabModel.NoMedia
    property bool muted: false

    width: Theme.iconSizeSmall
    height: width
    source: {
        if (control === "playback") {
            return mediaState === TabModel.MediaPlaying ? "image://theme/icon-m-pause"
                                                        : "image://theme/icon-m-play"
        }
        return muted ? "image://theme/icon-m-speaker-mute" : "image://theme/icon-m-speaker-on"
    }
}
