// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
import QtQuick 2.6
import Sailfish.Silica 1.0

Grid {
    id: hero

    property bool secure: true
    property string verifiedBy
    property string reason
    // Landscape: the icon beside the words, not above them.
    property bool compact: false

    objectName: "siteSecurityHero"
    width: parent.width
    columns: compact ? 2 : 1
    spacing: Theme.paddingMedium
    topPadding: compact ? Theme.paddingLarge : Theme.paddingLarge * 2
    bottomPadding: Theme.paddingLarge

    Item {
        id: iconSpace

        // Beside the words the icon keeps the page margin, as the rows under it do.
        width: hero.compact ? Theme.horizontalPageMargin + Theme.iconSizeLarge : hero.width
        height: Theme.iconSizeLarge

        Icon {
            objectName: "siteSecurityIcon"
            x: hero.compact ? Theme.horizontalPageMargin : (parent.width - width) / 2
            width: Theme.iconSizeLarge
            height: width
            sourceSize: Qt.size(width, height)
            source: hero.secure ? "image://theme/icon-m-device-lock"
                                : "image://theme/icon-m-warning"
            color: hero.secure ? Theme.primaryColor : Theme.errorColor
        }
    }

    Column {
        width: hero.compact ? hero.width - iconSpace.width - hero.spacing : hero.width
        spacing: Theme.paddingMedium

        Label {
            objectName: "siteSecurityTitle"
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x
            horizontalAlignment: hero.compact ? Text.AlignLeft : Text.AlignHCenter
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
            horizontalAlignment: hero.compact ? Text.AlignLeft : Text.AlignHCenter
            wrapMode: Text.Wrap
            textFormat: Text.PlainText
            color: Theme.secondaryHighlightColor
            text: hero.secure ? (hero.verifiedBy.length > 0
                                 //: Under "Connection is secure"; %1 is who issued
                                 //: the site's certificate
                                 ? qsTr("Verified by %1").arg(hero.verifiedBy) : "")
                              : hero.reason
        }

        Label {
            objectName: "siteSecurityWarning"
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x
            visible: !hero.secure
            horizontalAlignment: hero.compact ? Text.AlignLeft : Text.AlignHCenter
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.secondaryHighlightColor
            text: qsTr("Do not enter personal data, passwords, card details on this site")
        }
    }
}
