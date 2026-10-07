// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Choice names shared by choice pages and main-page value lines. Lists ordered by stored value.
import QtQuick 2.6
import harbour.salama 1.0

QtObject {
    function startPage(blank) {
        //: What a new tab shows: the start page's sections, or nothing
        return blank ? qsTr("Blank page") : qsTr("Your sites")
    }

    function websiteColors(colors) {
        return [
            //: Pages are drawn light or dark as the ambience is: sailfish-browser's words
            qsTr("Match ambience"),
            qsTr("Light"),
            qsTr("Dark")
        ][colors] || ""
    }

    // Settings.NotchGuard order.
    function notchGuard(guard) {
        return [
            //: Notch guard mode that lets adapted websites use the notch area
            qsTr("Automatic"),
            //: Notch guard mode that always keeps website content away from the notch
            qsTr("Forced"),
            //: Notch guard mode that lets every website use the notch area
            qsTr("Disabled")
        ][guard] || ""
    }

    function readerColors(colors) {
        return [
            //: The reader view in Firefox's light or dark colours as the ambience is
            qsTr("Automatic"),
            qsTr("Light"),
            qsTr("Sepia"),
            qsTr("Dark"),
            //: The reader view set as a Sailfish page is, in the ambience's colours
            qsTr("Ambience")
        ][colors] || ""
    }

    function typeface(typeface) {
        return typeface === ReaderSettings.Serif ? qsTr("Serif") : qsTr("Sans serif")
    }

    function textSize(step) {
        //: A text size, as a share of the default: "100 %"
        return qsTr("%1 %").arg(Math.round(100 * Reader.fontSizeFor(step)
                                           / Reader.fontSizeFor(ReaderSettings.TextSizeDefault)))
    }

    function reader(colors, typefaceChosen, step) {
        //: The reader view's colours, typeface and text size: "Ambience · Sans serif · 100 %"
        return qsTr("%1 · %2 · %3").arg(readerColors(colors)).arg(typeface(typefaceChosen))
                                   .arg(textSize(step))
    }

    function quickAction(action) {
        return [
            //: The cover has no quick action
            qsTr("None"),
            //: A quick action on the cover: the address bar, opened for a new tab
            qsTr("Search"),
            //: A quick action on the cover: the list of bookmarks
            qsTr("Bookmarks"),
            //: A quick action on the cover: one bookmark's page, picked on the next page
            qsTr("Open a bookmark"),
            //: A quick action on the cover: the list of downloads
            qsTr("Downloads"),
            //: A quick action on the cover: the history
            qsTr("History")
        ][action] || ""
    }

    // Deleted bookmark still offered by cover; opens bookmarks list.
    function quickActionValue(action, bookmarkId, bookmarkTitle) {
        if (action !== CoverSettings.QuickActionBookmark) {
            return quickAction(action)
        }
        //: The cover's quick action opens a bookmark that has since been deleted
        return bookmarkId > 0 ? bookmarkTitle : qsTr("Deleted bookmark")
    }

    function cover(action, bookmarkId, bookmarkTitle) {
        return action === CoverSettings.QuickActionNone
                //: The cover's line in Settings when it offers no quick action
                ? qsTr("No quick action")
                : quickActionValue(action, bookmarkId, bookmarkTitle)
    }

    function trackingProtection(level) {
        return [
            //: Tracking protection is off
            qsTr("Off"),
            qsTr("Standard"),
            qsTr("Strict")
        ][level] || ""
    }

    // Promises only what every supported engine does.
    function trackingProtectionDescription(level) {
        return [
            qsTr("Sites can follow you from one to another"),
            qsTr("Stops sites following you with cookies"),
            qsTr("Stops more tracking, and can break some sites")
        ][level] || ""
    }

    function notifications(allowed, blocked, blockRequests) {
        if (allowed === 0 && blocked === 0) {
            return blockRequests ? qsTr("Sites cannot ask") : qsTr("Sites can ask")
        }
        //: How many sites may send notifications
        var allowedText = qsTr("%n site(s) allowed", "", allowed)
        //: How many sites may not send notifications
        var blockedText = qsTr("%n blocked", "", blocked)
        if (blocked === 0) {
            return allowedText
        }
        if (allowed === 0) {
            //: How many sites may not send notifications, with none allowed
            return qsTr("%n site(s) blocked", "", blocked)
        }
        //: "2 sites allowed · 1 blocked"
        return qsTr("%1 · %2").arg(allowedText).arg(blockedText)
    }

    function sitePermissions(sites) {
        //: Settings' line under Site permissions when no site has a decision of its own
        return sites === 0 ? qsTr("No exceptions")
                             //: How many sites have a permission decided for them
                           : qsTr("%n site(s) with exceptions", "", sites)
    }

    function history(remember, clearOnClose) {
        if (!remember) {
            //: The pages visited are not kept in the history
            return qsTr("Not remembered")
        }
        //: The pages visited are kept until the browser closes, or kept for good
        return clearOnClose ? qsTr("Cleared when closed") : qsTr("Remembered")
    }

    function httpsOnly(on) {
        //: HTTPS-Only Mode is on
        return on ? qsTr("On", "HTTPS-Only Mode")
                    //: HTTPS-Only Mode is off
                  : qsTr("Off", "HTTPS-Only Mode")
    }

    function doh(level) {
        return [
            //: DNS over HTTPS is off
            qsTr("Off", "DNS over HTTPS"),
            qsTr("Increased Protection"),
            qsTr("Max Protection")
        ][level] || ""
    }

    // Promises nothing engine leaves to Firefox front end (e.g. Max Protection fallback warning).
    function dohDescription(level) {
        return [
            qsTr("Use your default DNS resolver"),
            qsTr("Only use your default DNS resolver if there is a problem with secure DNS"),
            qsTr("If secure DNS is not available sites will not load or function properly")
        ][level] || ""
    }

    function dohExceptions(sites) {
        //: No site is an exception to DNS over HTTPS
        return sites === 0 ? qsTr("None", "no sites")
                             //: How many sites DNS over HTTPS is not used for
                           : qsTr("%n site(s)", "", sites)
    }
}
