// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// A site that is an exception to a kind of permission, in the list of them
// (pages/SiteExceptionsPage.qml): its first letter on a square, as the start page draws a
// site whose icon it does not know (components/SiteTile.qml) -- the engine keeps no icon
// with a permission -- and its host. A menu changes it to the other way, or removes it,
// when it follows the default again (docs/DECISIONS/0039-site-permissions.md). The same
// row as Settings > Notifications lists its sites in (0033-web-notifications.md).
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

ListItem {
    id: site

    property int kind
    property string origin
    property string host
    property bool allowed
    // Tracking protection has no blocking: a site is on its list because it is off for
    // the site, and all there is to do with it is to take it off.
    property bool listOnly: false

    contentHeight: Theme.itemSizeSmall
    menu: ContextMenu {
        MenuItem {
            objectName: "siteExceptionToggle"
            visible: !site.listOnly
            text: site.allowed ? qsTr("Block") : qsTr("Allow")
            onClicked: SitePermissions.set(site.kind, site.origin,
                                           site.allowed ? SitePermissions.Block
                                                        : SitePermissions.Allow)
        }

        MenuItem {
            objectName: "siteExceptionRemove"
            //: Takes the site's exception away: it follows the default again
            text: qsTr("Remove")
            onClicked: SitePermissions.remove(site.kind, site.origin)
        }
    }
    onClicked: openMenu()

    Rectangle {
        id: letter

        objectName: "siteExceptionLetter"
        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeSmall
        height: width
        radius: Theme.paddingSmall
        color: Theme.rgba(Theme.highlightBackgroundColor, Theme.highlightBackgroundOpacity)

        Label {
            anchors.centerIn: parent
            text: SearchSettings.displayAddress(site.origin).charAt(0).toUpperCase()
            textFormat: Text.PlainText
            font.pixelSize: Theme.fontSizeExtraSmall
            font.bold: true
            color: Theme.primaryColor
        }
    }

    Label {
        objectName: "siteExceptionHost"
        anchors {
            left: letter.right
            right: parent.right
            leftMargin: Theme.paddingMedium
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        text: site.host
        textFormat: Text.PlainText
        truncationMode: TruncationMode.Fade
        color: site.highlighted ? Theme.highlightColor : Theme.primaryColor
    }
}
