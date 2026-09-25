// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "HistoryModel.h"

#include "storage/Storage.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QtDebug>
#include <algorithm>
#include <cmath>

namespace Salama {

namespace {

const qint64 Hour = qint64(60) * 60 * 1000;
const qint64 Day = 24 * Hour;
// Firefox's: a choice counts over nine tenths of those before it, and every count
// wears down by a fortieth a day (nsNavHistory::DecayFrecency).
const double InputUseDecay = 0.9;
const double InputDayDecay = 0.975;

bool run(QSqlQuery &query)
{
    if (!query.exec()) {
        qWarning() << "HistoryModel:" << query.lastError().text() << query.lastQuery();
        return false;
    }
    return true;
}

} // namespace

HistoryModel::HistoryModel(Storage &storage, QObject *parent)
    : QAbstractListModel(parent)
    , m_db(storage.database())
{
    prune();
    reload();
}

int HistoryModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_entries.count();
}

QVariant HistoryModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_entries.count()) {
        return {};
    }
    const Entry &entry = m_entries.at(index.row());
    switch (role) {
    case UrlRole:
        return entry.url;
    case TitleRole:
        return entry.title.isEmpty() ? entry.url : entry.title;
    case DateRole:
        return entry.date;
    case VisitCountRole:
        return entry.visitCount;
    default:
        return {};
    }
}

QHash<int, QByteArray> HistoryModel::roleNames() const
{
    return {
        {UrlRole, QByteArrayLiteral("url")},
        {TitleRole, QByteArrayLiteral("title")},
        {DateRole, QByteArrayLiteral("date")},
        {VisitCountRole, QByteArrayLiteral("visitCount")},
    };
}

int HistoryModel::count() const
{
    return m_entries.count();
}

QString HistoryModel::searchTerm() const
{
    return m_searchTerm;
}

void HistoryModel::setSearchTerm(const QString &term)
{
    if (m_searchTerm == term) {
        return;
    }
    m_searchTerm = term;
    emit searchTermChanged();
    reload();
}

QList<HistoryModel::Entry> HistoryModel::allEntries() const
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("SELECT id, url, title, date, visited_count "
                                 "FROM browser_history ORDER BY date DESC, id DESC"));
    QList<Entry> entries;
    if (!run(query)) {
        return entries;
    }
    while (query.next()) {
        entries.append(entryAt(query));
    }
    return entries;
}

HistoryModel::Entry HistoryModel::entryAt(const QSqlQuery &query)
{
    Entry entry;
    entry.id = query.value(0).toInt();
    entry.url = query.value(1).toString();
    entry.title = query.value(2).toString();
    entry.date = QDateTime::fromMSecsSinceEpoch(query.value(3).toLongLong());
    entry.visitCount = query.value(4).toInt();
    return entry;
}

bool HistoryModel::isRecordable(const QString &url)
{
    return !url.isEmpty() && !url.startsWith(QLatin1String("about:"));
}

void HistoryModel::visit(const QString &url, const QString &title)
{
    if (!isRecordable(url)) {
        return;
    }
    // Strictly increasing within a session, so two visits in the same millisecond
    // still order by recency rather than by row id.
    const qint64 now =
        std::max(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch(), m_lastVisit + 1);
    m_lastVisit = now;

    QSqlQuery exists(m_db);
    exists.prepare(QStringLiteral("SELECT id FROM browser_history WHERE url = ?"));
    exists.addBindValue(url);
    if (!run(exists)) {
        return;
    }

    QSqlQuery query(m_db);
    if (exists.next()) {
        if (title.isEmpty()) {
            query.prepare(QStringLiteral("UPDATE browser_history SET date = ?, "
                                         "visited_count = visited_count + 1 WHERE url = ?"));
            query.addBindValue(now);
        } else {
            query.prepare(QStringLiteral("UPDATE browser_history SET date = ?, title = ?, "
                                         "visited_count = visited_count + 1 WHERE url = ?"));
            query.addBindValue(now);
            query.addBindValue(title);
        }
        query.addBindValue(url);
    } else {
        query.prepare(
            QStringLiteral("INSERT INTO browser_history (url, title, date) VALUES (?, ?, ?)"));
        query.addBindValue(url);
        query.addBindValue(Storage::text(title));
        query.addBindValue(now);
    }
    if (run(query)) {
        reload();
    }
}

void HistoryModel::updateTitle(const QString &url, const QString &title)
{
    if (!isRecordable(url) || title.isEmpty()) {
        return;
    }
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("UPDATE browser_history SET title = ? WHERE url = ?"));
    query.addBindValue(title);
    query.addBindValue(url);
    if (!run(query)) {
        return;
    }
    for (int i = 0; i < m_entries.count(); ++i) {
        if (m_entries.at(i).url == url && m_entries.at(i).title != title) {
            m_entries[i].title = title;
            const QModelIndex modelIndex = index(i, 0);
            emit dataChanged(modelIndex, modelIndex, QVector<int>{TitleRole});
        }
    }
}

