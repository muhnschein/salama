// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Nav bar address in edit; host's size so text doesn't jump. sailfish-browser's entry: faint slab
// as tall as field, no underline or label, text centred, starting at page margin.
import QtQuick 2.6
import Sailfish.Silica 1.0

TextField {
    id: field

    // Silica clears focus on press outside field; pane press isn't end of typing.
    property bool keepsFocus: false

    objectName: "addressField"
    placeholderText: qsTr("Search or enter address")
    font.pixelSize: Theme.fontSizeMedium
    inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase | Qt.ImhUrlCharactersOnly
    labelVisible: false
    textTopMargin: (height - lineMetrics.height) / 2
    background: Rectangle {
        objectName: "addressFieldSlab"
        anchors.fill: parent
        color: Theme.primaryColor
        opacity: 0.1
    }
    EnterKey.enabled: text.length > 0
    EnterKey.iconSource: "image://theme/icon-m-enter-accept"

    FontMetrics {
        id: lineMetrics

        font: field.font
    }

    // Via Binding: Silica lacking prop logs, doesn't fail load.
    Binding {
        target: field
        property: "textLeftMargin"
        value: Theme.horizontalPageMargin
    }

    Binding {
        target: field
        property: "textRightMargin"
        value: Theme.paddingLarge
    }

    // Always bound: on Qt 5.6 released Binding leaves plain values as-is.
    Binding {
        target: field
        property: "focusOutBehavior"
        value: field.keepsFocus ? FocusBehavior.KeepFocus : FocusBehavior.ClearItemFocus
    }
}
