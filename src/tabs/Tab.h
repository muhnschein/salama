// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Modelled on sailfish-browser apps/storage/tab.h (Copyright (c) 2013 Jolla Ltd., MPL-2.0),
// only fields WebView doesn't keep per view.
#pragma once

#include <QString>

namespace Salama {

struct Tab
{
    int id = 0;
    QString url;
    QString title;
    QString favicon;
    QString thumbnail;
    // Activation counter, not time: order only. 0 = not fronted since DB written.
    qint64 lastActive = 0;
    int groupId = 0;

    bool isValid() const
    {
        return id > 0;
    }

    friend bool operator==(const Tab &lhs, const Tab &rhs)
    {
        return lhs.id == rhs.id && lhs.url == rhs.url && lhs.title == rhs.title &&
               lhs.favicon == rhs.favicon && lhs.thumbnail == rhs.thumbnail &&
               lhs.lastActive == rhs.lastActive && lhs.groupId == rhs.groupId;
    }

    friend bool operator!=(const Tab &lhs, const Tab &rhs)
    {
        return !(lhs == rhs);
    }
};

// Empty name -> UI names by content ("3 tabs"). First group = default, never removed.
struct TabGroup
{
    int id = 0;
    QString name;

    bool isValid() const
    {
        return id > 0;
    }

    friend bool operator==(const TabGroup &lhs, const TabGroup &rhs)
    {
        return lhs.id == rhs.id && lhs.name == rhs.name;
    }

    friend bool operator!=(const TabGroup &lhs, const TabGroup &rhs)
    {
        return !(lhs == rhs);
    }
};

struct ClosedTab
{
    int id = 0;
    QString url;
    QString title;
    QString favicon;
    // ms since epoch.
    qint64 closedAt = 0;

    friend bool operator==(const ClosedTab &lhs, const ClosedTab &rhs)
    {
        return lhs.id == rhs.id && lhs.url == rhs.url && lhs.title == rhs.title &&
               lhs.favicon == rhs.favicon && lhs.closedAt == rhs.closedAt;
    }

    friend bool operator!=(const ClosedTab &lhs, const ClosedTab &rhs)
    {
        return !(lhs == rhs);
    }
};

} // namespace Salama
