// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
// Cookies row only while tracking off or a cookie exception exists.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

Page {
    id: sitePermissionsPage

    objectName: "sitePermissionsPage"
    allowedOrientations: Orientation.Portrait

    property SettingNames names: SettingNames {}
    property SitePermissionNames siteNames: SitePermissionNames {}
    readonly property bool trackingOff:
        PrivacySettings.trackingProtection === PrivacySettings.TrackingProtectionOff
    // Re-queried on list change.
    readonly property int popupExceptions: exceptions(SitePermissions.Popups)
    readonly property int cookieExceptions: exceptions(SitePermissions.Cookies)
    readonly property int locationExceptions: exceptions(SitePermissions.Location)
    readonly property int cameraExceptions: exceptions(SitePermissions.Camera)
    readonly property int microphoneExceptions: exceptions(SitePermissions.Microphone)
    readonly property int trackingExceptions: exceptions(SitePermissions.TrackingProtection)
    readonly property int notificationSites:
        NotificationPermissions.allowedCount + NotificationPermissions.blockedCount

    function exceptions(kind) {
        return (SitePermissions.revision, SitePermissions.count(kind))
    }

    function showExceptions(kind) {
        pageStack.push(Qt.resolvedUrl("SiteExceptionsPage.qml"), { "kind": kind })
    }

    Component.onCompleted: SitePermissions.refresh()

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column

            width: parent.width

            PageHeader {
                //: Settings page: what sites may do
                title: qsTr("Site permissions")
            }

            Label {
                objectName: "sitePermissionsHint"
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                bottomPadding: Theme.paddingMedium
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryHighlightColor
                text: qsTr("What sites may do unless you decided otherwise for a site. Tap one to change it or see the exceptions.")
            }

            SitePermissionRow {
                objectName: "notificationsPermissionRow"
                iconSource: sitePermissionsPage.siteNames.kindIcon(SitePermissions.Notifications)
                text: sitePermissionsPage.siteNames.kindName(SitePermissions.Notifications)
                value: sitePermissionsPage.siteNames.defaultName(SitePermissions.Notifications)
                description: sitePermissionsPage.notificationSites === 0
                             ? sitePermissionsPage.siteNames.exceptionCount(0)
                             : sitePermissionsPage.names.notifications(
                                   NotificationPermissions.allowedCount,
                                   NotificationPermissions.blockedCount, false)
                onOpened: pageStack.push(Qt.resolvedUrl("NotificationSettingsPage.qml"))
            }

            SitePermissionRow {
                objectName: "popupsPermissionRow"
                iconSource: sitePermissionsPage.siteNames.kindIcon(SitePermissions.Popups)
                text: sitePermissionsPage.siteNames.kindName(SitePermissions.Popups)
                value: sitePermissionsPage.siteNames.defaultName(SitePermissions.Popups)
                description: sitePermissionsPage.siteNames.exceptionCount(
                                 sitePermissionsPage.popupExceptions)
                choices: [sitePermissionsPage.siteNames.allow(),
                          sitePermissionsPage.siteNames.block()]
                onChosen: SitePermissionSettings.popupsAllowed = index === 0
                onOpened: sitePermissionsPage.showExceptions(SitePermissions.Popups)
            }

            // Tracking on -> its level decides cookies, choice here moot.
            SitePermissionRow {
                objectName: "cookiesPermissionRow"
                visible: sitePermissionsPage.trackingOff || sitePermissionsPage.cookieExceptions > 0
                iconSource: sitePermissionsPage.siteNames.kindIcon(SitePermissions.Cookies)
                text: sitePermissionsPage.siteNames.kindName(SitePermissions.Cookies)
                value: sitePermissionsPage.siteNames.defaultName(SitePermissions.Cookies)
                description: {
                    var count = sitePermissionsPage.siteNames.exceptionCount(
                                sitePermissionsPage.cookieExceptions)
                    return sitePermissionsPage.trackingOff
                            //: Under the cookies row of Site permissions, which is there only while
                            //: tracking protection is off; %1 is how many exceptions there are
                            ? qsTr("Shown while tracking protection is off · %1").arg(count)
                            : count
                }
                choices: [sitePermissionsPage.siteNames.cookies(SitePermissionSettings.CookiesAllowAll),
                          sitePermissionsPage.siteNames.cookies(SitePermissionSettings.CookiesBlockCrossSite),
                          sitePermissionsPage.siteNames.cookies(SitePermissionSettings.CookiesBlockAll)]
                onChosen: SitePermissionSettings.cookies = index
                onOpened: sitePermissionsPage.showExceptions(SitePermissions.Cookies)
            }

            SitePermissionRow {
                objectName: "locationPermissionRow"
                iconSource: sitePermissionsPage.siteNames.kindIcon(SitePermissions.Location)
                text: sitePermissionsPage.siteNames.kindName(SitePermissions.Location)
                value: sitePermissionsPage.siteNames.defaultName(SitePermissions.Location)
                description: sitePermissionsPage.siteNames.exceptionCount(
                                 sitePermissionsPage.locationExceptions)
                choices: [sitePermissionsPage.siteNames.ask(), sitePermissionsPage.siteNames.block()]
                onChosen: SitePermissionSettings.locationBlocked = index === 1
                onOpened: sitePermissionsPage.showExceptions(SitePermissions.Location)
            }

            SitePermissionRow {
                objectName: "cameraPermissionRow"
                iconSource: sitePermissionsPage.siteNames.kindIcon(SitePermissions.Camera)
                text: sitePermissionsPage.siteNames.kindName(SitePermissions.Camera)
                value: sitePermissionsPage.siteNames.defaultName(SitePermissions.Camera)
                description: sitePermissionsPage.siteNames.exceptionCount(
                                 sitePermissionsPage.cameraExceptions)
                choices: [sitePermissionsPage.siteNames.ask(), sitePermissionsPage.siteNames.block()]
                onChosen: SitePermissionSettings.cameraBlocked = index === 1
                onOpened: sitePermissionsPage.showExceptions(SitePermissions.Camera)
            }

            SitePermissionRow {
                objectName: "microphonePermissionRow"
                iconSource: sitePermissionsPage.siteNames.kindIcon(SitePermissions.Microphone)
                text: sitePermissionsPage.siteNames.kindName(SitePermissions.Microphone)
                value: sitePermissionsPage.siteNames.defaultName(SitePermissions.Microphone)
                description: sitePermissionsPage.siteNames.exceptionCount(
                                 sitePermissionsPage.microphoneExceptions)
                choices: [sitePermissionsPage.siteNames.ask(), sitePermissionsPage.siteNames.block()]
                onChosen: SitePermissionSettings.microphoneBlocked = index === 1
                onOpened: sitePermissionsPage.showExceptions(SitePermissions.Microphone)
            }

            // Only sites turned off via site details.
            SectionHeader {
                objectName: "trackingExceptionsSection"
                visible: sitePermissionsPage.trackingExceptions > 0
                text: qsTr("Turned off for some sites")
            }

            SitePermissionRow {
                objectName: "trackingPermissionRow"
                visible: sitePermissionsPage.trackingExceptions > 0
                iconSource: sitePermissionsPage.siteNames.kindIcon(SitePermissions.TrackingProtection)
                text: sitePermissionsPage.siteNames.kindName(SitePermissions.TrackingProtection)
                value: qsTr("Off for %n site(s)", "", sitePermissionsPage.trackingExceptions)
                description: qsTr("Turned off from a site’s details")
                onOpened: sitePermissionsPage.showExceptions(SitePermissions.TrackingProtection)
            }
        }

        VerticalScrollDecorator {}
    }
}
