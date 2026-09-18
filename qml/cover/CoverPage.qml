// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// What the cover has to say while the app is minimised: how many tabs are open,
// and what they are.
//
// The heading is laid out as the platform's own covers lay theirs out, and with
// postivene's and vuo's measures exactly: the name top left with a line under it,
// the two set closer than their line boxes would put them, and the number top
// right, large. The rest of the cover is the tabs themselves, grey and half
// there, fading in a large padding below the heading -- see
// components/CoverTabField.qml and docs/DECISIONS/0014-cover-is-the-tab-count.md.
//
// The number is the message. The field is what makes it a browser's number.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

CoverBackground {
    id: cover

    /// The app's own name, and never a translated one: a name is not a word to be
    /// put into another language. Held as a property rather than written into the
    /// label so that ci/qml-lint.sh's untranslated-string check stays as strict as
    /// it is -- every bare string in a text: binding is a defect, and this is the one
    /// string that is not.
    readonly property string brandName: "Salama"

    /// What the cover is set to show (Settings.coverStyle): its own icon and nothing
    /// else, the heading over the one tab last read, or the heading over all of them.
    /// Everything below turns on these two.
    readonly property bool showsHeading: Settings.coverStyle !== Settings.CoverIconOnly
    readonly property bool showsEveryTab: Settings.coverStyle === Settings.CoverEveryTab

    objectName: "coverPage"

    // Under the words, and declared first so that it is: the field is the ground
    // the heading and the number are read against.
    // The icon-only cover: the app's own mark, quietly, and the one action. No count
    // and no pictures -- for a reader who wants the switcher to stay a row of apps
    // rather than a row of screens, and for whom a tab count is not news.
    Image {
        objectName: "coverIcon"
        anchors.centerIn: parent
        width: Math.round(cover.width * 0.45)
        height: width
        visible: !cover.showsHeading
        opacity: Theme.opacityHigh
        smooth: true
        source: Qt.resolvedUrl("../../art/harbour-salama.png")
    }

    CoverTabField {
        anchors {
            top: heading.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            topMargin: Theme.paddingLarge
        }
        // Texture, not a picture to be looked into. Grey at full strength reads as
        // a second screen inside the cover and pulls the eye off the number.
        visible: cover.showsHeading
        opacity: Theme.opacityLow
        fadeHeight: cover.height * 0.14
        // Most recently in front first, so what a glance lands on is where the reader
        // has just been rather than whichever tab is oldest. Cut to one for the middle
        // style, where the field becomes a single full-bleed picture of the tab just
        // left -- the grid shapes itself to what it is given.
        model: cover.showsEveryTab ? TabModel.recentThumbnails
                                   : TabModel.recentThumbnails.slice(0, 1)
    }

    // The name and what the number counts, top left; the number top right, always
    // -- a browser with one tab has something to say as much as one with twelve.
    Column {
        id: heading

        objectName: "coverHeading"
        visible: cover.showsHeading
        anchors {
            top: parent.top
            left: parent.left
            right: tabCount.left
            margins: Theme.paddingLarge
            rightMargin: Theme.paddingMedium
        }
        // The two lines are one heading: set closer than their own leading would
        // put them, as postivene's and vuo's are.
        spacing: -Theme.paddingSmall

        Label {
            objectName: "coverBrand"
            width: parent.width
            textFormat: Text.PlainText
            text: cover.brandName
            color: Theme.highlightColor
            font.pixelSize: Theme.fontSizeMedium
            truncationMode: TruncationMode.Fade
        }

        Label {
            objectName: "coverSubtitle"
            width: parent.width
            textFormat: Text.PlainText
            text: qsTr("Tabs")
            color: Theme.secondaryHighlightColor
            font.pixelSize: Theme.fontSizeExtraSmall
            truncationMode: TruncationMode.Fade
        }
    }

    Label {
        id: tabCount

        objectName: "coverTabCount"
        visible: cover.showsHeading
        anchors {
            top: parent.top
            right: parent.right
            topMargin: Theme.paddingMedium
            rightMargin: Theme.paddingLarge
        }
        textFormat: Text.PlainText
        // Two digits is what a browser needs. Past a hundred tabs the number has
        // stopped being a count and become a state of affairs, and the reader is
        // not totting them up off a cover anyway.
        text: TabModel.count > 99 ? "99+" : TabModel.count
        // Three glyphs at the huge size run straight over the app's name; the
        // number is anchored to the edge and grows leftwards into it. It steps
        // down instead, which keeps the digits legible AND the name readable.
        font.pixelSize: TabModel.count > 99 ? Theme.fontSizeExtraLarge : Theme.fontSizeHuge
        color: Theme.primaryColor
    }

    // One action, and it is the one a browser is opened for: a new tab with the
    // address field already up and the keyboard with it. The window does the work --
    // the field belongs to the browsing page, which is not in a cover's scope.
    CoverActionList {
        CoverAction {
            objectName: "searchCoverAction"
            iconSource: "image://theme/icon-cover-search"
            onTriggered: window.requestNewTab()
        }
    }
}
