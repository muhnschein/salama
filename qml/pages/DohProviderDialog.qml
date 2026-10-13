// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
// Custom DoH provider: must be https:// with host.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Dialog {
    id: dialog

    readonly property int problem: DohSettings.providerProblem(address.text)

    objectName: "dohProviderDialog"
    allowedOrientations: Orientation.Portrait | Orientation.LandscapeMask
    canAccept: problem === DohSettings.ProviderValid
    onAccepted: DohSettings.provider = address.text

    Column {
        width: parent.width

        DialogHeader {
            title: qsTr("Custom provider")
            //: Accept button of the dialog that gives a DNS over HTTPS provider
            acceptText: qsTr("Add")
        }

        TextField {
            id: address

            objectName: "dohProviderAddress"
            width: parent.width
            text: DohSettings.customProvider ? DohSettings.provider : "https://"
            placeholderText: qsTr("Provider")
            errorHighlight: text !== "" && dialog.problem !== DohSettings.ProviderValid
                            && text !== "https://"
            label: !errorHighlight ? placeholderText
                                   : dialog.problem === DohSettings.ProviderNotHttps
                                     ? qsTr("URL must start with “https://”")
                                     : qsTr("Invalid URL")
            inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase | Qt.ImhUrlCharactersOnly
        }
    }
}
