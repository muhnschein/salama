// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

ListItem {
    id: row

    property int kind
    property string origin
    property SitePermissionNames siteNames: SitePermissionNames {}
    // revision in comma expression forces re-eval on list change.
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
            objectName: "siteDecisionAsk"
            visible: SitePermissions.canAsk(row.kind)
            text: row.siteNames.alwaysAsk()
            onClicked: SitePermissions.set(row.kind, row.origin, SitePermissions.Ask)
        }

        MenuItem {
            objectName: "siteDecisionDefault"
            text: row.siteNames.followDefault(row.kind)
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

        Label {
            objectName: "siteDecisionValue"
            width: parent.width
            text: row.followsDefault ? row.siteNames.followDefault(row.kind)
                                     : row.siteNames.decisionName(row.decision)
            truncationMode: TruncationMode.Fade
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.highlightColor
        }
    }
}
