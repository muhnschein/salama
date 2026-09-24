// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Modelled on sailfish-browser apps/history/declarativehistorymodel.{h,cpp} and the
// browser_history handling in apps/storage/dbworker.cpp (Copyright (c) 2013 - 2021
// Jolla Ltd., MPL-2.0). Queries run synchronously: the table is capped at MaxEntries.
// Visit times are stored as milliseconds since the epoch, newest first.
#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QList>
#include <QSqlDatabase>
#include <QString>

class QSqlQuery;

namespace Salama {

class Storage;

class HistoryModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QString searchTerm READ searchTerm WRITE setSearchTerm NOTIFY searchTermChanged)

public:
    enum Role
    {
        UrlRole = Qt::UserRole + 1,
        TitleRole,
        DateRole,
        VisitCountRole
    };

    static const int MaxEntries = 2000;
    static const int DisplayLimit = 500;

    struct Entry
    {
        int id = 0;
        QString url;
        QString title;
        QDateTime date;
        int visitCount = 0;
    };

    explicit HistoryModel(Storage &storage, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;
    QString searchTerm() const;
    void setSearchTerm(const QString &term);

    // Every row of the table, newest first, whatever the search term and past the
    // DisplayLimit this model shows: the address bar's suggestions are matched from it
    // in C++, with the words matched as every other source's are (SearchWords), so
    // case folds by the same rules everywhere -- SQLite's LIKE folds ASCII alone
    // (docs/DECISIONS/0027-omnibar.md). A whole table is affordable because it is
    // bounded: pruned to MaxEntries each time the model is made, it holds no more than
    // that and the pages of one session.
    QList<Entry> allEntries() const;

    Q_INVOKABLE void visit(const QString &url, const QString &title = QString());
    Q_INVOKABLE void updateTitle(const QString &url, const QString &title);
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE void clear();

signals:
    void countChanged();
    void searchTermChanged();

private:
    // The row a query selecting id, url, title, date, visited_count is on.
    static Entry entryAt(const QSqlQuery &query);
    static bool isRecordable(const QString &url);
    void prune();
    void reload();

    QSqlDatabase m_db;
    QList<Entry> m_entries;
    QString m_searchTerm;
    qint64 m_lastVisit = 0;
};

} // namespace Salama
