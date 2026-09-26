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
    explicit TabPersistence(const Storage &storage);

    QList<Tab> loadTabs() const;
    int loadActiveTabId() const;

    void insertTab(const Tab &tab) const;
    void updateTab(const Tab &tab) const;
    void removeTab(int tabId) const;
    // Rewrites position from the order of the list.
    void saveOrder(const QList<Tab> &tabs) const;
    void removeAllTabs() const;
    void setActiveTabId(int tabId) const;

    // Tab groups, in the order the strip shows them (docs/DECISIONS/0015-tab-groups.md).
    QList<TabGroup> loadGroups() const;
    int loadCurrentGroupId() const;
    void insertGroup(const TabGroup &group) const;
    void updateGroup(const TabGroup &group) const;
    void removeGroup(int groupId) const;
    // Rewrites position from the order of the list.
    void saveGroupOrder(const QList<TabGroup> &groups) const;
    void setCurrentGroupId(int groupId) const;

    // Recently closed tabs, newest first.
    QList<ClosedTab> loadClosedTabs() const;
    void insertClosedTab(const ClosedTab &closed) const;
    void removeClosedTab(int closedId) const;
    void removeAllClosedTabs() const;

private:
    const Storage &m_storage;
};

} // namespace Salama
