// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Adds a site to the exceptions of a kind of permission: its address, which has to
// begin with http:// or https:// as an origin does, and whether it is allowed or blocked.
// The shape is sailfish-browser's own
// (apps/browser/qml/pages/components/PermissionCreateDialog.qml). Tracking protection's
// sites are only ever the ones it is off for, so there is nothing to choose
// (docs/DECISIONS/0039-site-permissions.md).
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

Dialog {
    id: dialog

    // A SitePermissions kind.
    property int kind
    // The site as the engine writes it, from what was typed; empty while that is no site.
    readonly property string origin: SitePermissions.originOf(address.text)
    readonly property bool listOnly: kind === SitePermissions.TrackingProtection
    // What the address starts as, for the reader to go on from: the one an origin begins
    // with nearly always.
    readonly property string scheme: "https://"
    property SitePermissionNames siteNames: SitePermissionNames {}

    objectName: "siteExceptionDialog"
    canAccept: origin.length > 0
    // The menu's choices in its order: Allow, Block, and Always ask for a kind that asks.
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
