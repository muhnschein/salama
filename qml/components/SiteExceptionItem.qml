// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Engine keeps no icon per permission, so initial tile.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

ListItem {
    id: site

    property int kind
    property string origin
    property string host
    property int decision
    // Tracking protection: no blocking; only action is remove.
    property bool listOnly: false
    property SitePermissionNames siteNames: SitePermissionNames {}

    contentHeight: Theme.itemSizeSmall
    menu: ContextMenu {
        MenuItem {
            objectName: "siteExceptionAllow"
            visible: !site.listOnly && site.decision !== SitePermissions.Allow
            text: site.siteNames.allow()
            onClicked: SitePermissions.set(site.kind, site.origin, SitePermissions.Allow)
        }

        MenuItem {
            objectName: "siteExceptionBlock"
            visible: !site.listOnly && site.decision !== SitePermissions.Block
            text: site.siteNames.block()
            onClicked: SitePermissions.set(site.kind, site.origin, SitePermissions.Block)
        }

        MenuItem {
            objectName: "siteExceptionAsk"
            visible: !site.listOnly && SitePermissions.canAsk(site.kind)
                     && site.decision !== SitePermissions.Ask
            text: site.siteNames.alwaysAsk()
            onClicked: SitePermissions.set(site.kind, site.origin, SitePermissions.Ask)
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
