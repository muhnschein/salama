// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The sites that are an exception to one kind of permission, whichever kind the page is
// given: the allowed under a heading and the blocked under another, as Settings >
// Notifications lists its own (docs/DECISIONS/0033-web-notifications.md,
// 0039-site-permissions.md). Each has a menu to change it or remove it, when it follows
// the default again; the pull-down menu adds a site by its address, as sailfish-browser's
// own page of exceptions does (apps/browser/qml/pages/PermissionExceptionsPage.qml), and
// removes them all, after a remorse.
//
// Tracking protection is a kind here too: its sites are the ones it is off for, which is
// all a site on its list can be, so it has no blocked, no way to change a site and one
// heading.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

Page {
    id: exceptionsPage

    // A SitePermissions kind.
    property int kind
    readonly property bool listOnly: kind === SitePermissions.TrackingProtection
    property SitePermissionNames siteNames: SitePermissionNames {}

    objectName: "siteExceptionsPage"
    allowedOrientations: Orientation.Portrait

    Component.onCompleted: SitePermissions.refresh()

    SiteExceptions {
        id: exceptions

        kind: exceptionsPage.kind
        permissions: SitePermissions
    }

    SilicaListView {
        objectName: "siteExceptionList"
        anchors.fill: parent
        model: exceptions
        header: PageHeader {
            title: exceptionsPage.siteNames.kindName(exceptionsPage.kind)
            description: exceptionsPage.listOnly
                         //: Under the title of the list of sites tracking protection is off for
                         ? qsTr("Exceptions")
                         //: Under the title of a list of exceptions to a permission; %1 is what
                         //: it is for every other site
                         : qsTr("Exceptions · default: %1").arg(
                               exceptionsPage.siteNames.defaultName(exceptionsPage.kind))
        }

        PullDownMenu {
            MenuItem {
                objectName: "addSiteMenuItem"
                text: qsTr("Add a site")
                onClicked: pageStack.push(Qt.resolvedUrl("SiteExceptionDialog.qml"),
                                          { "kind": exceptionsPage.kind })
            }

            MenuItem {
                objectName: "removeAllMenuItem"
                visible: exceptions.count > 0
                text: qsTr("Remove all exceptions")
                onClicked: Remorse.popupAction(exceptionsPage,
                                               //: Said while the exceptions are about to be removed
                                               qsTr("Removing exceptions"),
                                               function () {
                                                   SitePermissions.removeAll(exceptionsPage.kind)
                                               })
            }
        }

        // The model lists the allowed first, the blocked after them, and those asked each
        // time last.
        section.property: "decision"
        section.delegate: SectionHeader {
            objectName: "siteExceptionSection"
            text: exceptionsPage.listOnly ? qsTr("Tracking protection off")
                                          : exceptionsPage.siteNames.decisionName(Number(section))
        }

        delegate: SiteExceptionItem {
            objectName: "siteException"
            width: ListView.view.width
            kind: exceptionsPage.kind
            listOnly: exceptionsPage.listOnly
            // Held apart from the row, which a change in the menu may remove or move.
            origin: model.origin
            host: model.host
            decision: model.decision
        }

        footer: Label {
            objectName: "siteExceptionsFooter"
            x: Theme.horizontalPageMargin
            width: exceptionsPage.width - 2 * x
            visible: exceptions.count > 0
            topPadding: Theme.paddingLarge
            bottomPadding: Theme.paddingLarge
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryHighlightColor
            //: Under the sites that are an exception to a permission
            text: qsTr("A site you remove follows the default again.")
        }

        ViewPlaceholder {
            enabled: exceptions.count === 0
            text: qsTr("No exceptions")
        }

        VerticalScrollDecorator {}
    }
}
