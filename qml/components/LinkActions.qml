// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Link sheet's actions as Silica rows, like menu's: link group, gap, image group.
import QtQuick 2.6
import Sailfish.Silica 1.0

Column {
    property Item sheet

    Column {
        objectName: "linkPageRow"
        width: parent.width
        visible: sheet.opensPage

        MenuListItem {
            objectName: "newTabLinkButton"
            width: parent.width
            iconSource: "image://theme/icon-m-tab-new"
            //: The link sheet's action: the link in a new tab, brought to the front
            text: qsTr("New tab")
            onClicked: {
                sheet.hide()
                sheet.openRequested(sheet.target.link, true)
            }
        }

        MenuListItem {
            objectName: "backgroundTabLinkButton"
            width: parent.width
            iconSource: "image://theme/icon-m-tabs"
            //: The link sheet's action: the link in a new tab, left behind the one in front
            text: qsTr("Background tab")
            onClicked: sheet.openBehind()
        }

        MenuListItem {
            objectName: "shareLinkButton"
            width: parent.width
            iconSource: "image://theme/icon-m-share"
            text: qsTr("Share")
            onClicked: sheet.share()
        }

        MenuListItem {
            objectName: "saveLinkButton"
            width: parent.width
            iconSource: "image://theme/icon-m-downloads"
            //: The link sheet's action: what the link leads to, downloaded
            text: qsTr("Save link")
            onClicked: sheet.save(sheet.target.link, "")
        }
    }

    Column {
        objectName: "linkAppRow"
        width: parent.width
        visible: sheet.forApp

        MenuListItem {
            objectName: "appLinkButton"
            width: parent.width
            iconSource: sheet.appIcon(sheet.target.scheme)
            text: sheet.appAction(sheet.target.scheme)
            onClicked: {
                sheet.hide()
                Qt.openUrlExternally(sheet.target.link)
            }
        }

        MenuListItem {
            objectName: "shareAppLinkButton"
            width: parent.width
            iconSource: "image://theme/icon-m-share"
            text: qsTr("Share")
            onClicked: sheet.share()
        }
    }

    Item {
        objectName: "linkGroupGap"
        width: parent.width
        height: Theme.paddingLarge
        visible: sheet.hasLink && sheet.hasImage
    }

    Column {
        objectName: "linkImageRow"
        width: parent.width
        visible: sheet.hasImage

        MenuListItem {
            objectName: "openImageButton"
            width: parent.width
            iconSource: "image://theme/icon-m-file-image"
            //: The link sheet's action: the picture alone, in a new tab
            text: qsTr("Open image")
            onClicked: {
                sheet.hide()
                sheet.openRequested(sheet.target.image, true)
            }
        }

        MenuListItem {
            objectName: "saveImageButton"
            width: parent.width
            iconSource: "image://theme/icon-m-downloads"
            //: The link sheet's action: the picture, downloaded
            text: qsTr("Save image")
            onClicked: sheet.save(sheet.target.image, sheet.target.contentType)
        }

        MenuListItem {
            objectName: "copyImageLinkButton"
            width: parent.width
            iconSource: "image://theme/icon-m-clipboard"
            //: The link sheet's action: the picture's address, put on the clipboard
            text: qsTr("Copy image link")
            onClicked: sheet.copy(sheet.target.image, sheet.imageCopied())
        }
    }
}
