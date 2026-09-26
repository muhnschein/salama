// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The tutorial: the one gesture of this browser's that no other Sailfish application
// has, the tab grid dragged up from under the page and pulled back down, taught as the
// platform's own Tutorial teaches the app grid it opens from the bottom edge and closes
// from the top -- on a picture of the screen that answers the finger, with Silica's
// TouchInteractionHint showing the movement and an InteractionHintLabel saying it, one
// step at a time, each waiting for the finger to have done it. A card says what the
// lesson is for before it, and well done after it (docs/DECISIONS/0034-tutorial.md).
//
// The deck is the real one (components/TabDeck.qml), and so are the bar's gesture and
// its handle, so the drag is caught, followed and released here as it is on the page.
// What the deck carries is a sketch of a page and of the grid: nothing done here opens,
// moves or closes a tab. Back leaves it at any step, as it leaves any page.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

Page {
    id: tutorialPage

    // Where the lesson is: "intro", the card that says what it is for; "open", the bar
    // to drag up; "close", the grid to pull back down; "done", and the card that says so
    // once the page has been seen to come back.
    property string step: "intro"
    // The card after the lesson is up: a moment after it is done, so the page is seen to
    // come back before the card covers it, as the Tutorial waits after each lesson.
    property bool recapShown: false
    // The movement is shown, and said, while a gesture is waited for, and not under a
    // finger already making it; a finger lifted short of the threshold is shown it again.
    readonly property bool hinting: (step === "open" || step === "close") && !deck.dragging
                                    && status === PageStatus.Active

    // What the display's cutout takes at the top of the screen, which the sketch keeps
    // out of as the browsing page does, while Settings says so.
    readonly property real cutoutHeight: Settings.cutoutGuard && Screen.topCutout
                                         ? Math.max(0, Screen.topCutout.y + Screen.topCutout.height)
                                         : 0

    objectName: "tutorialPage"
    allowedOrientations: Orientation.Portrait

    function begin() {
        recapShown = false
        deck.settle(false)
        step = "open"
    }

    // A step is done when the deck settles where it asked for. A drag released short
    // of the threshold springs back and leaves the step as it was.
    function advance() {
        if (step === "open" && deck.tabsOpen) {
            step = "close"
        } else if (step === "close" && !deck.tabsOpen) {
            step = "done"
            recapDelay.restart()
        }
    }

    // The step moves on as a drag settles, with the finger lifted: the hint starts again
    // going the new step's way.
    onHintingChanged: {
        if (hinting) {
            hint.direction = step === "open" ? TouchInteraction.Up : TouchInteraction.Down
            hint.start()
        } else {
            hint.stop()
        }
    }

    TabDeck {
        id: deck

        objectName: "tutorialDeck"
        width: parent.width
        pageHeight: tutorialPage.height
        onTabsOpenChanged: tutorialPage.advance()

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

        TutorialBar {
            id: bar

            objectName: "tutorialBar"
            width: parent.width
            y: tutorialPage.height - height
            onDragStarted: deck.beginDrag()
            onDragMoved: deck.dragTo(distance)
            onDragFinished: deck.settle(distance > deck.pullThreshold)
        }

        grid: TutorialGrid {
            objectName: "tutorialGrid"
            anchors.fill: parent
            cutoutHeight: tutorialPage.cutoutHeight
            visible: deck.tabsOffset > 0
            onPullStarted: deck.beginDrag()
            onPulled: deck.dragTo(tutorialPage.height - distance)
            onPullFinished: deck.settle(distance <= deck.pullThreshold)
        }
    }

    // Where the finger is to go: up from the handle on the bar's edge, which is what the
    // browsing page's drag is aimed at, and down from the upper third of the grid, where
    // Silica starts a pull of its own.
    TouchInteractionHint {
        id: hint

        objectName: "tutorialHint"
        interactionMode: TouchInteraction.Pull
        loops: Animation.Infinite
        startY: direction === TouchInteraction.Up ? tutorialPage.height - bar.height - height / 2
                                                  : tutorialPage.height / 3 - height / 2
    }

    // At the other end of the screen from the movement, so the words do not cover it,
    // and put away while a finger is on the screen, as the Tutorial's own are.
    InteractionHintLabel {
        objectName: "tutorialHintLabel"
        y: tutorialPage.step === "open" ? 0 : parent.height - height
        invert: tutorialPage.step === "open"
        opacity: tutorialPage.hinting ? 1.0 : 0.0
        text: tutorialPage.step === "open"
              //: The tutorial's first step: the navigation bar at the foot of the screen
              //: is dragged upwards, and the grid of open tabs comes up from under the page
              ? qsTr("Drag the bar up to see your tabs")
              //: The tutorial's second step: the grid of tabs is pulled down past its top
              //: to bring the page back
              : qsTr("Pull down to go back to the page")

        Behavior on opacity {
            FadeAnimation {
                duration: 1000
            }
        }
    }

    TutorialCard {
        objectName: "tutorialIntro"
        anchors.fill: parent
        opacity: tutorialPage.step === "intro" ? 1.0 : 0.0
        //: The tutorial's first card: what it teaches
        heading: qsTr("Learn where your tabs are")
        //: Under the tutorial's heading: the grid of tabs is reached by a gesture, which
        //: the tutorial has the reader make on a picture of the browser
        text: qsTr("salama has no tabs button: your tabs lie under the page. Follow the instructions on the screen to learn the gesture that brings them up.")

        Button {
            objectName: "tutorialStartButton"
            //: Starts the tutorial's lesson
            text: qsTr("Start")
            onClicked: tutorialPage.begin()
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
        //: The tutorial's lesson has been done
        heading: qsTr("Well done!")
        //: What the tutorial taught, said back to the reader at its end
        text: qsTr("Now you know where your tabs are: under the page, one drag up from the bar.")

        Button {
            objectName: "tutorialAgainButton"
            //: Starts the tutorial's lesson over
            text: qsTr("Try again")
            onClicked: tutorialPage.begin()
        }

        Button {
            objectName: "tutorialCloseButton"
            //: Leaves the tutorial, for where it was opened from
            text: qsTr("Close tutorial")
            onClicked: pageStack.pop()
        }
    }
}
