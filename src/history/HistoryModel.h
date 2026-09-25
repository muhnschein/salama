// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Modelled on sailfish-browser apps/history/declarativehistorymodel.{h,cpp} and the
// browser_history handling in apps/storage/dbworker.cpp (Copyright (c) 2013 - 2021
// Jolla Ltd., MPL-2.0). Queries run synchronously: the table is capped at MaxEntries.
// Visit times are stored as milliseconds since the epoch, newest first.
//
// Beside the pages, what the address bar has learnt: which page was chosen from what it
// found after what was typed (input_history) -- Firefox's input history, which it
// keeps with its history and forgets with it (docs/DECISIONS/0027-omnibar.md).
#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QHash>
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

    // How far back clearing reaches: the choices Firefox's Clear browsing data dialog
    // offers, "Today" from midnight on.
    enum ClearRange
    {
        ClearLastHour,
        ClearLastTwoHours,
        ClearLastFourHours,
        ClearToday,
        ClearEverything
    };
    Q_ENUM(ClearRange)

    static const int MaxEntries = 2000;
    static const int DisplayLimit = 500;
    // What the address bar has learnt is pruned to the most recently used this many.
    static const int MaxInputs = 500;

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
    // One page, and what the address bar learnt leads to it.
    Q_INVOKABLE void remove(int index);
    // Every page, and all the address bar has learnt.
    Q_INVOKABLE void clear();
    // The pages last visited at or after a time, in milliseconds since the epoch, and
    // what the address bar learnt from then on; nothing, or less, is everything, as
    // clear(). A page visited before that time and again since goes whole: a row keeps
    // its last visit, not the ones before it.
    Q_INVOKABLE void clearSince(double since);
    // Where a ClearRange reaches back to from now, as clearSince() takes it: 0 for
    // everything, and for a range out of bounds.
    Q_INVOKABLE static double rangeStart(int range);
    static qint64 rangeStart(int range, const QDateTime &now);

    // That what was typed, trimmed and in lower case, led to the page chosen from what
    // the address bar found: each choice of the same page after the same text counts
    // one over nine tenths of what was counted before, so a habit that changes is
    // followed (UrlbarUtils.addToInputHistory). Empty text or an address the history
    // would not keep is not learnt.
    void recordInput(const QString &input, const QString &url);
    // How strongly what is typed now leads to each page, by address: for every text
    // learnt that begins with it, its count, twice that when the text is the very one,
    // worn down by a fortieth for each day since it was last chosen, as Firefox wears
    // its counts down day by day -- the most of those per address
    // (UrlbarProviderInputHistory). Nothing for empty text.
    QHash<QString, double> inputRanks(const QString &typed, qint64 now) const;

signals:
    void countChanged();
    void searchTermChanged();

private:
    // The row a query selecting id, url, title, date, visited_count is on.
    static Entry entryAt(const QSqlQuery &query);
    static bool isRecordable(const QString &url);
    // What input_history holds for text as recordInput() keeps it.
    static QString inputKey(const QString &input);
    void prune();
    void reload();

    QSqlDatabase m_db;
    QList<Entry> m_entries;
    QString m_searchTerm;
    qint64 m_lastVisit = 0;
};

} // namespace Salama
