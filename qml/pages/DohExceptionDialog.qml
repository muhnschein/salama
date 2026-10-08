// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
// Full address typed -> its domain.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Dialog {
    id: dialog

    readonly property string domain: DohSettings.domainOf(site.text)

    objectName: "dohExceptionDialog"
    canAccept: domain.length > 0
    onAccepted: DohSettings.addException(domain)

    Column {
        width: parent.width

        DialogHeader {
            //: Accept button of the dialog that adds a site DNS over HTTPS is not used for
            acceptText: qsTr("Save")
        }

        TextField {
            id: site

            objectName: "dohExceptionSite"
            width: parent.width
            //: An example of a domain, shown in the empty field
            placeholderText: qsTr("example.com")
            errorHighlight: text.trim() !== "" && dialog.domain === ""
            label: errorHighlight ? qsTr("Must be a valid domain") : qsTr("Site")
            inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase | Qt.ImhUrlCharactersOnly
        }
    }
}
