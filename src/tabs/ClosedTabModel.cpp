// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "ClosedTabModel.h"

#include "TabModel.h"
#include "TabPersistence.h"

#include <QDateTime>
#include <algorithm>

namespace Salama {

ClosedTabModel::ClosedTabModel(TabModel *tabs, TabPersistence *persistence)
    : QAbstractListModel(tabs)
    , m_tabs(tabs)
    , m_persistence(persistence)
{
    if (m_persistence != nullptr) {
        m_closed = m_persistence->loadClosedTabs();
    }
    for (const ClosedTab &closed : m_closed) {
        m_nextId = std::max(m_nextId, closed.id + 1);
    }
}

int ClosedTabModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_closed.count();
}

QVariant ClosedTabModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_closed.count()) {
        return {};
    }
    const ClosedTab &closed = m_closed.at(index.row());
    switch (role) {
    case ClosedIdRole:
        return closed.id;
    case UrlRole:
        return closed.url;
    case TitleRole:
        return closed.title;
    case FaviconRole:
        return closed.favicon;
    default:
        return {};
    }
}

QHash<int, QByteArray> ClosedTabModel::roleNames() const
{
    return {
        {ClosedIdRole, QByteArrayLiteral("closedId")},
        {UrlRole, QByteArrayLiteral("url")},
        {TitleRole, QByteArrayLiteral("title")},
        {FaviconRole, QByteArrayLiteral("favicon")},
    };
}

int ClosedTabModel::count() const
{
    return m_closed.count();
}

const QList<ClosedTab> &ClosedTabModel::closedTabs() const
{
    return m_closed;
}

void ClosedTabModel::record(const Tab &tab)
{
    if (tab.isPrivate || tab.url.isEmpty()) {
        return;
    }
    ClosedTab closed;
    closed.id = m_nextId++;
    closed.url = tab.url;
    closed.title = tab.title;
    closed.favicon = tab.favicon;
    closed.closedAt = QDateTime::currentMSecsSinceEpoch();

    beginInsertRows(QModelIndex(), 0, 0);
    m_closed.prepend(closed);
    endInsertRows();
    if (m_persistence != nullptr) {
        m_persistence->insertClosedTab(closed);
    }

    // The oldest go once there are more than the panel has any use for.
    if (m_closed.count() > Limit) {
        const int last = m_closed.count() - 1;
        beginRemoveRows(QModelIndex(), Limit, last);
        while (m_closed.count() > Limit) {
            const ClosedTab dropped = m_closed.takeLast();
            if (m_persistence != nullptr) {
                m_persistence->removeClosedTab(dropped.id);
            }
        }
        endRemoveRows();
    }
    emit countChanged();
}

void ClosedTabModel::reopen(int row)
{
    if (row < 0 || row >= m_closed.count()) {
        return;
    }
    const ClosedTab closed = m_closed.at(row);
    beginRemoveRows(QModelIndex(), row, row);
    m_closed.removeAt(row);
    endRemoveRows();
    if (m_persistence != nullptr) {
        m_persistence->removeClosedTab(closed.id);
    }
    emit countChanged();

    // Last: opening the tab re-enters the tab model, and the grid's cell wants the
    // title and the icon before the page has loaded to say them itself. It was an
    // ordinary tab, so it comes back as one: in the current group, unless that is
    // the private group, and then in the default one.
    const int groupIndex = m_tabs->currentGroupIndex();
    if (groupIndex >= 0 && m_tabs->groups().at(groupIndex).isPrivate) {
        m_tabs->setCurrentGroupId(m_tabs->defaultGroupId());
    }
    const int tabId = m_tabs->newTab(closed.url);
    if (tabId > 0) {
        m_tabs->updateTitle(tabId, closed.title);
        m_tabs->updateFavicon(tabId, closed.favicon);
    }
}

void ClosedTabModel::clear()
{
    if (m_closed.isEmpty()) {
        return;
    }
    beginRemoveRows(QModelIndex(), 0, m_closed.count() - 1);
    m_closed.clear();
    endRemoveRows();
    if (m_persistence != nullptr) {
        m_persistence->removeAllClosedTabs();
    }
    emit countChanged();
}

} // namespace Salama
