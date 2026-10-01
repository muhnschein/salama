// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "DownloadModel.h"

#include "engine/EngineData.h"
#include "storage/Storage.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QSqlError>
#include <QSqlQuery>
#include <QUrl>
#include <QtDebug>
#include <algorithm>
#include <utility>

namespace Salama {

namespace {

const QString Topic = QStringLiteral("embed:download");
const QString RequestTopic = QStringLiteral("embedui:download");

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

// Whether a row is one the engine still has and has not finished: what pausing and
// resuming go on from.
bool resumable(const DownloadModel::Download &download)
{
    return download.engineId != 0 &&
           (download.status == DownloadModel::Failed || download.status == DownloadModel::Canceled);
}

} // namespace

bool DownloadModel::Tray::operator==(const Tray &other) const
{
    return count == other.count && failed == other.failed && paused == other.paused &&
           progress == other.progress && names == other.names && row == other.row &&
           status == other.status && size == other.size;
}

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
    case Role::Resumable:
        return resumable(download);
    case Role::FileExists:
        return download.status == Done && !download.path.isEmpty() &&
               QFileInfo::exists(download.path);
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
        {roleId(Role::Resumable), QByteArrayLiteral("resumable")},
        {roleId(Role::FileExists), QByteArrayLiteral("fileExists")},
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

int DownloadModel::trayCount() const
{
    return m_tray.count;
}

int DownloadModel::trayFailed() const
{
    return m_tray.failed;
}

int DownloadModel::trayPaused() const
{
    return m_tray.paused;
}

int DownloadModel::trayProgress() const
{
    return m_tray.progress;
}

QStringList DownloadModel::trayNames() const
{
    return m_tray.names;
}

int DownloadModel::trayRow() const
{
    return m_tray.row;
}

int DownloadModel::trayStatus() const
{
    return m_tray.status;
}

double DownloadModel::traySize() const
{
    return double(m_tray.size);
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
        setStatus(row, Canceled);
    }
}

void DownloadModel::remove(int row)
{
    if (row < 0 || row >= m_downloads.count()) {
        return;
    }
    pause(row);
    removeRow(row);
}

void DownloadModel::clearFinished()
{
    const int before = m_downloads.count();
    for (int row = m_downloads.count() - 1; row >= 0; --row) {
        if (m_downloads.at(row).status != Done) {
            continue;
        }
        const int id = m_downloads.at(row).id;
        beginRemoveRows(QModelIndex(), row, row);
        m_downloads.removeAt(row);
        endRemoveRows();
        erase(id);
    }
    if (m_downloads.count() != before) {
        emit countChanged();
        recount();
    }
}

void DownloadModel::pause(int row)
{
    if (row < 0 || row >= m_downloads.count()) {
        return;
    }
    const Download &download = m_downloads.at(row);
    if (download.status != Running || download.engineId == 0) {
        return;
    }
    // The row says Paused when the engine says it has stopped (dl-cancel), not before.
    request({{QStringLiteral("msg"), QStringLiteral("cancelDownload")},
             {QStringLiteral("id"), download.engineId}});
}

void DownloadModel::resume(int row)
{
    if (row < 0 || row >= m_downloads.count()) {
        return;
    }
    const Download &download = m_downloads.at(row);
    if (download.status != Failed && download.status != Canceled) {
        return;
    }
    if (download.engineId != 0) {
        request({{QStringLiteral("msg"), QStringLiteral("retryDownload")},
                 {QStringLiteral("id"), download.engineId}});
        return;
    }
    // One of an earlier run: the engine has forgotten it, and fetches it anew. Its
    // dl-start makes the row that stands for it now.
    if (download.url.isEmpty()) {
        return;
    }
    const QString path =
        download.path.isEmpty() ? QDir(m_directory).filePath(download.name) : download.path;
    request({{QStringLiteral("msg"), QStringLiteral("addDownload")},
             {QStringLiteral("from"), download.url},
             {QStringLiteral("to"), path}});
    removeRow(row);
}

bool DownloadModel::deleteFile(int row)
{
    if (row < 0 || row >= m_downloads.count()) {
        return false;
    }
    const Download &download = m_downloads.at(row);
    if (download.status != Done || download.path.isEmpty()) {
        return false;
    }
    const QFileInfo file(download.path);
    if (file.exists()) {
        // Where the file really is, links and ".." followed, under where it may be.
        const QString root = QFileInfo(QFileInfo(m_directory).absolutePath()).canonicalFilePath();
        const QString real = file.canonicalFilePath();
        if (root.isEmpty() || !real.startsWith(root + QLatin1Char('/'))) {
            qWarning() << "DownloadModel: not deleting" << download.path;
            return false;
        }
        if (!QFile::remove(real)) {
            qWarning() << "DownloadModel: cannot delete" << real;
            return false;
        }
    }
    removeRow(row);
    return true;
}

