// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "DownloadModel.h"

#include "engine/EngineData.h"
#include "storage/Storage.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QUrl>
#include <QtDebug>
#include <algorithm>
#include <utility>

namespace Salama {

namespace {

const QString Topic = QStringLiteral("embed:download");

bool run(QSqlQuery &query)
{
    if (!query.exec()) {
        qWarning() << "DownloadModel:" << query.lastError().text() << query.lastQuery();
        return false;
    }
    return true;
}

} // namespace

DownloadModel::DownloadModel(Storage &storage, QString directory, QObject *parent)
    : QAbstractListModel(parent)
    , m_db(storage.database())
    , m_directory(std::move(directory))
{
    if (!QDir().mkpath(m_directory)) {
        qWarning() << "DownloadModel: cannot create download directory" << m_directory;
    }
    load();
    dropOldest();
}

int DownloadModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_downloads.count();
}

QVariant DownloadModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_downloads.count()) {
        return {};
    }
    const Download &download = m_downloads.at(index.row());
    switch (role) {
    case DownloadIdRole:
        return download.id;
    case NameRole:
        return download.name;
    case UrlRole:
        return download.url;
    case PathRole:
        return download.path;
    case MimeTypeRole:
        return download.mimeType;
    case SizeRole:
        return download.size;
    case ProgressRole:
        return download.progress;
    case StatusRole:
        return static_cast<int>(download.status);
    case StartedRole:
        return download.started;
    default:
        return {};
    }
}

QHash<int, QByteArray> DownloadModel::roleNames() const
{
    return {
        {DownloadIdRole, QByteArrayLiteral("downloadId")},
        {NameRole, QByteArrayLiteral("name")},
        {UrlRole, QByteArrayLiteral("url")},
        {PathRole, QByteArrayLiteral("path")},
        {MimeTypeRole, QByteArrayLiteral("mimeType")},
        {SizeRole, QByteArrayLiteral("size")},
        {ProgressRole, QByteArrayLiteral("progress")},
        {StatusRole, QByteArrayLiteral("status")},
        {StartedRole, QByteArrayLiteral("started")},
    };
}

int DownloadModel::count() const
{
    return m_downloads.count();
}

QString DownloadModel::topic() const
{
    return Topic;
}

QString DownloadModel::directory() const
{
    return m_directory;
}

const QList<DownloadModel::Download> &DownloadModel::downloads() const
{
    return m_downloads;
}

void DownloadModel::observe(const QString &topic, const QVariant &data)
{
    if (topic != Topic) {
        return;
    }
    const QVariantMap message = data.toMap();
    const int engineId = EngineData::id(message.value(QStringLiteral("id")));
    if (engineId == 0) {
        return;
    }
    const QString msg = message.value(QStringLiteral("msg")).toString();
    const int row = rowForEngineId(engineId);
    if (msg == QLatin1String("dl-start")) {
        // A download the engine starts again -- retried after it failed, or resumed
        // after it was canceled -- keeps its id, and its row.
        if (row < 0) {
            start(engineId, message);
        } else {
            setStatus(row, Running);
        }
        return;
    }
    if (row < 0) {
        return;
    }
    if (msg == QLatin1String("dl-progress")) {
        setProgress(row, message.value(QStringLiteral("percent")));
    } else if (msg == QLatin1String("dl-done")) {
        finish(row, message.value(QStringLiteral("targetPath")).toString());
    } else if (msg == QLatin1String("dl-fail")) {
        setStatus(row, Failed);
    } else if (msg == QLatin1String("dl-cancel")) {
        setStatus(row, Canceled);
    }
}

void DownloadModel::remove(int row)
{
    if (row < 0 || row >= m_downloads.count()) {
        return;
    }
    const int id = m_downloads.at(row).id;
    beginRemoveRows(QModelIndex(), row, row);
    m_downloads.removeAt(row);
    endRemoveRows();
    erase(id);
    emit countChanged();
}

void DownloadModel::clear()
{
    if (m_downloads.isEmpty()) {
        return;
    }
    beginRemoveRows(QModelIndex(), 0, m_downloads.count() - 1);
    m_downloads.clear();
    endRemoveRows();
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM download"));
    run(query);
    emit countChanged();
}

QString DownloadModel::fileUrl(int row) const
{
    if (row < 0 || row >= m_downloads.count() || m_downloads.at(row).path.isEmpty()) {
        return {};
    }
    return QUrl::fromLocalFile(m_downloads.at(row).path).toString();
}

int DownloadModel::rowOf(int downloadId) const
{
    for (int row = 0; row < m_downloads.count(); ++row) {
        if (m_downloads.at(row).id == downloadId) {
            return row;
        }
    }
    return -1;
}

