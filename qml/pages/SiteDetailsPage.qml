// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// What is known of the site of the page in front and what it may do, from the head of the
// menu sheet: whether the connection is secure and whose certificate says so, what
// tracking protection is doing for the site and a switch for it, and the permissions the
// site has been given or left to the defaults, each changed where it is. The page the
// padlock in the address bar stands for in sailfish-browser
// (apps/browser/qml/pages/components/CertificateInfo.qml), with the site's permissions
// beside it as its own SitePermissionPage has them (docs/DECISIONS/0040-site-details.md).
//
// The connection is the engine's, as the view has it, view.security, which is not there
// for an engine build that has none, nor for a view that has gone since the page was
// opened, and every binding here copes with that (0011-address-and-security.md). An
// address that is not http or https has no site: its permissions are not drawn.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

Page {
    id: detailsPage

    // The page: its address and title as the menu sheet's head had them, and the view
    // that shows it, to read its connection from and to load again.
    property string url
    property string title
    property Item view
    property SitePermissionNames siteNames: SitePermissionNames {}

    readonly property string origin: SitePermissions.originOf(url)
    readonly property bool https: url.indexOf("https://") === 0
    readonly property var security: view && view.security ? view.security : null
    // The engine is not satisfied with the connection, as the address bar's warning says
    // (components/BrowserMenu.qml).
    readonly property bool tlsBroken:
        https && !!security && !!security.validState && !security.allGood
    readonly property bool secure: https && !tlsBroken
    // Why, as the engine's own verdict says it, the most to the point first.
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
    // Cookies are tracking protection's while it is on, for every site and for this one.
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

    // What the engine keeps is read as the page opens: a site may have asked, and been
    // answered, since it was last read.
    Component.onCompleted: SitePermissions.refresh()

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

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

            Button {
                objectName: "clearSitePermissionsButton"
                anchors.horizontalCenter: parent.horizontalCenter
                visible: detailsPage.exceptions > 0
                text: qsTr("Clear site permissions")
                onClicked: {
                    // Tracking protection comes back on with the site's other decisions, and
                    // the page is told so as it is when the switch does it.
                    var wasOff = SitePermissions.decision(SitePermissions.TrackingProtection,
                                                          detailsPage.origin) === SitePermissions.Allow
                    SitePermissions.removeAllForOrigin(detailsPage.origin)
                    if (wasOff && detailsPage.view && detailsPage.view.reload) {
                        detailsPage.view.reload()
                    }
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
