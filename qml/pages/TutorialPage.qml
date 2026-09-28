// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The tutorial: what this browser does its own way, taught as the platform's own Tutorial
// teaches the system's gestures -- on a sketch of the screen that answers the finger, one
// step at a time, each shown by Silica's TapInteractionHint or TouchInteractionHint and
// said by an InteractionHintLabel, and each waiting for the finger to have done it. The
// address bar, which goes to an address and searches alike; the menu; the grid under the
// page; and in the grid, a tab closed, moved, and moved to another group
// (docs/DECISIONS/0034-tutorial.md).
//
// On the first start a card comes first, with the application's mark, to start the
// tutorial or skip it. From Settings the lessons start at once. Back leaves it at any step,
// as it leaves any page.
//
// The deck is the real one (components/TabDeck.qml), and so are the bar's gesture and
// handle and the grid's cells, so a drag, a slide or a carry is caught, followed and
// released here as it is in the browser. What the deck carries is a sketch: nothing done
// here opens, moves or closes a real tab.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

Page {
    id: tutorialPage

    // The first start's card, with the way to skip it all, before the lessons.
    property bool welcome: false
    // Where the tutorial is: "welcome"; then a step of a lesson, one of the lessons below;
    // then "done", and the card that says so a moment after.
    property string step: welcome ? "welcome" : lessons[0]
    readonly property var lessons: ["address", "omnibar", "menu", "menuOpen", "open", "closeTab",
                                    "moveTab", "groupTab", "close"]
    property bool recapShown: false
    // The deck may be raised, or lowered, now: only by the step that asks for it, and
    // freely once it is all done.
    readonly property bool canOpen: step === "open" || step === "done"
    readonly property bool canClose: step === "close" || step === "done"
    readonly property bool hinting: hintShown()
    // The label says the step while its hint is shown, and while the address bar's pane
    // is explained, which has no hint.
    readonly property bool saying: hinting || step === "omnibar"

    // What the display's cutout takes at the top of the screen, which the sketch keeps
    // out of as the browsing page does, while Settings says so.
    readonly property real cutoutHeight: Settings.cutoutGuard && Screen.topCutout
                                         ? Math.max(0, Screen.topCutout.y + Screen.topCutout.height)
                                         : 0

    objectName: "tutorialPage"
    allowedOrientations: Orientation.Portrait

    function begin() {
        recapShown = false
        grid.reset()
        deck.settle(false)
        step = lessons[0]
    }

    // The step after this one: the next of the lessons, and after the last, done.
    function next() {
        var at = lessons.indexOf(step)
        if (at < 0) {
            return
        }
        if (at + 1 < lessons.length) {
            step = lessons[at + 1]
        } else {
            step = "done"
            recapDelay.restart()
        }
    }

    // A gesture made on a cell: the step it is for moves on. Made out of turn, it changes
    // only the sketch, unless it leaves too few tabs for what comes next, when the sketch
    // starts over.
    function tabGesture(forStep) {
        if (step === forStep) {
            next()
        } else if (grid.count < 2) {
            grid.reset()
        }
    }

    // How a step is shown: a tap, a movement, or neither.
    function hintKind(forStep) {
        if (forStep === "address" || forStep === "menu" || forStep === "menuOpen") {
            return "tap"
        }
        if (forStep === "omnibar" || lessons.indexOf(forStep) < 0) {
            return ""
        }
        return "touch"
    }

    // Shown while a step waits for a gesture, and not under a finger already making one.
    // Read from the page's own state each time rather than from a binding on it: the
    // handlers below ask as the state changes, before a binding would have caught up.
    function hintShown() {
        return hintKind(step) !== "" && !deck.dragging && !grid.handling
                && status === PageStatus.Active
    }

    // The step's hint, while it waits for its gesture.
    function updateHints() {
        if (hintShown()) {
            hints.show(step)
        } else {
            hints.stop()
        }
    }

    // What the label says for a step: one short sentence, or two where the first is
    // what the step shows and the second what to do.
    function stepText(forStep) {
        switch (forStep) {
        case "address":
            //: The tutorial's first step: the address bar at the foot of the screen
            return qsTr("Tap the address bar to open a website or search.")
        case "omnibar":
            //: The tutorial shows the address bar being edited, with a row to go to an
            //: address and a row to search above it
            return qsTr("Type an address or a search. Matching tabs, bookmarks and history appear above the bar.")
        case "menu":
            return qsTr("Tap the menu button.")
        case "menuOpen":
            return qsTr("The menu has actions for this page and the browser. Tap outside it to close it.")
        case "open":
            //: The navigation bar at the foot of the screen is dragged upwards, and the
            //: grid of open tabs comes up from under the page
            return qsTr("Drag the bar up to see your tabs.")
        case "closeTab":
            return qsTr("Swipe a tab left to close it.")
        case "moveTab":
            return qsTr("Hold a tab, then drag it to a new position.")
        case "groupTab":
            //: The names of the tab groups are in a row at the foot of the grid
            return qsTr("Hold a tab, then drop it on a group name to move it there.")
        case "close":
            //: The grid of tabs is pulled down past its top to bring the page back
            return qsTr("Pull down to return to the page.")
        }
        return ""
    }

    // Which of the five lessons a step is part of: the address bar, the menu, the grid
    // brought up, the tabs in it, and the page brought back.
    function lessonOf(forStep) {
        return [["address", "omnibar"], ["menu", "menuOpen"], ["open"],
                ["closeTab", "moveTab", "groupTab"], ["close"]]
                .map(function (steps) { return steps.indexOf(forStep) >= 0 })
                .indexOf(true)
    }

    onStepChanged: updateHints()
    onStatusChanged: updateHints()

    TabDeck {
        id: deck

        objectName: "tutorialDeck"
        width: parent.width
        pageHeight: tutorialPage.height
        onTabsOpenChanged: {
            if (tutorialPage.step === "open" && tabsOpen
                    || tutorialPage.step === "close" && !tabsOpen) {
                tutorialPage.next()
            }
        }
        onDraggingChanged: tutorialPage.updateHints()

        // A page, sketched: a picture and a few lines of text under the cutout, over
        // the ambience as the start page is.
        Column {
            x: Theme.horizontalPageMargin
            y: tutorialPage.cutoutHeight + Theme.paddingLarge
            width: parent.width - 2 * Theme.horizontalPageMargin
            spacing: Theme.paddingLarge

            Rectangle {
                width: parent.width
                height: width / 2
                radius: Theme.paddingMedium
                color: Theme.rgba(Theme.primaryColor, Theme.opacityFaint)
            }

            Repeater {
                model: 5

                Rectangle {
                    width: parent.width * (index === 4 ? 0.6 : 1.0)
                    height: Theme.paddingLarge
                    radius: height / 2
                    color: Theme.rgba(Theme.primaryColor, Theme.opacityFaint)
                }
            }
        }

        // What an address typed into the bar offers: to go there, and to search for it.
        TutorialOmnibar {
            objectName: "tutorialOmnibar"
            width: parent.width
            y: tutorialPage.cutoutHeight
            height: bar.y - y
            visible: tutorialPage.step === "omnibar"
            typed: "sailfishos.org"
            address: "https://sailfishos.org"
        }

        TutorialBar {
            id: bar

            objectName: "tutorialBar"
            width: parent.width
            y: tutorialPage.height - height
            editing: tutorialPage.step === "omnibar"
            typed: "sailfishos.org"
            onTapped: {
                if (tutorialPage.step === "address" && region === "address"
                        || tutorialPage.step === "menu" && region === "menu") {
                    tutorialPage.next()
                }
            }
            onDragStarted: {
                if (tutorialPage.canOpen) {
                    deck.beginDrag()
                }
            }
            onDragMoved: {
                if (deck.dragging) {
                    deck.dragTo(distance)
                }
            }
            onDragFinished: {
                if (deck.dragging) {
                    deck.settle(distance > deck.pullThreshold)
                }
            }
        }

        grid: TutorialGrid {
            id: grid

            objectName: "tutorialGrid"
            anchors.fill: parent
            cutoutHeight: tutorialPage.cutoutHeight
            visible: deck.tabsOffset > 0
            onHandlingChanged: tutorialPage.updateHints()
            onPullStarted: {
                if (tutorialPage.canClose) {
                    deck.beginDrag()
                }
            }
            onPulled: {
                if (deck.dragging) {
                    deck.dragTo(tutorialPage.height - distance)
                }
            }
            onPullFinished: {
                if (deck.dragging) {
                    deck.settle(distance <= deck.pullThreshold)
                }
            }
            // A tap on a cell brings the page back, as it does in the browser, once
            // that is what is asked for.
            onTabTapped: {
                if (tutorialPage.canClose) {
                    deck.settle(false)
                }
            }
            onTabClosed: tutorialPage.tabGesture("closeTab")
            onTabMoved: tutorialPage.tabGesture("moveTab")
            onTabGrouped: tutorialPage.tabGesture("groupTab")
        }
    }

    TutorialMenu {
        id: menu

        objectName: "tutorialMenu"
        anchors.fill: parent
        open: tutorialPage.step === "menuOpen"
        onDismissed: tutorialPage.next()
    }

    TutorialHints {
        id: hints

        anchors.fill: parent
        bar: bar
        grid: grid
        menu: menu
    }

    // At the other end of the screen from the gesture, so the words do not cover it, and
    // put away while a finger is on the screen, as the Tutorial's own are.
    InteractionHintLabel {
        id: label

        readonly property bool atTop: ["closeTab", "moveTab", "close"].indexOf(tutorialPage.step) < 0

        objectName: "tutorialHintLabel"
        y: atTop ? 0 : parent.height - height
        invert: atTop
        opacity: tutorialPage.saying ? 1.0 : 0.0
        text: tutorialPage.stepText(tutorialPage.step)

        Behavior on opacity {
            FadeAnimation {
                duration: 1000
            }
        }
    }

    // Which lesson this is, just past the words, towards the middle of the screen; and
    // gone with them while a finger is down.
    TutorialProgress {
        objectName: "tutorialProgress"
        anchors.horizontalCenter: parent.horizontalCenter
        y: label.atTop ? label.y + label.height + Theme.paddingMedium
                       : label.y - height - Theme.paddingMedium
        count: 5
        current: Math.max(0, tutorialPage.lessonOf(tutorialPage.step))
        opacity: label.opacity
        visible: tutorialPage.lessonOf(tutorialPage.step) >= 0
    }

    // The explanation has no gesture to wait for: this goes on from it.
    Button {
        objectName: "tutorialContinueButton"
        anchors.centerIn: parent
        visible: tutorialPage.step === "omnibar"
        //: Goes on to the tutorial's next step
        text: qsTr("Continue")
        onClicked: tutorialPage.next()
    }

    TutorialCard {
        objectName: "tutorialWelcome"
        anchors.fill: parent
        opacity: tutorialPage.step === "welcome" ? 1.0 : 0.0
        showLogo: true
        // The application's name, which is not translated.
        heading: "Salama"
        //: Under the application's name on the tutorial's first card
        subheading: qsTr("Web browser for Sailfish OS")
        // What the lessons cover, by the icons the browser draws them with: the bar
        // searches, the menu is its button's, and the grid is the tabs'.
        topics: [
            //: What the tutorial covers: the address bar
            { "icon": "image://theme/icon-m-search", "text": qsTr("Address bar") },
            //: What the tutorial covers: the menu
            { "icon": "image://theme/icon-m-menu", "text": qsTr("Menu") },
            //: What the tutorial covers: the tabs and the grid of them
            { "icon": "image://theme/icon-m-tabs", "text": qsTr("Tabs") }
        ]

        Button {
            objectName: "tutorialStartButton"
            //: Starts the tutorial from its first card
            text: qsTr("Start tutorial")
            onClicked: tutorialPage.begin()
        }

        Button {
            objectName: "tutorialSkipButton"
            //: Leaves the tutorial from its first card, for the browser
            text: qsTr("Skip")
            onClicked: pageStack.pop()
        }
    }

    // A moment to see the page come back before the card covers it, as the Tutorial
    // waits after each lesson.
    Timer {
        id: recapDelay

        interval: 800
        onTriggered: tutorialPage.recapShown = true
    }

    TutorialCard {
        objectName: "tutorialRecap"
        anchors.fill: parent
        opacity: tutorialPage.recapShown ? 1.0 : 0.0
        showCheck: true
        heading: qsTr("Tutorial complete")
        text: qsTr("You can open it again from Settings.")

        Button {
            objectName: "tutorialCloseButton"
            //: Leaves the tutorial, for where it was opened from
            text: qsTr("Close tutorial")
            onClicked: pageStack.pop()
        }
    }
}
