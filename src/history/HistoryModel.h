// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Modelled on sailfish-browser apps/history/declarativehistorymodel.{h,cpp} and the
// browser_history handling in apps/storage/dbworker.cpp (Copyright (c) 2013 - 2021
// Jolla Ltd., MPL-2.0). Sync queries OK: table capped at MaxEntries. Times ms since epoch.
#pragma once

#include "ModelRoles.h"

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
    Q_PROPERTY(int pageCount READ pageCount NOTIFY pageCountChanged)
    Q_PROPERTY(QString searchTerm READ searchTerm WRITE setSearchTerm NOTIFY searchTermChanged)

public:
    enum class Role
    {
        Url = Qt::UserRole + 1,
        Title,
        Date,
        VisitCount,
        Favicon
    };

    enum ClearRange // NOSONAR(cpp:S3642) QML on Qt 5.6 reads no scoped enum
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
    static const int MaxInputs = 500;

    struct Entry
    {
        int id = 0;
        QString url;
        QString title;
        QDateTime date;
        int visitCount = 0;
        QString favicon;
    };

    explicit HistoryModel(const Storage &storage, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;
    int pageCount() const;
    QString searchTerm() const;
    void setSearchTerm(const QString &term);

    // Whole table for C++ matching: SQLite LIKE folds ASCII only. Bounded by MaxEntries.
    QList<Entry> allEntries() const;

    Q_INVOKABLE void visit(const QString &url, const QString &title = QString());
    Q_INVOKABLE void updateTitle(const QString &url, const QString &title);
    Q_INVOKABLE void updateFavicon(const QString &url, const QString &favicon);
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE void removeUrl(const QString &url);
    Q_INVOKABLE void clear();
    // `since` ms epoch; <= 0 -> clear(). Row keeps last visit only: page goes whole.
    Q_INVOKABLE void clearSince(double since);
    Q_INVOKABLE int countSince(double since) const;
    Q_INVOKABLE static double rangeStart(int range);
    static qint64 rangeStart(int range, const QDateTime &now);

    // count = 1 + 0.9 * old (UrlbarUtils.addToInputHistory).
    void recordInput(const QString &input, const QString &url) const;
    // Max count of learnt prefixes (x2 exact), decayed 1/40 per day (UrlbarProviderInputHistory).
    QHash<QString, double> inputRanks(const QString &typed, qint64 now) const;

signals:
    void countChanged();
    void pageCountChanged();
    void searchTermChanged();

private:
    // Query must select id, url, title, date, visited_count, favicon.
    static Entry entryAt(const QSqlQuery &query);
    static bool isRecordable(const QString &url);
    static QString inputKey(const QString &input);
    void prune() const;
    void reload();

    QSqlDatabase m_db;
    QList<Entry> m_entries;
    QString m_searchTerm;
    qint64 m_lastVisit = 0;
    int m_pageCount = 0;
};

} // namespace Salama
