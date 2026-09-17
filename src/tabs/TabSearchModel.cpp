// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 tuuli contributors
#include "TabSearchModel.h"

#include "TabModel.h"

namespace Tuuli {

TabSearchModel::TabSearchModel(TabModel *tabs, QObject *parent)
    : QAbstractListModel(parent)
    , m_tabs(tabs)
{
    // Rows come and go with the tabs; a title or address arriving can change whether
    // a row matches, and a group's name or order is part of what every row says.
    connect(m_tabs, &TabModel::countChanged, this, &TabSearchModel::rebuild);
    connect(m_tabs, &TabModel::rowsMoved, this, &TabSearchModel::rebuild);
    connect(m_tabs, &TabModel::dataChanged, this, &TabSearchModel::rebuild);
    connect(m_tabs, &TabModel::groupsChanged, this, &TabSearchModel::rebuild);
    rebuild();
}

int TabSearchModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_rows.count();
}

QVariant TabSearchModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_rows.count()) {
        return {};
    }
    const Row &row = m_rows.at(index.row());
    const int tabIndex = m_tabs->indexOf(row.tabId);
    if (tabIndex < 0) {
        return {};
    }
    const Tab &tab = m_tabs->tabs().at(tabIndex);
    switch (role) {
    case TabIdRole:
        return tab.id;
    case UrlRole:
        return tab.url;
    case TitleRole:
        return tab.title;
    case FaviconRole:
        return tab.favicon;
    case PrivateRole:
        return tab.isPrivate;
    case GroupIdRole:
        return row.groupId;
    case GroupNameRole: {
        const int groupIndex = m_tabs->groupIndexOf(row.groupId);
        return groupIndex >= 0 ? m_tabs->groups().at(groupIndex).name : QString();
    }
    case GroupTabCountRole:
        return m_tabs->tabCountInGroup(row.groupId);
    case GroupStartRole:
        return row.groupStart;
    default:
        return {};
    }
}

QHash<int, QByteArray> TabSearchModel::roleNames() const
{
    return {
        {TabIdRole, QByteArrayLiteral("tabId")},
        {UrlRole, QByteArrayLiteral("url")},
        {TitleRole, QByteArrayLiteral("title")},
        {FaviconRole, QByteArrayLiteral("favicon")},
        {PrivateRole, QByteArrayLiteral("privateTab")},
        {GroupIdRole, QByteArrayLiteral("groupId")},
        {GroupNameRole, QByteArrayLiteral("groupName")},
        {GroupTabCountRole, QByteArrayLiteral("groupTabCount")},
        {GroupStartRole, QByteArrayLiteral("groupStart")},
    };
}

QString TabSearchModel::searchTerm() const
{
    return m_searchTerm;
}

void TabSearchModel::setSearchTerm(const QString &term)
{
    const QString trimmed = term.trimmed();
    if (trimmed == m_searchTerm) {
        return;
    }
    m_searchTerm = trimmed;
    emit searchTermChanged();
    rebuild();
}

int TabSearchModel::count() const
{
    return m_rows.count();
}

bool TabSearchModel::matches(int tabIndex) const
{
    if (m_searchTerm.isEmpty()) {
        return true;
    }
    const Tab &tab = m_tabs->tabs().at(tabIndex);
    return tab.title.contains(m_searchTerm, Qt::CaseInsensitive) ||
           tab.url.contains(m_searchTerm, Qt::CaseInsensitive);
}

void TabSearchModel::rebuild()
{
    const int before = m_rows.count();
    beginResetModel();
    m_rows.clear();
    const QList<Tab> &tabs = m_tabs->tabs();
    for (const TabGroup &group : m_tabs->groups()) {
        bool first = true;
        for (int i = 0; i < tabs.count(); ++i) {
            if (tabs.at(i).groupId != group.id || !matches(i)) {
                continue;
            }
            m_rows.append(Row{tabs.at(i).id, group.id, first});
            first = false;
        }
    }
    endResetModel();
    if (m_rows.count() != before) {
        emit countChanged();
    }
}

} // namespace Tuuli
