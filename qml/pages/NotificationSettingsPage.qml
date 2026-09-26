// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Notifications: whether sites may ask to send them, and the sites allowed and blocked,
// as Firefox's Notification Settings list them -- each with its status, and a way to
// change it or remove the site, which then asks again the next time it wants to. The
// list is the engine's own, read as the page opens (docs/DECISIONS/0033-web-notifications.md).
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Page {
    id: notificationsPage

    objectName: "notificationSettingsPage"
    allowedOrientations: Orientation.Portrait

    Component.onCompleted: NotificationPermissions.refresh()

    SilicaListView {
        objectName: "notificationSiteList"
        anchors.fill: parent
        model: NotificationPermissions
        header: Column {
            width: notificationsPage.width

            PageHeader {
                title: qsTr("Notifications")
            }

            // Firefox's "Block new requests asking to allow notifications".
            TextSwitch {
                objectName: "blockNotificationRequestsSwitch"
                text: qsTr("Block new requests")
                description: qsTr("Sites not listed here cannot ask to send notifications")
                checked: Settings.blockNotificationRequests
                onCheckedChanged: Settings.blockNotificationRequests = checked
            }

            SectionHeader {
                text: qsTr("Sites")
                visible: NotificationPermissions.count > 0
            }
        }

        delegate: ListItem {
            id: site

            // Held apart from the row, which a change in the menu may remove or move.
            readonly property string origin: model.origin
            readonly property bool allowed: model.allowed

            objectName: "notificationSite"
            width: ListView.view.width
            contentHeight: Theme.itemSizeMedium
            menu: ContextMenu {
                MenuItem {
                    objectName: "notificationSiteToggle"
                    text: site.allowed ? qsTr("Block") : qsTr("Allow")
                    onClicked: NotificationPermissions.setAllowed(site.origin, !site.allowed)
                }

                MenuItem {
                    objectName: "notificationSiteRemove"
                    //: Forgets the site's permission: it asks again when it next wants to
                    text: qsTr("Remove")
                    onClicked: NotificationPermissions.remove(site.origin)
                }
            }

            Column {
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }

                Label {
                    objectName: "notificationSiteHost"
                    width: parent.width
                    text: model.host
                    truncationMode: TruncationMode.Fade
                    color: site.highlighted ? Theme.highlightColor : Theme.primaryColor
                }

                Label {
                    objectName: "notificationSiteStatus"
                    width: parent.width
                    text: site.allowed ? qsTr("Allowed") : qsTr("Blocked")
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: site.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                }
            }
        }

        ViewPlaceholder {
            enabled: NotificationPermissions.count === 0
            text: qsTr("No sites")
            hintText: qsTr("Sites you allow to send notifications, or block, are listed here")
        }

        VerticalScrollDecorator {}
    }
}
