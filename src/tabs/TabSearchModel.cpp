// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "TabSearchModel.h"

#include "TabModel.h"

namespace Salama {

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
    case GroupPrivateRole: {
        const int groupIndex = m_tabs->groupIndexOf(row.groupId);
        return groupIndex >= 0 && m_tabs->groups().at(groupIndex).isPrivate;
    }
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
        {GroupPrivateRole, QByteArrayLiteral("groupPrivate")},
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
    refine();
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

QList<TabSearchModel::Row> TabSearchModel::rowsForTerm() const
{
    QList<Row> rows;
    const QList<Tab> &tabs = m_tabs->tabs();
    for (const TabGroup &group : m_tabs->groups()) {
        bool first = true;
        for (int i = 0; i < tabs.count(); ++i) {
            if (tabs.at(i).groupId != group.id || !matches(i)) {
                continue;
            }
            rows.append(Row{tabs.at(i).id, group.id, first});
            first = false;
        }
    }
    return rows;
}

// The tabs or the groups changed: whatever the list was, it is built again.
void TabSearchModel::rebuild()
{
    const int before = m_rows.count();
    beginResetModel();
    m_rows = rowsForTerm();
    endResetModel();
    if (m_rows.count() != before) {
        emit countChanged();
    }
}

// Only the term changed, so the old rows and the new are both drawn from the same
// tabs in the same order: walking the two together, a row is kept, removed or
// inserted, and the list under the reader's finger is never rebuilt around a
// keystroke. The headings move with the rows: the first row of a group is told when
// it stops or starts being one.
void TabSearchModel::refine()
{
    const int before = m_rows.count();
    const QList<Row> wanted = rowsForTerm();
    auto wantedFrom = [&wanted](int from, int tabId) {
        for (int k = from; k < wanted.count(); ++k) {
            if (wanted.at(k).tabId == tabId) {
                return true;
            }
        }
        return false;
    };
    int have = 0;
    for (int want = 0; want < wanted.count(); ++want) {
        // Rows the new list no longer has come out first, in order.
        while (have < m_rows.count() && !wantedFrom(want, m_rows.at(have).tabId)) {
            beginRemoveRows(QModelIndex(), have, have);
            m_rows.removeAt(have);
            endRemoveRows();
        }
        if (have < m_rows.count() && m_rows.at(have).tabId == wanted.at(want).tabId) {
            if (m_rows.at(have).groupStart != wanted.at(want).groupStart) {
                m_rows[have].groupStart = wanted.at(want).groupStart;
                const QModelIndex changed = index(have, 0);
                emit dataChanged(changed, changed, QVector<int>{GroupStartRole});
            }
            ++have;
            continue;
        }
        beginInsertRows(QModelIndex(), have, have);
        m_rows.insert(have, wanted.at(want));
        endInsertRows();
        ++have;
    }
    if (have < m_rows.count()) {
        beginRemoveRows(QModelIndex(), have, m_rows.count() - 1);
        while (m_rows.count() > have) {
            m_rows.removeLast();
        }
        endRemoveRows();
    }
    if (m_rows.count() != before) {
        emit countChanged();
    }
}

} // namespace Salama
