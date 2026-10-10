// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Not a page: sits below browsing page in TabDeck. Way back = overscroll: grid shifts up by
// distance so content stays under finger while deck slides page back.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Item {
    id: tabsView

    signal pullStarted()
    signal pulled(real distance)
    signal pullFinished(real distance)
    signal tabActivated()

    // Silica PullDownMenu adds same to top margin in portrait.
    property real cutoutHeight: 0
    // Silica shrinks it while keyboard up; grid keeps full height.
    property real pageHeight: height
    readonly property GridCells cells: GridCells {
        width: tabsView.width
        height: tabsView.height
    }
    readonly property bool searching: searchField.text.length > 0
                                      && TabSearch.searchTerm.length > 0
    // Escaped term words, as Jolla contacts build for Theme.highlightText().
    readonly property var searchMatch: {
        var words = TabSearch.searchTerm.trim().split(/\s+/)
        var escaped = []
        for (var i = 0; i < words.length; ++i) {
            if (words[i].length > 0) {
                escaped.push(words[i].replace(/([.?*+^$[\]\\(){}|-])/g, "\\$1"))
            }
        }
        return escaped.length > 0 ? new RegExp(escaped.join("|"), "i") : null
    }

    objectName: "tabsView"
    // Unclipped, scrolled cells slide over page during head-row pull.
    clip: true

    function openFound(tabId) {
        TabModel.activateTabById(tabId)
        endSearch()
        tabActivated()
    }

    function endSearch() {
        searchField.text = ""
        searchField.focus = false
        searchDebounce.stop()
        TabSearch.searchTerm = ""
    }

    onVisibleChanged: {
        if (!visible) {
            endSearch()
        }
    }

    // Debounced, else list changes under finger as first results arrive.
    Timer {
        id: searchDebounce

        objectName: "searchDebounce"
        interval: 250
        onTriggered: TabSearch.searchTerm = searchField.text
    }

    // Reparented into grid: flickable filters only own children's presses, so as siblings
    // rows ate presses and page couldn't be pulled back.
    Component.onCompleted: {
        headRow.parent = tabGrid
        footRow.parent = tabGrid
    }

    SilicaGridView {
        id: tabGrid

        // Tracks finger only; flickable bounce-back would fight page animation.
        readonly property real overscroll: Math.max(0, originY - contentY)
        property real pullDistance: 0

        objectName: "tabGrid"
        width: parent.width
        height: parent.height
        y: -overscroll
        model: GroupTabs
        cellWidth: cells.cellWidth
        cellHeight: cells.cellHeight
        // Explicit vertical: automatic refuses drag when content fits.
        flickableDirection: Flickable.VerticalFlick
        boundsBehavior: Flickable.DragOverBounds

        onOverscrollChanged: {
            if (dragging) {
                pullDistance = overscroll
                tabsView.pulled(overscroll)
            }
        }
        onDragStarted: {
            pullDistance = 0
            tabsView.pullStarted()
        }
        onDragEnded: tabsView.pullFinished(pullDistance)

        header: Item {
            width: tabGrid.width
            height: headRow.height
        }

        footer: Item {
            width: tabGrid.width
            height: footRow.height
        }

        // By tab id: grid rows are current group's, model rows all groups'.
        delegate: TabPreview {
            dropTarget: groupStrip
            onHeldChanged: {
                if (!held) {
                    groupStrip.endCarry()
                }
            }
            onTapped: {
                TabModel.activateTabById(model.tabId)
                tabsView.tabActivated()
            }
            onCloseRequested: TabModel.closeTabById(model.tabId)
            onMoveRequested: GroupTabs.moveTab(from, to)
            onMuteToggled: PageMedia.toggleMuted(model.tabId)
        }

        ViewPlaceholder {
            enabled: GroupTabs.count === 0 && !tabsView.searching
            text: qsTr("No tabs in this group")
            hintText: qsTr("Open one with the button below")
        }

        VerticalScrollDecorator {}
    }

    // Rides grid (moves up on pull); margin compensates.
    Rectangle {
        id: headRow

        objectName: "gridHeadRow"
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            topMargin: tabGrid.overscroll
        }
        // Controls below cutout, not centred through notch.
        height: Theme.itemSizeLarge + tabsView.cutoutHeight
        color: Theme.highlightDimmerColor

        GlassTexture {
            objectName: "gridHeadGlass"
            anchors.fill: parent
        }

        Rectangle {
            objectName: "gridPullIndicator"
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
            }
            height: Theme.paddingSmall
            color: Theme.highlightBackgroundColor
        }

        Item {
            objectName: "gridHeadControls"
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }
            height: Theme.itemSizeLarge

            SearchField {
                id: searchField

                objectName: "tabSearchField"
                anchors {
                    left: parent.left
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                }
                placeholderText: qsTr("Search tabs")
                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false
                onTextChanged: searchDebounce.restart()
            }
        }

        // Drag down here is page's, however far grid scrolled.
        GridHeadGesture {
            objectName: "gridHeadPull"
            anchors.fill: parent
            field: searchField
            onPullStarted: tabsView.pullStarted()
            onPulled: tabsView.pulled(distance)
            onPullFinished: tabsView.pullFinished(distance)
        }
    }

    // Field not list header: Silica drops keyboard from field in flickable whose content
    // moves.
    SilicaListView {
        id: searchResults

        objectName: "tabSearchList"
        y: headRow.height
        width: parent.width
        // Above keyboard: grid layer is full height.
        height: Math.min(parent.height, tabsView.pageHeight) - headRow.height - footRow.height
        visible: tabsView.searching
        clip: true
        model: TabSearch

        delegate: TabSearchDelegate {
            match: tabsView.searchMatch
            onChosen: tabsView.openFound(model.tabId)
        }

        ViewPlaceholder {
            enabled: TabSearch.count === 0
            text: qsTr("No matching tabs")
        }

        VerticalScrollDecorator {}
    }

    Binding {
        target: tabGrid.contentItem
        property: "visible"
        value: !tabsView.searching
    }

    Rectangle {
        id: footRow

        objectName: "gridFootRow"
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            bottomMargin: -tabGrid.overscroll
        }
        height: Theme.itemSizeLarge
        color: Theme.highlightDimmerColor

        GlassTexture {
            objectName: "gridFootGlass"
            anchors.fill: parent
        }

        TabGroupStrip {
            id: groupStrip

            anchors.fill: parent
            onNewTabRequested: {
                TabModel.newTab("")
                tabsView.tabActivated()
            }
            onClosedTabsRequested: closedPanel.show()
            onEditRequested: pageStack.push(Qt.resolvedUrl("../pages/TabGroupsPage.qml"))
        }
    }

    RecentlyClosedPanel {
        id: closedPanel

        width: parent.width
        height: Math.round(parent.height * 0.6)
        onTabReopened: tabsView.tabActivated()
    }
}
