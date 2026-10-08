// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
// Real TabDeck, bar gesture/handle and grid cells, so gestures behave as in browser;
// content is sketch, no real tabs touched. Each step waits for its gesture.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

Page {
    id: tutorialPage

    property bool welcome: false
    // "welcome", then a lesson step, then "done".
    property string step: welcome ? "welcome" : lessons[0]
    readonly property var lessons: ["address", "omnibar", "menu", "menuOpen", "open", "closeTab",
                                    "moveTab", "groupTab", "close"]
    property bool recapShown: false
    // Only step asking for it moves deck; free once done.
    readonly property bool canOpen: step === "open" || step === "done"
    readonly property bool canClose: step === "close" || step === "done"
    readonly property bool hinting: hintShown()
    // Also during pane explanation (no hint).
    readonly property bool saying: hinting || step === "omnibar"

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

    // Out-of-turn gesture only changes sketch; sketch resets if too few tabs remain.
    function tabGesture(forStep) {
        if (step === forStep) {
            next()
        } else if (grid.count < 2) {
            grid.reset()
        }
    }

    function hintKind(forStep) {
        if (forStep === "address" || forStep === "menu" || forStep === "menuOpen") {
            return "tap"
        }
        if (forStep === "omnibar" || lessons.indexOf(forStep) < 0) {
            return ""
        }
        return "touch"
    }

    // Not under finger already gesturing. Read state directly, not via binding: handlers
    // query on state change before binding updates.
    function hintShown() {
        return hintKind(step) !== "" && !deck.dragging && !grid.handling
                && status === PageStatus.Active
    }

    function updateHints() {
        if (hintShown()) {
            hints.show(step)
        } else {
            hints.stop()
        }
    }

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

    // Opposite end from gesture so words don't cover it.
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

    // Explanation step has no gesture: this advances it.
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
        // Not translated.
        heading: "Salama"
        //: Under the application's name on the tutorial's first card
        subheading: qsTr("Web browser for Sailfish OS")
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
