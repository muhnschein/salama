// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Off for site = "trackingprotection" Allow permission on origin (Firefox's own exemption);
// page reloaded so engine applies it.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Column {
    id: tracking

    property string origin
    property Item view
    // From QMozSecurity, if present.
    property bool blockedTrackers: false
    property SitePermissionNames siteNames: SitePermissionNames {}
    readonly property bool offInSettings:
        PrivacySettings.trackingProtection === PrivacySettings.TrackingProtectionOff
    readonly property bool offForSite:
        (SitePermissions.revision,
         SitePermissions.decision(SitePermissions.TrackingProtection, origin) ===
         SitePermissions.Allow)
    readonly property bool isOn: !offInSettings && !offForSite

    objectName: "siteTrackingProtection"
    width: parent.width

    TextSwitch {
        objectName: "siteTrackingSwitch"
        // State follows permission via model, not tap.
        automaticCheck: false
        enabled: !tracking.offInSettings
        checked: tracking.isOn
        text: tracking.siteNames.kindName(SitePermissions.TrackingProtection)
        description: {
            if (tracking.offInSettings) {
                //: The site's details, under the tracking protection switch: it is off for every site
                return qsTr("Off in Settings")
            }
            if (tracking.offForSite) {
                return qsTr("Off for this site. Turn it on to block trackers here again.")
            }
            //: The site's details, under the tracking protection switch while it is on
            return qsTr("If something looks broken on this site, try turning this off.")
        }
        onClicked: {
            if (tracking.isOn) {
                SitePermissions.set(SitePermissions.TrackingProtection, tracking.origin,
                                    SitePermissions.Allow)
            } else {
                SitePermissions.remove(SitePermissions.TrackingProtection, tracking.origin)
            }
            if (tracking.view && tracking.view.reload) {
                tracking.view.reload()
            }
        }
    }

    Label {
        objectName: "siteTrackersBlocked"
        x: Theme.horizontalPageMargin
        width: parent.width - 2 * x
        visible: tracking.isOn && tracking.blockedTrackers
        topPadding: Theme.paddingSmall
        wrapMode: Text.Wrap
        font.pixelSize: Theme.fontSizeExtraSmall
        color: Theme.secondaryHighlightColor
        text: qsTr("Trackers were blocked on this page")
    }
}
