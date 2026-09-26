// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// A site asks to send notifications: Firefox's question, with Firefox's three answers,
// as a dialog, which is how the platform's WebView puts a site's question about the
// reader's location. Allow, above, lets the site for good; Always block, below, never
// lets it ask again; backing out is Firefox's Not now -- refused, and the page is not
// asked about again until it is loaded again. What is decided for good can be changed
// under Settings > Notifications (docs/DECISIONS/0033-web-notifications.md).
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Dialog {
    id: dialog

    // The tab whose page asks, and the site it is on.
    property int tabId: 0
    property string host
    // How the dialog ends when it is not accepted.
    property int refusal: WebNotifications.NotNow

    objectName: "notificationPermissionDialog"
    allowedOrientations: Orientation.Portrait

    onAccepted: WebNotifications.answer(tabId, WebNotifications.Allow)
    onRejected: WebNotifications.answer(tabId, refusal)

    Column {
        width: parent.width
        spacing: Theme.paddingLarge

        DialogHeader {
            //: Lets the site send notifications from now on
            acceptText: qsTr("Allow")
            //: Refuses the site this time; it may ask again when the page is next loaded
            cancelText: qsTr("Not now")
        }

        Label {
            objectName: "notificationPermissionQuestion"
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x
            //: %1 is the site asking, its host alone
            text: qsTr("Allow %1 to send notifications?").arg(dialog.host)
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeLarge
            color: Theme.highlightColor
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x
            text: qsTr("They show up with the phone's other notifications while the site is open in a tab")
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.secondaryHighlightColor
        }

        Button {
            objectName: "blockNotificationsButton"
            anchors.horizontalCenter: parent.horizontalCenter
            //: Refuses the site for good: it cannot ask again
            text: qsTr("Always block")
            onClicked: {
                dialog.refusal = WebNotifications.Block
                dialog.reject()
            }
        }
    }
}
