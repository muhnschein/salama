// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// What the navigation bar's address turns into while it is edited: Silica's own field,
// at the size the host is drawn at so the text does not jump when the label becomes a
// field, with its text inset by a padding rather than by a page's margin
// (docs/DECISIONS/0009-navigation-bar-gesture.md). Where it sits, and what editing
// means, are the bar's.
import QtQuick 2.6
import Sailfish.Silica 1.0

TextField {
    id: field

    // The omnibar's pane is up above the bar (docs/DECISIONS/0027-omnibar.md). Silica
    // takes a field's focus away at a press anywhere outside it, and a press on the
    // pane -- a row, the list being scrolled -- is not the end of typing.
    property bool keepsFocus: false

    objectName: "addressField"
    placeholderText: qsTr("Search or enter address")
    font.pixelSize: Theme.fontSizeMedium
    inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase | Qt.ImhUrlCharactersOnly
    EnterKey.enabled: text.length > 0
    EnterKey.iconSource: "image://theme/icon-m-enter-accept"

    // Silica insets the text inside a field by a page margin at each end, which is a
    // page's margin, not a bar's. These three through Binding: a Silica without them
    // should cost a line in the log rather than a bar that fails to load.
    Binding {
        target: field
        property: "textLeftMargin"
        value: Theme.paddingMedium
    }

    Binding {
        target: field
        property: "textRightMargin"
        value: Theme.paddingMedium
    }

    // Both ways, rather than only while the pane is up: a Binding that lets go puts
    // back a binding it had replaced, and on Qt 5.6 a plain value it leaves as it was.
    // Otherwise, Silica's own default.
    Binding {
        target: field
        property: "focusOutBehavior"
        value: field.keepsFocus ? FocusBehavior.KeepFocus : FocusBehavior.ClearItemFocus
    }
}
