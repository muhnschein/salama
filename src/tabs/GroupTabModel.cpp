// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "GroupTabModel.h"

#include "TabModel.h"

namespace Salama {

GroupTabModel::GroupTabModel(TabModel *tabs)
    : QAbstractListModel(tabs)
    , m_tabs(tabs)
{
}

int GroupTabModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_tabIds.count();
}

QVariant GroupTabModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_tabIds.count()) {
        return {};
    }
    const int source = m_tabs->indexOf(m_tabIds.at(index.row()));
    return m_tabs->data(m_tabs->index(source, 0), role);
}

QHash<int, QByteArray> GroupTabModel::roleNames() const
{
    return m_tabs->roleNames();
}

int GroupTabModel::count() const
{
    return m_tabIds.count();
}

int GroupTabModel::tabIdAt(int row) const
{
    return row >= 0 && row < m_tabIds.count() ? m_tabIds.at(row) : 0;
}

int GroupTabModel::rowOf(int tabId) const
{
    return m_tabIds.indexOf(tabId);
}

void GroupTabModel::moveTab(int from, int to)
{
    const int last = m_tabIds.count() - 1;
    if (from == to || from < 0 || from > last || to < 0 || to > last) {
        return;
    }
    // The tab model moves the row and reports the move back here through moveTabRow():
    // one place decides the order, and the grid's picture of it follows.
    m_tabs->moveTab(m_tabs->indexOf(m_tabIds.at(from)), m_tabs->indexOf(m_tabIds.at(to)));
}

void GroupTabModel::reset(const QList<int> &tabIds)
{
    beginResetModel();
    m_tabIds = tabIds;
    endResetModel();
    emit countChanged();
}

void GroupTabModel::append(int tabId)
{
    const int row = m_tabIds.count();
    beginInsertRows(QModelIndex(), row, row);
    m_tabIds.append(tabId);
    endInsertRows();
    emit countChanged();
}

void GroupTabModel::remove(int tabId)
{
    const int row = m_tabIds.indexOf(tabId);
    if (row < 0) {
        return;
    }
    beginRemoveRows(QModelIndex(), row, row);
    m_tabIds.removeAt(row);
    endRemoveRows();
    emit countChanged();
}

void GroupTabModel::moveTabRow(int from, int to)
{
    const int last = m_tabIds.count() - 1;
    if (from == to || from < 0 || from > last || to < 0 || to > last) {
        return;
    }
    // beginMoveRows wants the row the block lands *before*, one past the destination
    // when moving down the list.
    const int destination = to > from ? to + 1 : to;
    if (!beginMoveRows(QModelIndex(), from, from, QModelIndex(), destination)) {
        return;
    }
    m_tabIds.move(from, to);
    endMoveRows();
}

void GroupTabModel::changed(int tabId, int role)
{
    const int row = m_tabIds.indexOf(tabId);
    if (row < 0) {
        return;
    }
    const QModelIndex modelIndex = index(row, 0);
    emit dataChanged(modelIndex, modelIndex, QVector<int>{role});
}

} // namespace Salama
