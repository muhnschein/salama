// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// What the engine says of a site's connection, under a heading of its own: whom the
// certificate was issued to and by, until when it is good, and the protocol and the
// cipher suite in use, as Silica lays details out (DetailItem). The words are
// sailfish-browser's own for its certificate page
// (apps/browser/qml/pages/components/CertificateInfo.qml). A line the engine had nothing
// for is not drawn, and with nothing at all neither is the heading
// (docs/DECISIONS/0040-site-details.md).
//
// The engine's own is QMozSecurity, which the page's view has (qtmozembed
// src/qmozsecurity.h). It is read through bindings that cope with its not being there, as
// the address bar's padlock is (0011-address-and-security.md).
import QtQuick 2.6
import Sailfish.Silica 1.0

Column {
    id: details

    // The view's security, or null.
    property var security

    readonly property string issuedTo: plain(security ? security.subjectDisplayName : "")
    readonly property string issuedBy: plain(security ? security.issuerDisplayName : "")
    // The expiry as Silica shows a date, or empty when there is none or it is not one.
    readonly property string validUntil: {
        var expiry = security ? security.expiryDate : null
        if (!expiry || isNaN(new Date(expiry).getTime())) {
            return ""
        }
        //: How a certificate's last day is written, as Qt reads a date format: "14 Dec 2026"
        return Qt.formatDate(new Date(expiry), qsTr("d MMM yyyy"))
    }
    // QMozSecurity::TLS_VERSION, in the order of the enum from SSL 3, which is 0.
    readonly property string protocol: {
        var version = security ? security.protocolVersion : -1
        return ["SSL 3.0", "TLS 1.0", "TLS 1.1", "TLS 1.2", "TLS 1.3"][version] || ""
    }
    readonly property string cipher: plain(security ? security.cipherName : "")
    readonly property bool hasDetails: issuedTo.length > 0 || issuedBy.length > 0
                                       || validUntil.length > 0 || protocol.length > 0
                                       || cipher.length > 0

    function plain(value) {
        return value ? String(value) : ""
    }

    objectName: "siteConnectionDetails"
    width: parent.width
    visible: hasDetails

    SectionHeader {
        //: Heading over what is known of a site's connection
        text: qsTr("Connection")
    }

    DetailItem {
        objectName: "siteIssuedTo"
        visible: value.length > 0
        //: Whom a site's certificate was issued to
        label: qsTr("Issued to")
        value: details.issuedTo
    }

    DetailItem {
        objectName: "siteVerifiedBy"
        visible: value.length > 0
        //: Who issued a site's certificate
        label: qsTr("Verified by")
        value: details.issuedBy
    }

    DetailItem {
        objectName: "siteValidUntil"
        visible: value.length > 0
        //: The last day a site's certificate is good
        label: qsTr("Valid until")
        value: details.validUntil
    }

    DetailItem {
        objectName: "siteProtocol"
        visible: value.length > 0
        //: The protocol the connection to a site uses
        label: qsTr("Protocol")
        value: details.protocol
    }

    DetailItem {
        objectName: "siteCipherSuite"
        visible: value.length > 0
        //: The cipher suite the connection to a site uses
        label: qsTr("Cipher suite")
        value: details.cipher
    }
}
