// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "DownloadModel.h"

#include "engine/EngineData.h"
#include "engine/EngineMessages.h"
#include "storage/Storage.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QUrl>
#include <QtDebug>
#include <algorithm>
#include <cmath>
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

// The row a download of this run is on, by the engine's id for it, or -1.
int rowForEngineId(const QList<DownloadModel::Download> &downloads, int engineId)
{
    for (int row = 0; row < downloads.count(); ++row) {
        if (downloads.at(row).engineId == engineId) {
            return row;
        }
    }
    return -1;
}

// Counts the downloads still coming again, and how far along they are together, into
// count and progress: answers whether either changed, so the model says so only then.
// Called after every change to a row's status or progress, and to which rows there are.
bool recountRunning(const QList<DownloadModel::Download> &downloads, int &count, int &progress)
{
    int running = 0;
    int percent = 0;
    for (const DownloadModel::Download &download : downloads) {
        if (download.status == DownloadModel::Running) {
            ++running;
            percent += download.progress;
        }
    }
    const int together = running > 0 ? qRound(double(percent) / running) : 0;
    if (running == count && together == progress) {
        return false;
    }
    count = running;
    progress = together;
    return true;
}

} // namespace

DownloadModel::DownloadModel(const Storage &storage, QString directory, QObject *parent)
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
    switch (static_cast<Role>(role)) {
    case Role::DownloadId:
        return download.id;
    case Role::Name:
        return download.name;
    case Role::Url:
        return download.url;
    case Role::Path:
        return download.path;
    case Role::MimeType:
        return download.mimeType;
    case Role::Size:
        return download.size;
    case Role::Progress:
        return download.progress;
    case Role::Status:
        return static_cast<int>(download.status);
    case Role::Started:
        return download.started;
    default:
        return {};
    }
}

