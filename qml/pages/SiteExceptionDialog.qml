// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
// Tracking kind: off-list only, so no allow/block choice.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

Dialog {
    id: dialog

    property int kind
    // Empty while not a site.
    readonly property string origin: SitePermissions.originOf(address.text)
    readonly property bool listOnly: kind === SitePermissions.TrackingProtection
    readonly property string scheme: "https://"
    property SitePermissionNames siteNames: SitePermissionNames {}

    objectName: "siteExceptionDialog"
    allowedOrientations: Orientation.Portrait | Orientation.LandscapeMask
    canAccept: origin.length > 0
    // Always ask only for asking kinds.
    onAccepted: SitePermissions.set(kind, origin,
                                    listOnly ? SitePermissions.Allow
                                             : [SitePermissions.Allow, SitePermissions.Block,
                                                SitePermissions.Ask][decision.currentIndex])

    Column {
        width: parent.width

        DialogHeader {
            //: Accept button of the dialog that adds a site to the exceptions
            acceptText: qsTr("Add")
        }

        TextField {
            id: address

            objectName: "siteExceptionAddress"
            width: parent.width
            text: dialog.scheme
            placeholderText: qsTr("Address of the site")
            label: errorHighlight ? qsTr("Must begin with http:// or https://") : placeholderText
            inputMethodHints: Qt.ImhUrlCharactersOnly
            errorHighlight: text !== "" && !/^https?:\/\//i.test(text)
        }

        ComboBox {
            id: decision

            objectName: "siteExceptionDecision"
            visible: !dialog.listOnly
            width: parent.width
            label: dialog.siteNames.kindName(dialog.kind)
            menu: ContextMenu {
                MenuItem {
                    text: dialog.siteNames.allow()
                }

                MenuItem {
                    text: dialog.siteNames.block()
                }

                MenuItem {
                    objectName: "siteExceptionAskChoice"
                    visible: SitePermissions.canAsk(dialog.kind)
                    text: dialog.siteNames.alwaysAsk()
                }
            }
        }
    }
}
