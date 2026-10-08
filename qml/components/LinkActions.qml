// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
import QtQuick 2.6
import Sailfish.Silica 1.0

Column {
    property Item sheet

    Grid {
        objectName: "linkPageRow"
        width: parent.width
        columns: 4
        visible: sheet.opensPage

        MenuButton {
            objectName: "newTabLinkButton"
            width: sheet.width / 4
            round: true
            iconSource: "image://theme/icon-m-tab-new"
            //: The link sheet's action: the link in a new tab, brought to the front
            text: qsTr("New tab")
            onClicked: {
                sheet.hide()
                sheet.openRequested(sheet.target.link, true)
            }
        }

        MenuButton {
            objectName: "backgroundTabLinkButton"
            width: sheet.width / 4
            round: true
            iconSource: "image://theme/icon-m-tabs"
            //: The link sheet's action: the link in a new tab, left behind the one in front
            text: qsTr("Background tab")
            onClicked: sheet.openBehind()
        }

        MenuButton {
            objectName: "shareLinkButton"
            width: sheet.width / 4
            round: true
            iconSource: "image://theme/icon-m-share"
            text: qsTr("Share")
            onClicked: sheet.share()
        }

        MenuButton {
            objectName: "saveLinkButton"
            width: sheet.width / 4
            round: true
            iconSource: "image://theme/icon-m-downloads"
            //: The link sheet's action: what the link leads to, downloaded
            text: qsTr("Save link")
            onClicked: sheet.save(sheet.target.link, "")
        }
    }

    Grid {
        objectName: "linkAppRow"
        width: parent.width
        columns: 4
        visible: sheet.forApp

        MenuButton {
            objectName: "appLinkButton"
            width: sheet.width / 4
            round: true
            iconSource: sheet.appIcon(sheet.target.scheme)
            text: sheet.appAction(sheet.target.scheme)
            onClicked: {
                sheet.hide()
                Qt.openUrlExternally(sheet.target.link)
            }
        }

        MenuButton {
            objectName: "shareAppLinkButton"
            width: sheet.width / 4
            round: true
            iconSource: "image://theme/icon-m-share"
            text: qsTr("Share")
            onClicked: sheet.share()
        }
    }

    Item {
        width: parent.width
        height: Theme.paddingLarge
        visible: sheet.hasLink && sheet.hasImage

        Row {
            objectName: "linkMenuSeparator"
            anchors.centerIn: parent

            Separator {
                width: sheet.width / 2 - Theme.horizontalPageMargin
                color: Theme.primaryColor
                rotation: 180
            }

            Separator {
                width: sheet.width / 2 - Theme.horizontalPageMargin
                color: Theme.primaryColor
            }
        }
    }

    Grid {
        objectName: "linkImageRow"
        width: parent.width
        columns: 4
        visible: sheet.hasImage

        MenuButton {
            objectName: "openImageButton"
            width: sheet.width / 4
            round: true
            iconSource: "image://theme/icon-m-file-image"
            //: The link sheet's action: the picture alone, in a new tab
            text: qsTr("Open image")
            onClicked: {
                sheet.hide()
                sheet.openRequested(sheet.target.image, true)
            }
        }

        MenuButton {
            objectName: "saveImageButton"
            width: sheet.width / 4
            round: true
            iconSource: "image://theme/icon-m-downloads"
            //: The link sheet's action: the picture, downloaded
            text: qsTr("Save image")
            onClicked: sheet.save(sheet.target.image, sheet.target.contentType)
        }

        MenuButton {
            objectName: "copyImageLinkButton"
            width: sheet.width / 4
            round: true
            iconSource: "image://theme/icon-m-clipboard"
            //: The link sheet's action: the picture's address, put on the clipboard
            text: qsTr("Copy image link")
            onClicked: sheet.copy(sheet.target.image, sheet.imageCopied())
        }
    }
}