void DownloadModel::refreshFiles()
{
    if (m_downloads.isEmpty()) {
        return;
    }
    emit dataChanged(index(0, 0), index(m_downloads.count() - 1, 0), {roleId(Role::FileExists)});
}

void DownloadModel::dismissTray()
{
    for (Download &download : m_downloads) {
        if (download.engineId != 0 && download.status != Done) {
            download.dismissed = true;
        }
    }
    recount();
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
        recount();
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
    for (int row = 0; row < m_downloads.count(); ++row) {
        pause(row);
    }
    beginRemoveRows(QModelIndex(), 0, m_downloads.count() - 1);
    m_downloads.clear();
    endRemoveRows();
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM download"));
    run(query);
    emit countChanged();
    recount();
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

QString DownloadModel::folderUrl(int row) const
{
    if (row < 0 || row >= m_downloads.count() || m_downloads.at(row).path.isEmpty()) {
        return QUrl::fromLocalFile(m_directory).toString();
    }
    return QUrl::fromLocalFile(QFileInfo(m_downloads.at(row).path).absolutePath()).toString();
}

QString DownloadModel::formatSize(double bytes)
{
    static const char *const units[] = {"B", "kB", "MB", "GB", "TB"};
    const int last = int(sizeof(units) / sizeof(units[0])) - 1;
    double value = qIsFinite(bytes) ? std::max(0.0, bytes) : 0.0;
    int unit = 0;
    while (value >= 1024 && unit < last) {
        value /= 1024;
        ++unit;
    }
    // A size reads as one number: "1023 B", never "1,023 B".
    QLocale locale;
    locale.setNumberOptions(QLocale::OmitGroupSeparator);
    const QString number = unit == 0 ? locale.toString(qRound64(value))
                                     : locale.toString(value, 'f', value < 10 ? 1 : 0);
    return number + QLatin1Char(' ') + QLatin1String(units[unit]);
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
    recount();
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
    changed(row, {roleId(Role::Progress)});
}

void DownloadModel::finish(int row, const QString &path)
{
    Download &download = m_downloads[row];
    QVector<int> roles;
    const bool arrived = download.status != Done;
    if (arrived) {
        download.status = Done;
        roles.append({roleId(Role::Status), roleId(Role::Resumable), roleId(Role::FileExists)});
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
    store(download);
    // Said before the row's change, so that the banner has the name before the row
    // leaves what it shows.
    if (arrived) {
        emit finished(download.id, download.name);
    }
    changed(row, roles);
}

void DownloadModel::setStatus(int row, Status status)
{
    Download &download = m_downloads[row];
    if (download.status == status) {
        return;
    }
    download.status = status;
    // A change of state is news again, swiped away or not.
    download.dismissed = false;
    store(download);
    changed(row, {roleId(Role::Status), roleId(Role::Resumable)});
}

void DownloadModel::changed(int row, const QVector<int> &roles)
{
    const QModelIndex modelIndex = index(row, 0);
    emit dataChanged(modelIndex, modelIndex, roles);
    // Every change to a row's status or progress comes through here.
    recount();
}

void DownloadModel::recount()
{
    if (recountRunning(m_downloads, m_runningCount, m_runningProgress)) {
        emit runningChanged();
    }
    Tray tray;
    int coming = 0;
    int percent = 0;
    for (int row = 0; row < m_downloads.count(); ++row) {
        const Download &download = m_downloads.at(row);
        if (download.engineId == 0 || download.status == Done || download.dismissed) {
            continue;
        }
        if (tray.count == 0) {
            tray.row = row;
            tray.status = download.status;
            tray.size = download.size;
        }
        ++tray.count;
        tray.names.append(download.name);
        if (download.status == Failed) {
            ++tray.failed;
            continue;
        }
        if (download.status == Canceled) {
            ++tray.paused;
        }
        ++coming;
        percent += download.progress;
    }
    tray.progress = coming > 0 ? qRound(double(percent) / coming) : 0;
    if (tray == m_tray) {
        return;
    }
    m_tray = tray;
    emit trayChanged();
}

void DownloadModel::request(const QVariantMap &data)
{
    emit engineRequest(RequestTopic, data);
}

void DownloadModel::removeRow(int row)
{
    const int id = m_downloads.at(row).id;
    beginRemoveRows(QModelIndex(), row, row);
    m_downloads.removeAt(row);
    endRemoveRows();
    erase(id);
    emit countChanged();
    recount();
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
