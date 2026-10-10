// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
// Domain covers subdomains.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Page {
    id: exceptionsPage

    objectName: "dohExceptionsPage"
    allowedOrientations: Orientation.Portrait | Orientation.LandscapeMask

    SilicaListView {
        objectName: "dohExceptionList"
        anchors.fill: parent
        model: DohSettings.exceptions
        header: Column {
            width: exceptionsPage.width

            PageHeader {
                title: qsTr("Exceptions")
            }

            Label {
                objectName: "dohExceptionsSummary"
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                bottomPadding: Theme.paddingLarge
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryHighlightColor
                //: Salama is the browser's name
                text: qsTr("Salama won’t use secure DNS on these sites and their subdomains.")
            }
        }

        PullDownMenu {
            MenuItem {
                objectName: "addDohExceptionMenuItem"
                text: qsTr("Add site")
                onClicked: pageStack.push(Qt.resolvedUrl("DohExceptionDialog.qml"))
            }

            MenuItem {
                objectName: "removeAllDohExceptionsMenuItem"
                visible: DohSettings.exceptions.length > 0
                text: qsTr("Remove all exceptions")
                onClicked: Remorse.popupAction(exceptionsPage,
                                               //: Said while the exceptions are about to be removed
                                               qsTr("Removing exceptions"),
                                               function () { DohSettings.removeAllExceptions() })
            }
        }

        delegate: ListItem {
            id: site

            // Held outside row, which removal destroys.
            property string domain: modelData

            objectName: "dohException"
            width: ListView.view.width
            contentHeight: Theme.itemSizeSmall
            menu: ContextMenu {
                MenuItem {
                    objectName: "dohExceptionRemove"
                    text: qsTr("Remove")
                    onClicked: DohSettings.removeException(site.domain)
                }
            }
            onClicked: openMenu()

            Label {
                objectName: "dohExceptionDomain"
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                anchors.verticalCenter: parent.verticalCenter
                text: site.domain
                textFormat: Text.PlainText
                truncationMode: TruncationMode.Fade
                color: site.highlighted ? Theme.highlightColor : Theme.primaryColor
            }
        }

        ViewPlaceholder {
            enabled: DohSettings.exceptions.length === 0
            text: qsTr("No exceptions")
        }

        VerticalScrollDecorator {}
    }
}
