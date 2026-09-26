// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "SiteListModel.h"

namespace Salama {

SiteListModel::SiteListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int SiteListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_sites.count();
}

QVariant SiteListModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_sites.count()) {
        return {};
    }
    const Site &site = m_sites.at(index.row());
    switch (static_cast<Role>(role)) {
    case Role::Url:
        return site.url;
    case Role::Title:
        return site.title;
    case Role::Favicon:
        return site.favicon;
    default:
        return {};
    }
}

QHash<int, QByteArray> SiteListModel::roleNames() const
{
    return {
        {roleId(Role::Url), QByteArrayLiteral("url")},
        {roleId(Role::Title), QByteArrayLiteral("title")},
        {roleId(Role::Favicon), QByteArrayLiteral("favicon")},
    };
}

int SiteListModel::count() const
{
    return m_sites.count();
}

const QList<Site> &SiteListModel::sites() const
{
    return m_sites;
}

void SiteListModel::setSites(const QList<Site> &sites)
{
    if (sites == m_sites) {
        return;
    }
    const int oldCount = m_sites.count();
    beginResetModel();
    m_sites = sites;
    endResetModel();
    if (oldCount != m_sites.count()) {
        emit countChanged();
    }
}

} // namespace Salama
