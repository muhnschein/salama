// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "HistoryModel.h"

#include "DateGroup.h"
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
// Firefox values (nsNavHistory::DecayFrecency).
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

HistoryModel::HistoryModel(const Storage &storage, QObject *parent)
    : QAbstractListModel(parent)
    , m_db(storage.database())
{
    prunePages();
    pruneInputs();
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
    switch (static_cast<Role>(role)) {
    case Role::Url:
        return entry.url;
    case Role::Title:
        return entry.title.isEmpty() ? entry.url : entry.title;
    case Role::MarkedTitle:
        return m_words.marked(entry.title.isEmpty() ? entry.url : entry.title);
    case Role::MarkedUrl:
        return m_words.marked(entry.url);
    case Role::Date:
        return entry.date;
    case Role::VisitCount:
        return entry.visitCount;
    case Role::Favicon:
        return entry.favicon;
    case Role::DateGroup:
        return static_cast<int>(DateGroup::of(entry.date.toMSecsSinceEpoch()));
    default:
        return {};
    }
}

QHash<int, QByteArray> HistoryModel::roleNames() const
{
    return {
        {roleId(Role::Url), QByteArrayLiteral("url")},
        {roleId(Role::Title), QByteArrayLiteral("title")},
        {roleId(Role::Date), QByteArrayLiteral("date")},
        {roleId(Role::VisitCount), QByteArrayLiteral("visitCount")},
        {roleId(Role::Favicon), QByteArrayLiteral("favicon")},
        {roleId(Role::DateGroup), QByteArrayLiteral("dateGroup")},
        {roleId(Role::MarkedTitle), QByteArrayLiteral("markedTitle")},
        {roleId(Role::MarkedUrl), QByteArrayLiteral("markedUrl")},
    };
}

int HistoryModel::count() const
{
    return m_entries.count();
}

int HistoryModel::pageCount() const
{
    return m_pageCount;
}

int HistoryModel::countSince(double since) const
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("SELECT COUNT(*) FROM browser_history WHERE date >= ?"));
    query.addBindValue(since > 0 ? qint64(since) : 0);
    if (!run(query) || !query.next()) {
        return 0;
    }
    return query.value(0).toInt();
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
    m_words = SearchWords(term);
    emit searchTermChanged();
    reload();
}

