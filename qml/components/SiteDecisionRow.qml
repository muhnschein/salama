// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// One kind of permission for the one site, in its details: the kind's icon and name, and
// under the name what the site has been given -- allowed or blocked -- or, with no
// exception, what the default is, marked "default". A tap offers allowing, blocking, and
// the site's following the default again: written "Always ask" where the default is to
// ask, which is what following it comes to, and "Default" where it is not. The engine has
// no record that says to ask, so the third is taking the record away
// (docs/DECISIONS/0039-site-permissions.md, 0040-site-details.md).
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

ListItem {
    id: row

    // A SitePermissions kind, and the site's origin.
    property int kind
    property string origin
    property SitePermissionNames siteNames: SitePermissionNames {}
    // As the site has it now, asked again as the list changes.
    readonly property int decision: (SitePermissions.revision,
                                     SitePermissions.decision(kind, origin))
    readonly property bool followsDefault: decision === SitePermissions.Default

    contentHeight: Theme.itemSizeMedium
    menu: ContextMenu {
        MenuItem {
            objectName: "siteDecisionAllow"
            text: row.siteNames.allow()
            onClicked: SitePermissions.set(row.kind, row.origin, SitePermissions.Allow)
        }

        MenuItem {
            objectName: "siteDecisionBlock"
            text: row.siteNames.block()
            onClicked: SitePermissions.set(row.kind, row.origin, SitePermissions.Block)
        }

        MenuItem {
            objectName: "siteDecisionDefault"
            text: row.siteNames.defaultAsks(row.kind)
                  //: Lets the site be asked about the permission each time it wants it
                  ? qsTr("Always ask")
                  //: Takes the site's own choice away, so that what is set for every site
                  //: applies to it
                  : qsTr("Default")
            onClicked: SitePermissions.remove(row.kind, row.origin)
        }
    }
    onClicked: openMenu()

    Icon {
        id: icon

        objectName: "siteDecisionIcon"
        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeSmallPlus
        height: width
        sourceSize: Qt.size(width, height)
        source: row.siteNames.kindIcon(row.kind)
        highlighted: row.highlighted
    }

    Column {
        anchors {
            left: icon.right
            right: parent.right
            leftMargin: Theme.paddingMedium
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }

        Label {
            objectName: "siteDecisionName"
            width: parent.width
            text: row.siteNames.kindName(row.kind)
            truncationMode: TruncationMode.Fade
            color: row.highlighted ? Theme.highlightColor : Theme.primaryColor
        }

        Row {
            width: parent.width
            spacing: Theme.paddingMedium

            Label {
                id: value

                objectName: "siteDecisionValue"
                text: row.followsDefault ? row.siteNames.defaultName(row.kind)
                                         : row.siteNames.decisionName(row.decision)
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.highlightColor
            }

            Label {
                objectName: "siteDecisionDefaultMark"
                width: Math.max(0, parent.width - value.width - parent.spacing)
                visible: row.followsDefault
                truncationMode: TruncationMode.Fade
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryHighlightColor
                //: Marks a permission that is what is set for every site, rather than the site's own
                text: qsTr("default")
            }
        }
    }
}
