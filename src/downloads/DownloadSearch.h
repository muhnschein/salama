// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "DownloadModel.h"
#include "ModelRoles.h"
#include "search/SearchWords.h"

#include <QObject>
#include <QSortFilterProxyModel>
#include <QString>

namespace Salama {

// Downloads page's rows: name or address holds every word, as in address bar. In progress
// first, then model's order. Rows differ from DownloadModel's: act by id (DownloadModel.rowOf).
class DownloadSearch : public QSortFilterProxyModel
{
    Q_OBJECT
    // QObject: Qt 5.6 QML passes models so. Non-DownloadModel refused.
    Q_PROPERTY(QObject *downloads READ downloads WRITE setDownloads NOTIFY downloadsChanged)
    Q_PROPERTY(QString searchTerm READ searchTerm WRITE setSearchTerm NOTIFY searchTermChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    // Beyond DownloadModel's. StyledText, escaped: search term's words in <b>.
    enum class Role
    {
        MarkedName = roleId(DownloadModel::Role::DateGroup) + 1,
        // SearchSettings::displayAddress of url, as row shows it.
        MarkedHost
    };

    explicit DownloadSearch(QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QObject *downloads() const;
    void setDownloads(QObject *downloads);
    QString searchTerm() const;
    void setSearchTerm(const QString &term);

signals:
    void downloadsChanged();
    void searchTermChanged();
    void countChanged();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    void recount();

    QString m_searchTerm;
    SearchWords m_words;
    int m_count = 0;
};

} // namespace Salama