void HistoryModel::remove(int index)
{
    if (index < 0 || index >= m_entries.count()) {
        return;
    }
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM browser_history WHERE id = ?"));
    query.addBindValue(m_entries.at(index).id);
    if (!run(query)) {
        return;
    }
    QSqlQuery inputs(m_db);
    inputs.prepare(QStringLiteral("DELETE FROM input_history WHERE url = ?"));
    inputs.addBindValue(m_entries.at(index).url);
    run(inputs);
    beginRemoveRows(QModelIndex(), index, index);
    m_entries.removeAt(index);
    endRemoveRows();
    emit countChanged();
}

void HistoryModel::clear()
{
    clearSince(0);
}

void HistoryModel::clearSince(double since)
{
    const qint64 from = since > 0 ? qint64(since) : 0;
    QSqlQuery inputs(m_db);
    inputs.prepare(QStringLiteral("DELETE FROM input_history WHERE used >= ?"));
    inputs.addBindValue(from);
    run(inputs);
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM browser_history WHERE date >= ?"));
    query.addBindValue(from);
    if (run(query)) {
        reload();
    }
}

double HistoryModel::rangeStart(int range)
{
    return double(rangeStart(range, QDateTime::currentDateTime()));
}

qint64 HistoryModel::rangeStart(int range, const QDateTime &now)
{
    const qint64 at = now.toMSecsSinceEpoch();
    switch (range) {
    case ClearLastHour:
        return at - Hour;
    case ClearLastTwoHours:
        return at - 2 * Hour;
    case ClearLastFourHours:
        return at - 4 * Hour;
    case ClearToday:
        return QDateTime(now.toLocalTime().date(), QTime(0, 0)).toMSecsSinceEpoch();
    default:
        return 0;
    }
}

QString HistoryModel::inputKey(const QString &input)
{
    return input.trimmed().toLower();
}

void HistoryModel::recordInput(const QString &input, const QString &url) const
{
    const QString key = inputKey(input);
    if (key.isEmpty() || !isRecordable(url)) {
        return;
    }
    QSqlQuery known(m_db);
    known.prepare(
        QStringLiteral("SELECT use_count FROM input_history WHERE input = ? AND url = ?"));
    known.addBindValue(key);
    known.addBindValue(url);
    if (!run(known)) {
        return;
    }
    const double count = known.next() ? known.value(0).toDouble() : 0;
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("INSERT OR REPLACE INTO input_history (input, url, use_count, "
                                 "used) VALUES (?, ?, ?, ?)"));
    query.addBindValue(key);
    query.addBindValue(url);
    query.addBindValue(count * InputUseDecay + 1);
    query.addBindValue(QDateTime::currentMSecsSinceEpoch());
    run(query);
}

// The whole table, matched here rather than in SQL, as the history is (allEntries()):
// it is pruned to MaxInputs.
QHash<QString, double> HistoryModel::inputRanks(const QString &typed, qint64 now) const
{
    QHash<QString, double> ranks;
    const QString key = inputKey(typed);
    if (key.isEmpty()) {
        return ranks;
    }
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("SELECT input, url, use_count, used FROM input_history"));
    if (!run(query)) {
        return ranks;
    }
    while (query.next()) {
        const QString input = query.value(0).toString();
        if (!input.startsWith(key)) {
            continue;
        }
        const double days = double(std::max(qint64(0), now - query.value(3).toLongLong())) / Day;
        const double rank =
            query.value(2).toDouble() * std::pow(InputDayDecay, days) * (input == key ? 2 : 1);
        const QString url = query.value(1).toString();
        ranks.insert(url, std::max(rank, ranks.value(url)));
    }
    return ranks;
}

void HistoryModel::prune()
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM browser_history WHERE id NOT IN "
                                 "(SELECT id FROM browser_history ORDER BY date DESC LIMIT ?)"));
    query.addBindValue(MaxEntries);
    run(query);
    QSqlQuery inputs(m_db);
    inputs.prepare(QStringLiteral("DELETE FROM input_history WHERE rowid NOT IN "
                                  "(SELECT rowid FROM input_history ORDER BY used DESC LIMIT ?)"));
    inputs.addBindValue(MaxInputs);
    run(inputs);
}

void HistoryModel::reload()
{
    QSqlQuery query(m_db);
    if (m_searchTerm.isEmpty()) {
        query.prepare(QStringLiteral("SELECT id, url, title, date, visited_count "
                                     "FROM browser_history ORDER BY date DESC, id DESC LIMIT ?"));
    } else {
        query.prepare(QStringLiteral("SELECT id, url, title, date, visited_count "
                                     "FROM browser_history WHERE url LIKE ? OR title LIKE ? "
                                     "ORDER BY date DESC, id DESC LIMIT ?"));
        const QString pattern = QLatin1Char('%') + m_searchTerm + QLatin1Char('%');
        query.addBindValue(pattern);
        query.addBindValue(pattern);
    }
    query.addBindValue(DisplayLimit);
    if (!run(query)) {
        return;
    }

    QList<Entry> entries;
    while (query.next()) {
        entries.append(entryAt(query));
    }

    const int oldCount = m_entries.count();
    beginResetModel();
    m_entries = entries;
    endResetModel();
    if (oldCount != m_entries.count()) {
        emit countChanged();
    }
}

} // namespace Salama
