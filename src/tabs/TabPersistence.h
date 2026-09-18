// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Modelled on the tab handling of sailfish-browser apps/storage/dbworker.cpp and
// apps/history/persistenttabmodel.cpp (Copyright (c) 2013 - 2021 Jolla Ltd., MPL-2.0).
// The engine keeps each view's navigation history, so only the current page of every
// tab is stored here; sailfish-browser's link/tab_history tables are not needed.
#pragma once

#include "Tab.h"

#include <QList>
#include <QString>

namespace Salama {

class Storage;

class TabPersistence
{
public:
    explicit TabPersistence(Storage &storage);

    QList<Tab> loadTabs() const;
    int loadActiveTabId() const;

    // Private tabs are written like the rest, with their flag: the private group keeps
    // its tabs across a restart (docs/DECISIONS/0017-private-group.md).
    void insertTab(const Tab &tab);
    void updateTab(const Tab &tab);
    void removeTab(int tabId);
    // Rewrites position from the order of the list.
    void saveOrder(const QList<Tab> &tabs);
    void removeAllTabs();
    void setActiveTabId(int tabId);

    // Tab groups, in the order the strip shows them (docs/DECISIONS/0015-tab-groups.md).
    QList<TabGroup> loadGroups() const;
    int loadCurrentGroupId() const;
    void insertGroup(const TabGroup &group);
    void updateGroup(const TabGroup &group);
    void removeGroup(int groupId);
    // Rewrites position from the order of the list.
    void saveGroupOrder(const QList<TabGroup> &groups);
    void setCurrentGroupId(int groupId);

    // Recently closed tabs, newest first.
    QList<ClosedTab> loadClosedTabs() const;
    void insertClosedTab(const ClosedTab &closed);
    void removeClosedTab(int closedId);
    void removeAllClosedTabs();

private:
    Storage &m_storage;
};

} // namespace Salama