QList<HistoryModel::Entry> HistoryModel::allEntries() const
{
    QSqlQuery query(m_db);
    // Read once in order: no row cache.
    query.setForwardOnly(true);
    query.prepare(QStringLiteral("SELECT id, url, title, date, visited_count, favicon "
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
    entry.favicon = query.value(5).toString();
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
    // Strictly increasing: same-ms visits still order by recency, not row id.
    const qint64 now =
        std::max(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch(), m_lastVisit + 1);
    m_lastVisit = now;

    QSqlQuery exists(m_db);
    exists.prepare(QStringLiteral("SELECT id, url, title, date, visited_count, favicon "
                                  "FROM browser_history WHERE url = ?"));
    exists.addBindValue(url);
    if (!run(exists)) {
        return;
    }

    const bool known = exists.next();
    Entry entry = known ? entryAt(exists) : Entry();
    QSqlQuery query(m_db);
    if (known) {
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
    if (!run(query)) {
        return;
    }

    // Row as written, no re-read.
    int pages = m_pageCount;
    if (known) {
        ++entry.visitCount;
        if (!title.isEmpty()) {
            entry.title = title;
        }
    } else {
        entry.id = query.lastInsertId().toInt();
        entry.url = url;
        entry.title = title;
        entry.visitCount = 1;
        // Long session: launch prune alone lets table grow without bound.
        if (++pages > MaxEntries) {
            pages -= prunePages();
        }
    }
    entry.date = QDateTime::fromMSecsSinceEpoch(now);

    // Search filters rows; clock set back leaves newer rows above. Both rare: reload.
    if (!m_searchTerm.isEmpty() || !belongsOnTop(entry)) {
        reload();
    } else {
        showOnTop(entry);
        if (pages != m_pageCount) {
            m_pageCount = pages;
            emit pageCountChanged();
        }
    }
    emit entriesChanged();
}

bool HistoryModel::belongsOnTop(const Entry &entry) const
{
    const qint64 date = entry.date.toMSecsSinceEpoch();
    for (const Entry &other : m_entries) {
        if (other.id == entry.id) {
            continue;
        }
        // Reload's order: date DESC, id DESC.
        const qint64 otherDate = other.date.toMSecsSinceEpoch();
        return date > otherDate || (date == otherDate && entry.id > other.id);
    }
    return true;
}

void HistoryModel::showOnTop(const Entry &entry)
{
    const auto shown = std::find_if(m_entries.cbegin(), m_entries.cend(),
                                    [&entry](const Entry &one) { return one.id == entry.id; });
    if (shown != m_entries.cend()) {
        const int row = shown - m_entries.cbegin();
        if (row > 0) {
            beginMoveRows(QModelIndex(), row, row, QModelIndex(), 0);
            m_entries.move(row, 0);
            endMoveRows();
        }
        m_entries[0] = entry;
        emit dataChanged(index(0, 0), index(0, 0));
        return;
    }
    beginInsertRows(QModelIndex(), 0, 0);
    m_entries.prepend(entry);
    endInsertRows();
    if (m_entries.count() > DisplayLimit) {
        const int last = m_entries.count() - 1;
        beginRemoveRows(QModelIndex(), last, last);
        m_entries.removeLast();
        endRemoveRows();
    } else {
        emit countChanged();
    }
}

void HistoryModel::updateTitle(const QString &url, const QString &title)
{
    if (!isRecordable(url) || title.isEmpty()) {
        return;
    }
    // Tabs repeat title each load: unchanged one no news.
    QSqlQuery query(m_db);
    query.prepare(
        QStringLiteral("UPDATE browser_history SET title = ? WHERE url = ? AND title != ?"));
    query.addBindValue(title);
    query.addBindValue(url);
    query.addBindValue(title);
    if (!run(query) || query.numRowsAffected() <= 0) {
        return;
    }
    for (int i = 0; i < m_entries.count(); ++i) {
        if (m_entries.at(i).url == url && m_entries.at(i).title != title) {
            m_entries[i].title = title;
            const QModelIndex modelIndex = index(i, 0);
            emit dataChanged(modelIndex, modelIndex, QVector<int>{roleId(Role::Title)});
        }
    }
    emit entriesChanged();
}

void HistoryModel::updateFavicon(const QString &url, const QString &favicon)
{
    if (!isRecordable(url)) {
        return;
    }
    QSqlQuery query(m_db);
    query.prepare(
        QStringLiteral("UPDATE browser_history SET favicon = ? WHERE url = ? AND favicon != ?"));
    query.addBindValue(Storage::text(favicon));
    query.addBindValue(url);
    query.addBindValue(Storage::text(favicon));
    if (!run(query) || query.numRowsAffected() <= 0) {
        return;
    }
    for (int i = 0; i < m_entries.count(); ++i) {
        if (m_entries.at(i).url == url && m_entries.at(i).favicon != favicon) {
            m_entries[i].favicon = favicon;
            const QModelIndex modelIndex = index(i, 0);
            emit dataChanged(modelIndex, modelIndex, QVector<int>{roleId(Role::Favicon)});
        }
    }
    emit entriesChanged();
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
    --m_pageCount;
    emit pageCountChanged();
    emit entriesChanged();
}

void HistoryModel::removeUrl(const QString &url)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM browser_history WHERE url = ?"));
    query.addBindValue(url);
    if (!run(query) || query.numRowsAffected() <= 0) {
        return;
    }
    QSqlQuery inputs(m_db);
    inputs.prepare(QStringLiteral("DELETE FROM input_history WHERE url = ?"));
    inputs.addBindValue(url);
    run(inputs);
    reload();
    emit entriesChanged();
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
        emit entriesChanged();
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

int HistoryModel::prunePages() const
{
    // Ties by id as reload(): dropped rows never shown ones.
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM browser_history WHERE id NOT IN (SELECT id FROM "
                                 "browser_history ORDER BY date DESC, id DESC LIMIT ?)"));
    query.addBindValue(MaxEntries);
    return run(query) ? std::max(query.numRowsAffected(), 0) : 0;
}

void HistoryModel::pruneInputs() const
{
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
        query.prepare(QStringLiteral("SELECT id, url, title, date, visited_count, favicon "
                                     "FROM browser_history ORDER BY date DESC, id DESC LIMIT ?"));
    } else {
        query.prepare(QStringLiteral("SELECT id, url, title, date, visited_count, favicon "
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
    const int pages = countSince(0);
    if (pages != m_pageCount) {
        m_pageCount = pages;
        emit pageCountChanged();
    }
}

} // namespace Salama
