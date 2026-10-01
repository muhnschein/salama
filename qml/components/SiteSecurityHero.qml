// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// What the top of a site's details says of its connection: a padlock and "Connection is
// secure" with who vouches for it, or a warning in the error colour and "Connection is not
// secure" with why -- the certificate's fault when the engine named one, and nothing for
// an address that never had one -- and, with the warning, what not to do on the site. The
// shape and the words are sailfish-browser's own for its certificate page
// (apps/browser/qml/pages/components/CertificateInfo.qml), the icons its own too
// (docs/DECISIONS/0040-site-details.md).
import QtQuick 2.6
import Sailfish.Silica 1.0

Column {
    id: hero

    property bool secure: true
    // Who vouches for the connection, and why it is not to be trusted when it is not.
    property string verifiedBy
    property string reason

    objectName: "siteSecurityHero"
    width: parent.width
    spacing: Theme.paddingMedium
    bottomPadding: Theme.paddingLarge

    Icon {
        objectName: "siteSecurityIcon"
        anchors.horizontalCenter: parent.horizontalCenter
        width: Theme.iconSizeLarge
        height: width
        sourceSize: Qt.size(width, height)
        source: hero.secure ? "image://theme/icon-m-device-lock" : "image://theme/icon-m-warning"
        color: hero.secure ? Theme.primaryColor : Theme.errorColor
    }

    Label {
        objectName: "siteSecurityTitle"
        x: Theme.horizontalPageMargin
        width: parent.width - 2 * x
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        font.pixelSize: Theme.fontSizeLarge
        color: hero.secure ? Theme.primaryColor : Theme.errorColor
        text: hero.secure
              //: The connection to the site is encrypted and its certificate in order
              ? qsTr("Connection is secure")
              //: Either no encryption is in use, or the connection is broken in some way
              : qsTr("Connection is not secure")
    }

    Label {
        objectName: "siteSecurityDetail"
        x: Theme.horizontalPageMargin
        width: parent.width - 2 * x
        visible: text.length > 0
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        textFormat: Text.PlainText
        color: Theme.secondaryHighlightColor
        text: hero.secure ? (hero.verifiedBy.length > 0
                             //: Under "Connection is secure"; %1 is who issued the site's certificate
                             ? qsTr("Verified by %1").arg(hero.verifiedBy) : "")
                          : hero.reason
    }

    Label {
        objectName: "siteSecurityWarning"
        x: Theme.horizontalPageMargin
        width: parent.width - 2 * x
        visible: !hero.secure
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        font.pixelSize: Theme.fontSizeSmall
        color: Theme.secondaryHighlightColor
        text: qsTr("Do not enter personal data, passwords, card details on this site")
    }
}
