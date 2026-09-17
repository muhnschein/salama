// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 tuuli contributors
//
// Modelled on sailfish-browser apps/storage/tab.h (Copyright (c) 2013 Jolla Ltd., MPL-2.0),
// reduced to the fields the platform WebView does not already keep per view.
#pragma once

#include <QString>

namespace Tuuli {

struct Tab
{
    int id = 0;
    QString url;
    QString title;
    QString favicon;
    // Absolute path to the last captured page preview, empty when there is none.
    QString thumbnail;
    // When this tab was last the active one, on the model's own activation clock: a
    // counter, not a time, because all the order needs is which came after which. Zero
    // for a tab that has not been in front since the database was written. The cover
    // reads it (docs/DECISIONS/0014-cover-is-the-tab-count.md).
    qint64 lastActive = 0;
    // The tab group this tab belongs to (docs/DECISIONS/0015-tab-groups.md). Every tab
    // is in exactly one; the model gives a tab the group that was current when it was
    // opened, and repairs a stored id that names no group on load.
    int groupId = 0;
    bool isPrivate = false;

    bool isValid() const
    {
        return id > 0;
    }

    bool operator==(const Tab &other) const
    {
        return id == other.id && url == other.url && title == other.title &&
               favicon == other.favicon && thumbnail == other.thumbnail &&
               lastActive == other.lastActive && groupId == other.groupId &&
               isPrivate == other.isPrivate;
    }

    bool operator!=(const Tab &other) const
    {
        return !(*this == other);
    }
};

// A tab group: a name and a place in the order the strip shows them in. The name may
// be empty, in which case the interface names the group by what it holds -- "3 tabs"
// -- the way Safari names its ungrouped tabs.
struct TabGroup
{
    int id = 0;
    QString name;

    bool isValid() const
    {
        return id > 0;
    }

    bool operator==(const TabGroup &other) const
    {
        return id == other.id && name == other.name;
    }

    bool operator!=(const TabGroup &other) const
    {
        return !(*this == other);
    }
};

} // namespace Tuuli
