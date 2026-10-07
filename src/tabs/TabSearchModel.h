// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "ModelRoles.h"

#include "search/SearchWords.h"

#include <QAbstractListModel>
#include <QList>
#include <QString>

namespace Salama {

class TabModel;

// Open tabs holding every word, in strip then grid order. Same SearchWords as address bar.
// Full rebuild on tab/group change: short list.
class TabSearchModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString searchTerm READ searchTerm WRITE setSearchTerm NOTIFY searchTermChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum class Role
    {
        TabId = Qt::UserRole + 1,
        Url,
        Title,
        Favicon,
        GroupId,
        GroupName,
        GroupTabCount,
        GroupStart
    };

    explicit TabSearchModel(TabModel *tabs, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString searchTerm() const;
    void setSearchTerm(const QString &term);
    int count() const;

signals:
    void searchTermChanged();
    void countChanged();

private:
    struct Row
    {
        int tabId;
        int groupId;
        bool groupStart;
    };

    QList<Row> rowsForTerm() const;
    void rebuild();
    // Incremental, no reset.
    void refine();
    bool matches(int tabIndex) const;

    TabModel *m_tabs;
    QString m_searchTerm;
    SearchWords m_words;
    QList<Row> m_rows;
};

} // namespace Salama
