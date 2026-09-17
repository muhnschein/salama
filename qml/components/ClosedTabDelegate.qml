// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 tuuli contributors
//
// One recently closed tab, as the search results draw theirs.
import QtQuick 2.6
import Sailfish.Silica 1.0

TabRow {
    objectName: "closedTabDelegate"
    width: ListView.view.width
    title: model.title
    subtitle: model.url
    icon: model.favicon
}
