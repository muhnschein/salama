// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "DownloadSearch.h"

#include "DateGroup.h"
#include "DownloadModel.h"

namespace Salama {

DownloadSearch::DownloadSearch(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    // lessThan's.
    setSortRole(roleId(DownloadModel::Role::DateGroup));
    connect(this, &QAbstractItemModel::rowsInserted, this, &DownloadSearch::recount);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &DownloadSearch::recount);
    connect(this, &QAbstractItemModel::modelReset, this, &DownloadSearch::recount);
    connect(this, &QAbstractItemModel::layoutChanged, this, &DownloadSearch::recount);
}

QObject *DownloadSearch::downloads() const
{
    return sourceModel();
}

void DownloadSearch::setDownloads(QObject *downloads)
{
    auto *model = qobject_cast<DownloadModel *>(downloads);
    if (model == nullptr || model == sourceModel()) {
        return;
    }
    setSourceModel(model);
    sort(0);
    emit downloadsChanged();
}

QString DownloadSearch::searchTerm() const
{
    return m_searchTerm;
}

void DownloadSearch::setSearchTerm(const QString &term)
{
    if (term == m_searchTerm) {
        return;
    }
    m_searchTerm = term;
    m_words = SearchWords(term);
    invalidateFilter();
    emit searchTermChanged();
}

bool DownloadSearch::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const QModelIndex row = sourceModel()->index(sourceRow, 0, sourceParent);
    return m_words.matches({sourceModel()->data(row, roleId(DownloadModel::Role::Name)).toString(),
                            sourceModel()->data(row, roleId(DownloadModel::Role::Url)).toString()});
}

bool DownloadSearch::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    const auto inProgress = [this](const QModelIndex &row) {
        return sourceModel()->data(row, sortRole()).toInt() == DateGroup::InProgress;
    };
    if (inProgress(left) != inProgress(right)) {
        return inProgress(left);
    }
    return left.row() < right.row();
}

void DownloadSearch::recount()
{
    if (rowCount() != m_count) {
        m_count = rowCount();
        emit countChanged();
    }
}

} // namespace Salama
