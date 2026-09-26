// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "TabGroupModel.h"

#include "TabModel.h"

namespace Salama {

TabGroupModel::TabGroupModel(TabModel *tabs)
    : QAbstractListModel(tabs)
    , m_tabs(tabs)
{
}

int TabGroupModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_tabs->groups().count();
}

QVariant TabGroupModel::data(const QModelIndex &index, int role) const
{
    const QList<TabGroup> &groups = m_tabs->groups();
    if (index.row() < 0 || index.row() >= groups.count()) {
        return {};
    }
    const TabGroup &group = groups.at(index.row());
    switch (static_cast<Role>(role)) {
    case Role::GroupId:
        return group.id;
    case Role::Name:
        return group.name;
    case Role::TabCount:
        return m_tabs->tabCountInGroup(group.id);
    case Role::Current:
        return group.id == m_tabs->currentGroupId();
    case Role::Default:
        return group.id == m_tabs->defaultGroupId();
    default:
        return {};
    }
}

QHash<int, QByteArray> TabGroupModel::roleNames() const
{
    return {
        {roleId(Role::GroupId), QByteArrayLiteral("groupId")},
        {roleId(Role::Name), QByteArrayLiteral("name")},
        {roleId(Role::TabCount), QByteArrayLiteral("tabCount")},
        {roleId(Role::Current), QByteArrayLiteral("currentGroup")},
        {roleId(Role::Default), QByteArrayLiteral("defaultGroup")},
    };
}

int TabGroupModel::count() const
{
    return m_tabs->groups().count();
}

int TabGroupModel::groupIdAt(int row) const
{
    const QList<TabGroup> &groups = m_tabs->groups();
    return row >= 0 && row < groups.count() ? groups.at(row).id : 0;
}

void TabGroupModel::activate(int row)
{
    const int groupId = groupIdAt(row);
    if (groupId > 0) {
        m_tabs->setCurrentGroupId(groupId);
    }
}

int TabGroupModel::addGroup(const QString &name)
{
    return m_tabs->addGroup(name);
}

void TabGroupModel::renameGroup(int groupId, const QString &name)
{
    m_tabs->renameGroup(groupId, name);
}

bool TabGroupModel::removeGroup(int groupId)
{
    return m_tabs->removeGroup(groupId);
}

bool TabGroupModel::moveTab(int tabId, int groupId)
{
    return m_tabs->moveTabToGroup(tabId, groupId);
}

void TabGroupModel::inserted(int row)
{
    // The tab model has already put the group in its list; this only tells the views.
    beginInsertRows(QModelIndex(), row, row);
    endInsertRows();
    emit countChanged();
}

void TabGroupModel::removed(int row)
{
    beginRemoveRows(QModelIndex(), row, row);
    endRemoveRows();
    emit countChanged();
}

void TabGroupModel::changed(int row, Role role)
{
    if (row < 0 || row >= m_tabs->groups().count()) {
        return;
    }
    const QModelIndex modelIndex = index(row, 0);
    emit dataChanged(modelIndex, modelIndex, QVector<int>{roleId(role)});
}

void TabGroupModel::changedAll(Role role)
{
    const int last = m_tabs->groups().count() - 1;
    if (last < 0) {
        return;
    }
    emit dataChanged(index(0, 0), index(last, 0), QVector<int>{roleId(role)});
}

} // namespace Salama
