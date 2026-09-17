// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 tuuli contributors
#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>

namespace Tuuli {

class TabModel;

// The open tabs whose title or address contains the search term, group by group in
// the strip's order and within a group in the grid's. Every change to the tabs or the
// groups rebuilds the whole list: it is read on one page, and it is never long
// (docs/DECISIONS/0015-tab-groups.md).
class TabSearchModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString searchTerm READ searchTerm WRITE setSearchTerm NOTIFY searchTermChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Role
    {
        TabIdRole = Qt::UserRole + 1,
        UrlRole,
        TitleRole,
        FaviconRole,
        PrivateRole,
        GroupIdRole,
        GroupNameRole,
        GroupTabCountRole,
        GroupPrivateRole,
        // True on the first row of each group, where the page draws the group's heading.
        GroupStartRole
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
    // The term changed: rows come and go one at a time, the list is not reset.
    void refine();
    bool matches(int tabIndex) const;

    TabModel *m_tabs;
    QString m_searchTerm;
    QList<Row> m_rows;
};

} // namespace Tuuli