QHash<int, QByteArray> DownloadModel::roleNames() const
{
    return {
        {roleId(Role::DownloadId), QByteArrayLiteral("downloadId")},
        {roleId(Role::Name), QByteArrayLiteral("name")},
        {roleId(Role::Url), QByteArrayLiteral("url")},
        {roleId(Role::Path), QByteArrayLiteral("path")},
        {roleId(Role::MimeType), QByteArrayLiteral("mimeType")},
        {roleId(Role::Size), QByteArrayLiteral("size")},
        {roleId(Role::Progress), QByteArrayLiteral("progress")},
        {roleId(Role::Status), QByteArrayLiteral("status")},
        {roleId(Role::Started), QByteArrayLiteral("started")},
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

int DownloadModel::runningCount() const
{
    return m_runningCount;
}

int DownloadModel::runningProgress() const
{
    return m_runningProgress;
}

int DownloadModel::finishedCount() const
{
    const qint64 since = earliestRunning();
    if (since == 0) {
        return 0;
    }
    int finished = 0;
    for (const Download &download : m_downloads) {
        const bool ended = download.status == Done || download.status == Failed;
        if (ended && download.finished >= since) {
            ++finished;
        }
    }
    return finished;
}

int DownloadModel::failedCount() const
{
    const qint64 since = earliestRunning();
    if (since == 0) {
        return 0;
    }
    int failed = 0;
    for (const Download &download : m_downloads) {
        if (download.status == Failed && download.finished >= since) {
            ++failed;
        }
    }
    return failed;
}

qint64 DownloadModel::earliestRunning() const
{
    qint64 earliest = 0;
    for (const Download &download : m_downloads) {
        if (download.status == Running && (earliest == 0 || download.started < earliest)) {
            earliest = download.started;
        }
    }
    return earliest;
}

qint64 DownloadModel::estimate() const
{
    qint64 eta = -1;
    for (const Download &download : m_downloads) {
        if (download.status != Running || download.rate <= 0) {
            continue;
        }
        const auto own = qint64(std::ceil((100.0 - download.progress) / download.rate));
        if (eta < 0 || own > eta) {
            eta = own;
        }
    }
    return eta;
}

qint64 DownloadModel::etaSeconds() const
{
    return estimate();
}

QString DownloadModel::runningName() const
{
    if (m_runningCount != 1) {
        return {};
    }
    for (const Download &download : m_downloads) {
        if (download.status == Running) {
            return download.name;
        }
    }
    return {};
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
    const int row = rowForEngineId(m_downloads, engineId);
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
        // Our own pause reaches here: it is the engine's cancel, whose echo must not
        // untell the list's Paused. A cancel said of anything else is one.
        const Download &download = m_downloads.at(row);
        if (download.status != Paused) {
            setStatus(row, Canceled);
        }
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
    if (recountRunning(m_downloads, m_runningCount, m_runningProgress)) {
        emit runningChanged();
    }
    emit finishedChanged();
    emit etaChanged();
}

void DownloadModel::clearSince(double since)
{
    const int before = m_downloads.count();
    for (int row = m_downloads.count() - 1; row >= 0; --row) {
        const Download &download = m_downloads.at(row);
        if (download.status == Running || double(download.started) < since) {
            continue;
        }
        const int id = download.id;
        beginRemoveRows(QModelIndex(), row, row);
        m_downloads.removeAt(row);
        endRemoveRows();
        erase(id);
    }
    if (m_downloads.count() != before) {
        emit countChanged();
        emit finishedChanged();
        emit etaChanged();
    }
}

int DownloadModel::countSince(double since) const
{
    return int(
        std::count_if(m_downloads.cbegin(), m_downloads.cend(), [since](const Download &download) {
            return download.status != Running && double(download.started) >= since;
        }));
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
    if (recountRunning(m_downloads, m_runningCount, m_runningProgress)) {
        emit runningChanged();
    }
    emit finishedChanged();
    emit etaChanged();
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

void DownloadModel::pause(int downloadId)
{
    const int row = rowOf(downloadId);
    if (row < 0 || m_downloads.at(row).status != Running) {
        return;
    }
    // The engine's cancel keeps the partial file, so the same download can start again.
    // The row is Paused for the list's sake, and the engine's echo of its own cancel is
    // not allowed to untell it (observe()). Written down, like the statuses the engine
    // sends; a paused download read back after a restart is failed with the rest.
    Download &download = m_downloads[row];
    download.status = Paused;
    store(download);
    changed(row, {roleId(Role::Status)});
    if (m_downloads.at(row).engineId > 0) {
        emit engineRequest(EngineMessages::downloadTopic(),
                           EngineMessages::downloadCancel(m_downloads.at(row).engineId));
    }
}

void DownloadModel::resume(int downloadId)
{
    const int row = rowOf(downloadId);
    if (row < 0 || m_downloads.at(row).status != Paused) {
        return;
    }
    // Retry starts the download again from the partial file, and the engine says so
    // with a dl-start for the id it kept, which brings the row back to Running.
    if (m_downloads.at(row).engineId > 0) {
        emit engineRequest(EngineMessages::downloadTopic(),
                           EngineMessages::downloadRetry(m_downloads.at(row).engineId));
    } else {
        setStatus(row, Running);
    }
}

void DownloadModel::cancel(int downloadId)
{
    const int row = rowOf(downloadId);
    if (row < 0) {
        return;
    }
    const Status status = m_downloads.at(row).status;
    if (status != Running && status != Paused) {
        return;
    }
    setStatus(row, Canceled);
    if (m_downloads.at(row).engineId > 0) {
        emit engineRequest(EngineMessages::downloadTopic(),
                           EngineMessages::downloadCancel(m_downloads.at(row).engineId));
    }
}

bool DownloadModel::hasFile(int downloadId)
{
    const int row = rowOf(downloadId);
    return row >= 0 && m_downloads.at(row).status == Done && !m_downloads.at(row).path.isEmpty() &&
           QFileInfo::exists(m_downloads.at(row).path);
}

void DownloadModel::deleteFile(int downloadId)
{
    const int row = rowOf(downloadId);
    if (row < 0 || m_downloads.at(row).status != Done || m_downloads.at(row).path.isEmpty()) {
        return;
    }
    // The file is the downloaded thing; the row is the record of it. The file goes, and
    // the record keeps the site, so nothing left to open says so by opening nothing.
    Download &download = m_downloads[row];
    QFile file(download.path);
    if (!file.remove()) {
        return;
    }
    download.path.clear();
    store(download);
    changed(row, {roleId(Role::Path)});
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
    if (recountRunning(m_downloads, m_runningCount, m_runningProgress)) {
        emit runningChanged();
    }
    emit finishedChanged();
    emit etaChanged();
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
    const int old = download.progress;
    download.progress = progress;
    // The pace the estimate of the time left works from: how far the percentage has
    // come since it last said, over the time between. Smoothed, so a pause in the
    // reports does not swing the estimate whole, and only from reports a second apart,
    // which a burst of them would otherwise read as an absurd pace. A report of less
    // than the last one -- a server that restarts a file -- is moving backwards, and
    // says nothing. A download started over carries no pace (start()).
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (download.progressAt > 0 && now > download.progressAt) {
        const qint64 elapsed = now - download.progressAt;
        if (elapsed >= 1000) {
            const qreal instant = qreal(progress - old) / (qreal(elapsed) / 1000);
            if (instant > 0) {
                download.rate = download.rate > 0 ? 0.75 * download.rate + 0.25 * instant : instant;
            }
        }
    }
    download.progressAt = now;
    changed(row, {roleId(Role::Progress)});
}

void DownloadModel::finish(int row, const QString &path)
{
    Download &download = m_downloads[row];
    QVector<int> roles;
    if (download.status != Done) {
        download.status = Done;
        roles.append(roleId(Role::Status));
    }
    if (download.progress != 100) {
        download.progress = 100;
        roles.append(roleId(Role::Progress));
    }
    if (!path.isEmpty() && download.path != path) {
        download.path = path;
        roles.append(roleId(Role::Path));
    }
    if (roles.isEmpty()) {
        return;
    }
    // Not written: only true while this process runs, as a download's progress is.
    download.finished = QDateTime::currentMSecsSinceEpoch();
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
    // Coming again after the engine started it over, or ended: the strip's count of the
    // recent follows either way. Coming again is a fresh pace: what it made before, and
    // the gap while it was away, are not the file's.
    download.finished = status == Running ? 0 : QDateTime::currentMSecsSinceEpoch();
    if (status == Running) {
        download.rate = 0;
        download.progressAt = 0;
    }
    store(download);
    changed(row, {roleId(Role::Status)});
}

void DownloadModel::changed(int row, const QVector<int> &roles)
{
    const QModelIndex modelIndex = index(row, 0);
    emit dataChanged(modelIndex, modelIndex, roles);
    // Every change to a row's status or progress comes through here.
    if (recountRunning(m_downloads, m_runningCount, m_runningProgress)) {
        emit runningChanged();
    }
    emit finishedChanged();
    emit etaChanged();
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

void DownloadModel::insert(const Download &download) const
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

void DownloadModel::store(const Download &download) const
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("UPDATE download SET status = ?, path = ? WHERE id = ?"));
    query.addBindValue(static_cast<int>(download.status));
    query.addBindValue(Storage::text(download.path));
    query.addBindValue(download.id);
    run(query);
}

void DownloadModel::erase(int id) const
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM download WHERE id = ?"));
    query.addBindValue(id);
    run(query);
}

} // namespace Salama
