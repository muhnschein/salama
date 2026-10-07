// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Switch light centred on icon column (sailfish-browser _textSwitchIconCenter math).
import QtQuick 2.6
import Sailfish.Silica 1.0

TextSwitch {
    leftMargin: Theme.horizontalPageMargin + Theme.paddingLarge
                + Math.round((Theme.iconSizeMedium - Theme.itemSizeExtraSmall) / 2)
}
