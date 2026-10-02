// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// DNS over HTTPS: Firefox for Android's page for it, its levels, its providers and its
// exceptions, in its words (docs/DECISIONS/0047-secure-connections.md). The levels are
// all on the screen at once, the one chosen lit, as Tracking protection lays its own out
// (TrackingSettingsPage.qml); Firefox's Default Protection is not among them, since what
// it stands for -- Firefox deciding when to turn it on -- is not done by the engine here.
// The provider is chosen while it is used at all: Firefox's two, or one of the reader's
// own, given in a dialog of its own. The exceptions have a page of their own.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

Page {
    id: dohPage

    property SettingNames names: SettingNames {}
    // The provider's place among the combo box's choices: a built-in one's own, or the
    // last, Custom.
    readonly property int providerIndex: DohSettings.customProvider
                                         ? 2
                                         : (DohSettings.provider === DohSettings.providers[0].url
                                            ? 0 : 1)

    function restoreProvider() {
        provider.currentIndex = Qt.binding(function () { return dohPage.providerIndex })
    }

    function openCustomProvider() {
        var dialog = pageStack.push(Qt.resolvedUrl("DohProviderDialog.qml"))
        dialog.accepted.connect(dohPage.restoreProvider)
        dialog.rejected.connect(dohPage.restoreProvider)
    }

    objectName: "dohSettingsPage"
    allowedOrientations: Orientation.Portrait

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column

            width: parent.width

            PageHeader {
                title: qsTr("DNS over HTTPS")
            }

            Label {
                objectName: "dohSummary"
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryHighlightColor
                bottomPadding: Theme.paddingLarge
                text: qsTr("Domain Name System (DNS) over HTTPS sends your request for a domain "
                           + "name through an encrypted connection, providing a secure DNS and "
                           + "making it harder for others to see which website you’re about to "
                           + "access.")
            }

            // Firefox for Android's order, without Default: the stored values
            // DohSettings.ProtectionIncreased, ProtectionMax, ProtectionOff.
            Repeater {
                model: [DohSettings.ProtectionIncreased, DohSettings.ProtectionMax,
                        DohSettings.ProtectionOff]

                TextSwitch {
                    objectName: "dohProtectionChoice"
                    automaticCheck: false
                    text: dohPage.names.doh(modelData)
                    description: dohPage.names.dohDescription(modelData)
                    checked: DohSettings.protection === modelData
                    onClicked: DohSettings.protection = modelData
                }
            }

            ComboBox {
                id: provider

                objectName: "dohProviderCombo"
                visible: DohSettings.protection !== DohSettings.ProtectionOff
                width: parent.width
                label: qsTr("Choose provider")
                // The reader's own provider's address, under the choice.
                description: DohSettings.customProvider ? DohSettings.provider : ""
                currentIndex: dohPage.providerIndex
                menu: ContextMenu {
                    MenuItem {
                        objectName: "dohProviderChoice"
                        //: %1 is a DNS over HTTPS provider's name; the default provider
                        text: qsTr("%1 (default)").arg(DohSettings.providers[0].name)
                        onClicked: DohSettings.provider = DohSettings.providers[0].url
                    }

                    MenuItem {
                        objectName: "dohProviderChoice"
                        text: DohSettings.providers[1].name
                        onClicked: DohSettings.provider = DohSettings.providers[1].url
                    }

                    MenuItem {
                        objectName: "dohProviderChoice"
                        //: A DNS over HTTPS provider the reader gives the address of
                        text: qsTr("Custom")
                        onClicked: dohPage.openCustomProvider()
                    }
                }
            }

            SettingsEntry {
                objectName: "dohExceptionsEntry"
                //: The sites DNS over HTTPS is not used for
                text: qsTr("Exceptions")
                value: dohPage.names.dohExceptions(DohSettings.exceptions.length)
                onClicked: pageStack.push(Qt.resolvedUrl("DohExceptionsPage.qml"))
            }
        }

        VerticalScrollDecorator {}
    }
}
