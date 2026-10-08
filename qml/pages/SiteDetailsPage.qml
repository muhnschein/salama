// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
// view.security absent on engines without it or after view gone: bindings cope.
// Non-http(s) address has no site -> no permissions drawn.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

Page {
    id: detailsPage

    property string url
    property string title
    property Item view
    property SitePermissionNames siteNames: SitePermissionNames {}

    readonly property string origin: SitePermissions.originOf(url)
    readonly property bool https: url.indexOf("https://") === 0
    readonly property var security: view && view.security ? view.security : null
    readonly property bool tlsBroken:
        https && !!security && !!security.validState && !security.allGood
    readonly property bool secure: https && !tlsBroken
    // Most relevant first.
    readonly property string reason: {
        if (!tlsBroken) {
            return ""
        }
        if (security.notValidAtThisTime) {
            return qsTr("The certificate has expired or is not yet valid")
        }
        if (security.domainMismatch) {
            return qsTr("The certificate is for another site")
        }
        return security.untrusted ? qsTr("The certificate is not trusted") : ""
    }
    // Tracking protection owns cookies while on.
    readonly property bool trackingOff:
        PrivacySettings.trackingProtection === PrivacySettings.TrackingProtectionOff
        || (SitePermissions.revision,
            SitePermissions.decision(SitePermissions.TrackingProtection, origin)
            === SitePermissions.Allow)
    readonly property bool cookiesShown:
        trackingOff || (SitePermissions.revision,
                        SitePermissions.decision(SitePermissions.Cookies, origin)
                        !== SitePermissions.Default)
    readonly property int exceptions: (SitePermissions.revision, SitePermissions.originCount(origin))

    objectName: "siteDetailsPage"
    allowedOrientations: Orientation.Portrait

    // Reread on open: site may have asked since.
    Component.onCompleted: SitePermissions.refresh()

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        // Only when site has own choice: no empty pulley.
        PullDownMenu {
            objectName: "siteDetailsPulley"
            visible: detailsPage.exceptions > 0

            MenuItem {
                objectName: "clearSitePermissionsMenuItem"
                text: qsTr("Clear site permissions")
                onClicked: {
                    // Tracking protection returns on with other decisions; page told as switch would.
                    var wasOff = SitePermissions.decision(SitePermissions.TrackingProtection,
                                                          detailsPage.origin) === SitePermissions.Allow
                    SitePermissions.removeAllForOrigin(detailsPage.origin)
                    if (wasOff && detailsPage.view && detailsPage.view.reload) {
                        detailsPage.view.reload()
                    }
                }
            }
        }

        Column {
            id: column

            width: parent.width

            PageHeader {
                title: SearchSettings.displayAddress(detailsPage.url)
                description: detailsPage.title
            }

            SiteSecurityHero {
                secure: detailsPage.secure
                verifiedBy: detailsPage.secure && detailsPage.security
                            ? detailsPage.security.issuerDisplayName || "" : ""
                reason: detailsPage.reason
            }

            SiteConnectionDetails {
                security: detailsPage.https ? detailsPage.security : null
            }

            SectionHeader {
                objectName: "siteTrackingSection"
                visible: detailsPage.origin.length > 0
                text: detailsPage.siteNames.kindName(SitePermissions.TrackingProtection)
            }

            SiteTrackingSwitch {
                visible: detailsPage.origin.length > 0
                origin: detailsPage.origin
                view: detailsPage.view
                blockedTrackers: !!detailsPage.security && !!detailsPage.security.blockedTrackingContent
            }

            SectionHeader {
                objectName: "sitePermissionsSection"
                visible: detailsPage.origin.length > 0
                //: Heading over what the site may do
                text: qsTr("Permissions")
            }

            Repeater {
                model: detailsPage.origin.length > 0
                       ? [SitePermissions.Notifications, SitePermissions.Popups,
                          SitePermissions.Cookies, SitePermissions.Location,
                          SitePermissions.Camera, SitePermissions.Microphone]
                       : []

                SiteDecisionRow {
                    objectName: "siteDecisionRow"
                    width: column.width
                    visible: modelData !== SitePermissions.Cookies || detailsPage.cookiesShown
                    kind: modelData
                    origin: detailsPage.origin
                }
            }

            Item {
                width: parent.width
                height: Theme.paddingLarge
            }
        }

        VerticalScrollDecorator {}
    }
}