void DownloadModel::start(int engineId, const QVariantMap &message)
{
    Download download;
    download.id = m_nextId++;
    download.engineId = engineId;
    download.path = message.value(QStringLiteral("targetPath")).toString();
    download.name = message.value(QStringLiteral("displayName")).toString();
    if (download.name.isEmpty()) {
        download.name = QFileInfo(download.path).fileName();
    }
    download.url = message.value(QStringLiteral("sourceUrl")).toString();
    download.mimeType = message.value(QStringLiteral("mimeType")).toString();
    // The engine's totalBytes, which is 0 while the size is not known; a size that is
    // not a number says no more than that.
    const QVariant size = message.value(QStringLiteral("size"));
    download.size = EngineData::isNumber(size) ? std::max<qint64>(0, size.toLongLong()) : 0;
    download.status = Running;
    download.started = QDateTime::currentMSecsSinceEpoch();

    const int before = m_downloads.count();
    beginInsertRows(QModelIndex(), 0, 0);
    m_downloads.prepend(download);
    endInsertRows();
    insert(download);
    dropOldest();
    if (m_downloads.count() != before) {
        emit countChanged();
    }
}

void DownloadModel::setProgress(int row, const QVariant &percent)
{
    if (!EngineData::isNumber(percent)) {
        return;
    }
    // Bounded as a double, before rounding: a number past what an int holds has no
    // int to round to.
    const int progress = qRound(qBound(0.0, percent.toDouble(), 100.0));
    Download &download = m_downloads[row];
    if (download.progress == progress) {
        return;
    }
    // Not written: it would be worth nothing after a restart, when a download read
    // back has either finished or never will (load()).
    download.progress = progress;
    changed(row, {ProgressRole});
}

void DownloadModel::finish(int row, const QString &path)
{
    Download &download = m_downloads[row];
    QVector<int> roles;
    if (download.status != Done) {
        download.status = Done;
        roles.append(StatusRole);
    }
    if (download.progress != 100) {
        download.progress = 100;
        roles.append(ProgressRole);
    }
    if (!path.isEmpty() && download.path != path) {
        download.path = path;
        roles.append(PathRole);
    }
    if (roles.isEmpty()) {
        return;
    }
    store(download);
    changed(row, roles);
}

void DownloadModel::setStatus(int row, Status status)
{
    Download &download = m_downloads[row];
    if (download.status == status) {
        return;
    }
    download.status = status;
    store(download);
    changed(row, {StatusRole});
}

void DownloadModel::changed(int row, const QVector<int> &roles)
{
    const QModelIndex modelIndex = index(row, 0);
    emit dataChanged(modelIndex, modelIndex, roles);
}

int DownloadModel::rowForEngineId(int engineId) const
{
    for (int row = 0; row < m_downloads.count(); ++row) {
        if (m_downloads.at(row).engineId == engineId) {
            return row;
        }
    }
    return -1;
}

void DownloadModel::dropOldest()
{
    if (m_downloads.count() <= Limit) {
        return;
    }
    beginRemoveRows(QModelIndex(), Limit, m_downloads.count() - 1);
    while (m_downloads.count() > Limit) {
        erase(m_downloads.takeLast().id);
    }
    endRemoveRows();
}

void DownloadModel::load()
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("SELECT id, name, url, path, mime, size, status, started "
                                 "FROM download ORDER BY started DESC, id DESC"));
    if (!run(query)) {
        return;
    }
    QList<Download> stale;
    while (query.next()) {
        Download download;
        download.id = query.value(0).toInt();
        download.name = query.value(1).toString();
        download.url = query.value(2).toString();
        download.path = query.value(3).toString();
        download.mimeType = query.value(4).toString();
        download.size = query.value(5).toLongLong();
        download.started = query.value(7).toLongLong();
        // A download still running when the application stopped never finishes: the
        // engine forgets every download when it starts -- EmbedliteDownloadManager.js
        // removes them all at profile-after-change -- so no message will name it
        // again. It failed, and the database is told so. A status this build does not
        // know is read the same way.
        const int status = query.value(6).toInt();
        if (status == Done || status == Failed || status == Canceled) {
            download.status = static_cast<Status>(status);
        } else {
            download.status = Failed;
            stale.append(download);
        }
        download.progress = download.status == Done ? 100 : 0;
        m_downloads.append(download);
        m_nextId = std::max(m_nextId, download.id + 1);
    }
    // Written once the rows are read: SQLite leaves it undefined whether a query still
    // stepping through a table sees rows changed under it (sqlite.org/isolation.html).
    for (const Download &download : stale) {
        store(download);
    }
}

void DownloadModel::insert(const Download &download)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("INSERT INTO download "
                                 "(id, name, url, path, mime, size, status, started) "
                                 "VALUES (?, ?, ?, ?, ?, ?, ?, ?)"));
    query.addBindValue(download.id);
    query.addBindValue(Storage::text(download.name));
    query.addBindValue(Storage::text(download.url));
    query.addBindValue(Storage::text(download.path));
    query.addBindValue(Storage::text(download.mimeType));
    query.addBindValue(download.size);
    query.addBindValue(static_cast<int>(download.status));
    query.addBindValue(download.started);
    run(query);
}

void DownloadModel::store(const Download &download)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("UPDATE download SET status = ?, path = ? WHERE id = ?"));
    query.addBindValue(static_cast<int>(download.status));
    query.addBindValue(Storage::text(download.path));
    query.addBindValue(download.id);
    run(query);
}

void DownloadModel::erase(int id)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM download WHERE id = ?"));
    query.addBindValue(id);
    run(query);
}

} // namespace Salama
