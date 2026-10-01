// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "SiteExceptions.h"

#include "SitePermissions.h"

namespace Salama {

SiteExceptions::SiteExceptions(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    // dynamicSortFilter, which is on, keeps the order as rows change; what the order is
    // is lessThan()'s.
    connect(this, &QAbstractItemModel::rowsInserted, this, &SiteExceptions::recount);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &SiteExceptions::recount);
    connect(this, &QAbstractItemModel::modelReset, this, &SiteExceptions::recount);
    connect(this, &QAbstractItemModel::layoutChanged, this, &SiteExceptions::recount);
}

int SiteExceptions::kind() const
{
    return m_kind;
}

void SiteExceptions::setKind(int kind)
{
    if (kind == m_kind) {
        return;
    }
    m_kind = kind;
    invalidateFilter();
    emit kindChanged();
}

QObject *SiteExceptions::permissions() const
{
    return sourceModel();
}

void SiteExceptions::setPermissions(QObject *permissions)
{
    auto *model = qobject_cast<SitePermissions *>(permissions);
    if (model == nullptr || model == sourceModel()) {
        return;
    }
    setSourceModel(model);
    sort(0);
    emit permissionsChanged();
}

bool SiteExceptions::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const QModelIndex row = sourceModel()->index(sourceRow, 0, sourceParent);
    return sourceModel()->data(row, roleId(SitePermissions::Role::Kind)).toInt() == m_kind;
}

bool SiteExceptions::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    const auto read = [this](const QModelIndex &row, SitePermissions::Role role) {
        return sourceModel()->data(row, roleId(role));
    };
    // Allowed first, then blocked, then those asked each time: Allow, Block and Ask are
    // 1, 2 and 3.
    const int leftDecision = read(left, SitePermissions::Role::Decision).toInt();
    const int rightDecision = read(right, SitePermissions::Role::Decision).toInt();
    if (leftDecision != rightDecision) {
        return leftDecision < rightDecision;
    }
    const QString leftHost = read(left, SitePermissions::Role::Host).toString();
    const QString rightHost = read(right, SitePermissions::Role::Host).toString();
    if (leftHost != rightHost) {
        return leftHost < rightHost;
    }
    return read(left, SitePermissions::Role::Origin).toString() <
           read(right, SitePermissions::Role::Origin).toString();
}

void SiteExceptions::recount()
{
    if (rowCount() != m_count) {
        m_count = rowCount();
        emit countChanged();
    }
}

} // namespace Salama
