// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Silica's own ring, at half the width it draws itself at, in the highlight colour over a
// track in the faint primary colour: what says a download is coming, round the menu's
// Downloads, the menu button and each download's own circle
// (docs/DECISIONS/0038-download-controls.md).
import QtQuick 2.6
import Sailfish.Silica 1.0

ProgressCircle {
    width: Theme.iconSizeMedium
    height: width
    progressColor: Theme.highlightColor
    backgroundColor: Theme.rgba(Theme.primaryColor, Theme.opacityFaint)
    borderWidth: Theme.paddingSmall / 2
}
