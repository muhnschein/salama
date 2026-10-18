// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Not a page: sits below browsing page in TabDeck. Way back = overscroll: grid shifts up by
// distance so content stays under finger while deck slides page back. Landscape: no head or
// foot row; search field and groups in a rail left of grid.
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
    readonly property bool wide: width > height
    readonly property real railWidth: wide ? Math.round(width / 5) : 0
    readonly property GridCells cells: GridCells {
        width: tabsView.width - tabsView.railWidth
        height: tabsView.height
        pageWidth: tabsView.width
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

    // Under grid, so a carried cell shows over the group it is dropped on.
    Rectangle {
        id: rail

        objectName: "tabGroupRail"
        width: tabsView.railWidth
        height: parent.height
        visible: tabsView.wide
        color: Theme.highlightDimmerColor

        GlassTexture {
            objectName: "railGlass"
            anchors.fill: parent
        }

        Item {
            id: railHead

            objectName: "railHead"
            width: parent.width
            height: Theme.itemSizeSmall

            // z: field is reparented in after it.
            GridHeadGesture {
                objectName: "railHeadPull"
                anchors.fill: parent
                z: 1
                field: searchField
                onPullStarted: tabsView.pullStarted()
                onPulled: tabsView.pulled(distance)
                onPullFinished: tabsView.pullFinished(distance)
            }
        }

        Item {
            id: railGroups

            anchors {
                left: parent.left
                right: parent.right
                top: railHead.bottom
                bottom: parent.bottom
            }
        }
    }

    SilicaGridView {
        id: tabGrid

        // Tracks finger only; flickable bounce-back would fight page animation.
        readonly property real overscroll: Math.max(0, originY - contentY)
        property real pullDistance: 0

        objectName: "tabGrid"
        x: tabsView.railWidth
        width: parent.width - x
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
            height: tabsView.wide ? 0 : headRow.height
        }

        footer: Item {
            width: tabGrid.width
            height: tabsView.wide ? 0 : footRow.height
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
        visible: !tabsView.wide
        color: Theme.highlightDimmerColor

        GlassTexture {
            objectName: "gridHeadGlass"
            anchors.fill: parent
        }

        // Landscape: along whole top, over rail too.
        Rectangle {
            objectName: "gridPullIndicator"
            parent: tabsView.wide ? tabsView : headRow
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
            parent: tabsView.wide ? railHead : headRow
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }
            height: tabsView.wide ? railHead.height : Theme.itemSizeLarge

            SearchField {
                id: searchField

                // Silica's own placeholder, "Search" in Silica's catalogs: rail is narrow.
                property string searchWord

                objectName: "tabSearchField"
                anchors {
                    left: parent.left
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                }
                font.pixelSize: tabsView.wide ? Theme.fontSizeMedium : Theme.fontSizeLarge
                leftItem: Icon {
                    width: tabsView.wide ? Theme.iconSizeSmallPlus : Theme.iconSizeMedium
                    height: width
                    sourceSize: Qt.size(width, height)
                    source: "image://theme/icon-m-search"
                }
                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false
                onTextChanged: searchDebounce.restart()
                Component.onCompleted: {
                    searchWord = placeholderText
                    placeholderText = Qt.binding(function() {
                        return tabsView.wide ? searchWord : qsTr("Search tabs")
                    })
                }
            }
        }

        // Drag down here is page's, however far grid scrolled. z: controls come back from rail
        // after it.
        GridHeadGesture {
            objectName: "gridHeadPull"
            anchors.fill: parent
            z: 1
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
        visible: tabsView.searching && !tabsView.wide
        clip: true
        // Only while searching: model resets on every tab change, a hidden list made all its
        // rows again each time (several per page load, measured).
        model: tabsView.searching && !tabsView.wide ? TabSearch : null

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

    // Landscape: two columns of compact rows beside rail, above keyboard.
    SilicaGridView {
        objectName: "tabSearchGrid"
        x: tabsView.railWidth
        width: parent.width - x
        height: Math.min(parent.height, tabsView.pageHeight)
        visible: tabsView.searching && tabsView.wide
        clip: true
        model: tabsView.searching && tabsView.wide ? TabSearch : null
        cellWidth: width / 2
        cellHeight: Theme.itemSizeSmall

        delegate: TabSearchDelegate {
            compact: true
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
        visible: !tabsView.wide
        color: Theme.highlightDimmerColor

        GlassTexture {
            objectName: "gridFootGlass"
            anchors.fill: parent
        }

        TabGroupStrip {
            id: groupStrip

            parent: tabsView.wide ? railGroups : footRow
            anchors.fill: parent
            vertical: tabsView.wide
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
