// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
// List read from engine on open. Forgotten site asks again.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Page {
    id: notificationsPage

    objectName: "notificationSettingsPage"
    allowedOrientations: Orientation.Portrait | Orientation.LandscapeMask

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

            // Firefox's "Block new requests..." inverted so on = allowed.
            TextSwitch {
                objectName: "sitesCanAskSwitch"
                //: Whether sites not yet allowed or blocked may ask to send notifications
                text: qsTr("Sites can ask")
                checked: !PrivacySettings.blockNotificationRequests
                onCheckedChanged: PrivacySettings.blockNotificationRequests = !checked
            }
        }

        section.property: "allowed"
        section.delegate: SectionHeader {
            objectName: "notificationSiteSection"
            text: section === "true" ? qsTr("Allowed") : qsTr("Blocked")
        }

        delegate: ListItem {
            id: site

            // Held outside row, which menu change may remove or move.
            readonly property string origin: model.origin
            readonly property bool allowed: model.allowed
            readonly property string host: model.host

            objectName: "notificationSite"
            width: ListView.view.width
            contentHeight: Theme.itemSizeSmall
            menu: ContextMenu {
                MenuItem {
                    objectName: "notificationSiteToggle"
                    text: site.allowed ? qsTr("Block") : qsTr("Allow")
                    onClicked: NotificationPermissions.setAllowed(site.origin, !site.allowed)
                }

                MenuItem {
                    objectName: "notificationSiteRemove"
                    //: Forgets the site's permission: it asks again when it next wants to
                    text: qsTr("Forget this site")
                    onClicked: NotificationPermissions.remove(site.origin)
                }
            }

            // Engine keeps no icon with permission.
            Rectangle {
                id: letter

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
                    font.pixelSize: Theme.fontSizeExtraSmall
                    font.bold: true
                    color: Theme.primaryColor
                }
            }

            Label {
                objectName: "notificationSiteHost"
                anchors {
                    left: letter.right
                    right: parent.right
                    leftMargin: Theme.paddingMedium
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                text: site.host
                truncationMode: TruncationMode.Fade
                color: site.highlighted ? Theme.highlightColor : Theme.primaryColor
            }
        }

        footer: Label {
            objectName: "notificationSitesFooter"
            x: Theme.horizontalPageMargin
            width: notificationsPage.width - 2 * x
            visible: NotificationPermissions.count > 0
            topPadding: Theme.paddingLarge
            bottomPadding: Theme.paddingLarge
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryHighlightColor
            //: Under the sites allowed and blocked from sending notifications
            text: qsTr("A site you forget asks again the next time it wants to send one.")
        }

        ViewPlaceholder {
            enabled: NotificationPermissions.count === 0
            text: qsTr("No sites")
            hintText: qsTr("Sites you allow to send notifications, or block, are listed here")
        }

        VerticalScrollDecorator {}
    }
}
