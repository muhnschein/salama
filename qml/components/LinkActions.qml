// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Rows under each other, in the sheet's cells.
import QtQuick 2.6
import Sailfish.Silica 1.0

Column {
    id: actions

    property Item sheet

    Grid {
        id: pageRow

        objectName: "linkPageRow"
        width: 4 * sheet.cell
        columns: 4
        visible: sheet.opensPage

        MenuButton {
            objectName: "newTabLinkButton"
            width: sheet.cell
            labelSize: sheet.labelSize
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
            width: sheet.cell
            labelSize: sheet.labelSize
            round: true
            iconSource: "image://theme/icon-m-tabs"
            //: The link sheet's action: the link in a new tab, left behind the one in front
            text: qsTr("Background tab")
            onClicked: sheet.openBehind()
        }

        MenuButton {
            objectName: "shareLinkButton"
            width: sheet.cell
            labelSize: sheet.labelSize
            round: true
            iconSource: "image://theme/icon-m-share"
            text: qsTr("Share")
            onClicked: sheet.share()
        }

        MenuButton {
            objectName: "saveLinkButton"
            width: sheet.cell
            labelSize: sheet.labelSize
            round: true
            iconSource: "image://theme/icon-m-downloads"
            //: The link sheet's action: what the link leads to, downloaded
            text: qsTr("Save link")
            onClicked: sheet.save(sheet.target.link, "")
        }
    }

    Grid {
        id: appRow

        objectName: "linkAppRow"
        width: 4 * sheet.cell
        columns: 4
        visible: sheet.forApp

        MenuButton {
            objectName: "appLinkButton"
            width: sheet.cell
            labelSize: sheet.labelSize
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
            width: sheet.cell
            labelSize: sheet.labelSize
            round: true
            iconSource: "image://theme/icon-m-share"
            text: qsTr("Share")
            onClicked: sheet.share()
        }
    }

    MenuSeparator {
        width: actions.width
        visible: sheet.hasLink && sheet.hasImage
        lineName: "linkMenuSeparator"
    }

    Grid {
        id: imageRow

        objectName: "linkImageRow"
        width: 4 * sheet.cell
        columns: 4
        visible: sheet.hasImage

        MenuButton {
            objectName: "openImageButton"
            width: sheet.cell
            labelSize: sheet.labelSize
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
            width: sheet.cell
            labelSize: sheet.labelSize
            round: true
            iconSource: "image://theme/icon-m-downloads"
            //: The link sheet's action: the picture, downloaded
            text: qsTr("Save image")
            onClicked: sheet.save(sheet.target.image, sheet.target.contentType)
        }

        MenuButton {
            objectName: "copyImageLinkButton"
            width: sheet.cell
            labelSize: sheet.labelSize
            round: true
            iconSource: "image://theme/icon-m-clipboard"
            //: The link sheet's action: the picture's address, put on the clipboard
            text: qsTr("Copy image link")
            onClicked: sheet.copy(sheet.target.image, sheet.imageCopied())
        }
    }
}
