// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "TabPersistence.h"

#include "storage/Storage.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QtDebug>

namespace Salama {

namespace {

const char *const ActiveTabSetting = "activeTabId";
const char *const CurrentGroupSetting = "currentGroupId";

bool run(QSqlQuery &query)
{
    if (!query.exec()) {
        qWarning() << "TabPersistence:" << query.lastError().text() << query.lastQuery();
        return false;
    }
    return true;
}

int readSetting(const QSqlDatabase &db, const char *name)
{
    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT value FROM setting WHERE name = ?"));
    query.addBindValue(QLatin1String(name));
    if (run(query) && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

void writeSetting(const QSqlDatabase &db, const char *name, int value)
{
    QSqlQuery query(db);
    query.prepare(QStringLiteral("INSERT OR REPLACE INTO setting (name, value) VALUES (?, ?)"));
    query.addBindValue(QLatin1String(name));
    query.addBindValue(QString::number(value));
    run(query);
}

} // namespace

TabPersistence::TabPersistence(const Storage &storage)
    : m_storage(storage)
{
}

QList<Tab> TabPersistence::loadTabs() const
{
    QList<Tab> tabs;
    QSqlQuery query(m_storage.database());
    query.prepare(QStringLiteral("SELECT tab_id, url, title, favicon, thumbnail, last_active, "
                                 "group_id FROM tab ORDER BY position ASC"));
    if (!run(query)) {
        return tabs;
    }
    while (query.next()) {
        Tab tab;
        tab.id = query.value(0).toInt();
        tab.url = query.value(1).toString();
        tab.title = query.value(2).toString();
        tab.favicon = query.value(3).toString();
        tab.thumbnail = query.value(4).toString();
        tab.lastActive = query.value(5).toLongLong();
        tab.groupId = query.value(6).toInt();
        tabs.append(tab);
    }
    return tabs;
}

int TabPersistence::loadActiveTabId() const
{
    return readSetting(m_storage.database(), ActiveTabSetting);
}

void TabPersistence::insertTab(const Tab &tab) const
{
    if (!tab.isValid()) {
        return;
    }
    QSqlQuery query(m_storage.database());
    query.prepare(
        QStringLiteral("INSERT INTO tab (tab_id, position, url, title, favicon, thumbnail, "
                       "last_active, group_id) "
                       "VALUES (?, (SELECT COALESCE(MAX(position), 0) + 1 FROM tab), "
                       "?, ?, ?, ?, ?, ?)"));
    query.addBindValue(tab.id);
    query.addBindValue(Storage::text(tab.url));
    query.addBindValue(Storage::text(tab.title));
    query.addBindValue(Storage::text(tab.favicon));
    query.addBindValue(Storage::text(tab.thumbnail));
    query.addBindValue(tab.lastActive);
    query.addBindValue(tab.groupId);
    run(query);
}

void TabPersistence::updateTab(const Tab &tab) const
{
    if (!tab.isValid()) {
        return;
    }
    QSqlQuery query(m_storage.database());
    query.prepare(QStringLiteral("UPDATE tab SET url = ?, title = ?, favicon = ?, "
                                 "thumbnail = ?, last_active = ?, group_id = ? "
                                 "WHERE tab_id = ?"));
    query.addBindValue(Storage::text(tab.url));
    query.addBindValue(Storage::text(tab.title));
    query.addBindValue(Storage::text(tab.favicon));
    query.addBindValue(Storage::text(tab.thumbnail));
    query.addBindValue(tab.lastActive);
    query.addBindValue(tab.groupId);
    query.addBindValue(tab.id);
    run(query);
}

void TabPersistence::saveOrder(const QList<Tab> &tabs) const
{
    // Numbered from 1 so that insertTab's MAX(position) + 1 still lands last.
    int position = 0;
    for (const Tab &tab : tabs) {
        if (!tab.isValid()) {
            continue;
        }
        ++position;
        QSqlQuery query(m_storage.database());
        query.prepare(QStringLiteral("UPDATE tab SET position = ? WHERE tab_id = ?"));
        query.addBindValue(position);
        query.addBindValue(tab.id);
        run(query);
    }
}

void TabPersistence::removeTab(int tabId) const
{
    QSqlQuery query(m_storage.database());
    query.prepare(QStringLiteral("DELETE FROM tab WHERE tab_id = ?"));
    query.addBindValue(tabId);
    run(query);
}

void TabPersistence::removeAllTabs() const
{
    QSqlQuery query(m_storage.database());
    query.prepare(QStringLiteral("DELETE FROM tab"));
    run(query);
}

void TabPersistence::setActiveTabId(int tabId) const
{
    writeSetting(m_storage.database(), ActiveTabSetting, tabId);
}

QList<TabGroup> TabPersistence::loadGroups() const
{
    QList<TabGroup> groups;
    QSqlQuery query(m_storage.database());
    query.prepare(QStringLiteral("SELECT group_id, name FROM tab_group ORDER BY position ASC"));
    if (!run(query)) {
        return groups;
    }
    while (query.next()) {
        TabGroup group;
        group.id = query.value(0).toInt();
        group.name = query.value(1).toString();
        groups.append(group);
    }
    return groups;
}

int TabPersistence::loadCurrentGroupId() const
{
    return readSetting(m_storage.database(), CurrentGroupSetting);
}

void TabPersistence::insertGroup(const TabGroup &group) const
{
    if (!group.isValid()) {
        return;
    }
    QSqlQuery query(m_storage.database());
    query.prepare(QStringLiteral("INSERT INTO tab_group (group_id, name, position) "
                                 "VALUES (?, ?, (SELECT COALESCE(MAX(position), 0) + 1 "
                                 "FROM tab_group))"));
    query.addBindValue(group.id);
    query.addBindValue(Storage::text(group.name));
    run(query);
}

void TabPersistence::updateGroup(const TabGroup &group) const
{
    if (!group.isValid()) {
        return;
    }
    QSqlQuery query(m_storage.database());
    query.prepare(QStringLiteral("UPDATE tab_group SET name = ? WHERE group_id = ?"));
    query.addBindValue(Storage::text(group.name));
    query.addBindValue(group.id);
    run(query);
}

void TabPersistence::removeGroup(int groupId) const
{
    QSqlQuery query(m_storage.database());
    query.prepare(QStringLiteral("DELETE FROM tab_group WHERE group_id = ?"));
    query.addBindValue(groupId);
    run(query);
}

void TabPersistence::saveGroupOrder(const QList<TabGroup> &groups) const
{
    int position = 0;
    for (const TabGroup &group : groups) {
        if (!group.isValid()) {
            continue;
        }
        ++position;
        QSqlQuery query(m_storage.database());
        query.prepare(QStringLiteral("UPDATE tab_group SET position = ? WHERE group_id = ?"));
        query.addBindValue(position);
        query.addBindValue(group.id);
        run(query);
    }
}

void TabPersistence::setCurrentGroupId(int groupId) const
{
    writeSetting(m_storage.database(), CurrentGroupSetting, groupId);
}

QList<ClosedTab> TabPersistence::loadClosedTabs() const
{
    QList<ClosedTab> closedTabs;
    QSqlQuery query(m_storage.database());
    query.prepare(QStringLiteral("SELECT id, url, title, favicon, closed FROM closed_tab "
                                 "ORDER BY closed DESC, id DESC"));
    if (!run(query)) {
        return closedTabs;
    }
    while (query.next()) {
        ClosedTab closed;
        closed.id = query.value(0).toInt();
        closed.url = query.value(1).toString();
        closed.title = query.value(2).toString();
        closed.favicon = query.value(3).toString();
        closed.closedAt = query.value(4).toLongLong();
        closedTabs.append(closed);
    }
    return closedTabs;
}

void TabPersistence::insertClosedTab(const ClosedTab &closed) const
{
    if (closed.id <= 0) {
        return;
    }
    QSqlQuery query(m_storage.database());
    query.prepare(QStringLiteral("INSERT INTO closed_tab (id, url, title, favicon, closed) "
                                 "VALUES (?, ?, ?, ?, ?)"));
    query.addBindValue(closed.id);
    query.addBindValue(Storage::text(closed.url));
    query.addBindValue(Storage::text(closed.title));
    query.addBindValue(Storage::text(closed.favicon));
    query.addBindValue(closed.closedAt);
    run(query);
}

void TabPersistence::removeClosedTab(int closedId) const
{
    QSqlQuery query(m_storage.database());
    query.prepare(QStringLiteral("DELETE FROM closed_tab WHERE id = ?"));
    query.addBindValue(closedId);
    run(query);
}

void TabPersistence::removeAllClosedTabs() const
{
    QSqlQuery query(m_storage.database());
    query.prepare(QStringLiteral("DELETE FROM closed_tab"));
    run(query);
}

} // namespace Salama
