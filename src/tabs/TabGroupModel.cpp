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
    case Role::Previews:
        return m_tabs->groupThumbnails(group.id, PreviewLimit);
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
        {roleId(Role::Previews), QByteArrayLiteral("previews")},
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

bool TabGroupModel::ungroup(int groupId)
{
    return m_tabs->ungroup(groupId);
}

bool TabGroupModel::moveGroup(int from, int to)
{
    return m_tabs->moveGroup(from, to);
}

bool TabGroupModel::moveTab(int tabId, int groupId)
{
    return m_tabs->moveTabToGroup(tabId, groupId);
}

void TabGroupModel::inserted(int row)
{
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

void TabGroupModel::moved(int from, int to)
{
    // beginMoveRows wants dest + 1 when moving down.
    if (beginMoveRows(QModelIndex(), from, from, QModelIndex(), to > from ? to + 1 : to)) {
        endMoveRows();
    }
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
