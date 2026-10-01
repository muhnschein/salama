// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Tracking protection for the one site, in its details: the switch, with what it means
// said under it, and while it is on and the engine blocked trackers on the page, that
// it did. Tracking protection is Firefox's, and a site it breaks is put on its allow
// list: the "trackingprotection" permission of the site's origin, which is what Firefox's
// "Turn off Enhanced Tracking Protection for this site" writes. So the switch adds the
// permission or takes it away, and the page is loaded again for the engine to apply it
// (docs/DECISIONS/0023-tracking-protection.md, 0039-site-permissions.md,
// 0040-site-details.md). Off in Settings it is off for every site, and the switch is
// dimmed and says so rather than offer what would change nothing.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Column {
    id: tracking

    // The site's origin, and the page's view, which is loaded again after a change.
    property string origin
    property Item view
    // The engine blocked trackers on the page; its QMozSecurity says so, if it has one.
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
        // The state is the permission's, not the tap's: it is set again from the model.
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
