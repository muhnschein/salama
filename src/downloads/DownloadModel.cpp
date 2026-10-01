// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "DownloadModel.h"

#include "engine/EngineData.h"
#include "storage/Storage.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QSqlError>
#include <QSqlQuery>
#include <QUrl>
#include <QtDebug>
#include <algorithm>
#include <limits>
#include <utility>

namespace Salama {

namespace {

const QString Topic = QStringLiteral("embed:download");
// What EmbedliteDownloadManager.js observes for what it is told back.
const QString CommandTopic = QStringLiteral("embedui:download");

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

// The row asked for again with addDownload that a dl-start for a new id is, by where
// the file goes, or -1.
int rowRefetching(const QList<DownloadModel::Download> &downloads, const QString &path)
{
    for (int row = 0; row < downloads.count(); ++row) {
        const DownloadModel::Download &download = downloads.at(row);
        if (download.refetching && !path.isEmpty() && download.path == path) {
            return row;
        }
    }
    return -1;
}

// The kind of file a name's extension says, in the words iconFor() answers in, or empty.
QString kindOfExtension(const QString &name)
{
    static const QHash<QString, QString> kinds = {
        {QStringLiteral("pdf"), QStringLiteral("pdf")},
        {QStringLiteral("jpg"), QStringLiteral("image")},
        {QStringLiteral("jpeg"), QStringLiteral("image")},
        {QStringLiteral("png"), QStringLiteral("image")},
        {QStringLiteral("gif"), QStringLiteral("image")},
        {QStringLiteral("webp"), QStringLiteral("image")},
        {QStringLiteral("svg"), QStringLiteral("image")},
        {QStringLiteral("heic"), QStringLiteral("image")},
        {QStringLiteral("mp3"), QStringLiteral("audio")},
        {QStringLiteral("ogg"), QStringLiteral("audio")},
        {QStringLiteral("opus"), QStringLiteral("audio")},
        {QStringLiteral("flac"), QStringLiteral("audio")},
        {QStringLiteral("wav"), QStringLiteral("audio")},
        {QStringLiteral("m4a"), QStringLiteral("audio")},
        {QStringLiteral("mp4"), QStringLiteral("video")},
        {QStringLiteral("webm"), QStringLiteral("video")},
        {QStringLiteral("mkv"), QStringLiteral("video")},
        {QStringLiteral("mov"), QStringLiteral("video")},
        {QStringLiteral("apk"), QStringLiteral("apk")},
        {QStringLiteral("rpm"), QStringLiteral("rpm")},
        {QStringLiteral("vcf"), QStringLiteral("vcard")},
        {QStringLiteral("csv"), QStringLiteral("spreadsheet")},
        {QStringLiteral("ods"), QStringLiteral("spreadsheet")},
        {QStringLiteral("xls"), QStringLiteral("spreadsheet")},
        {QStringLiteral("xlsx"), QStringLiteral("spreadsheet")},
        {QStringLiteral("odp"), QStringLiteral("presentation")},
        {QStringLiteral("ppt"), QStringLiteral("presentation")},
        {QStringLiteral("pptx"), QStringLiteral("presentation")},
        {QStringLiteral("zip"), QStringLiteral("archive-folder")},
        {QStringLiteral("tar"), QStringLiteral("archive-folder")},
        {QStringLiteral("gz"), QStringLiteral("archive-folder")},
        {QStringLiteral("tgz"), QStringLiteral("archive-folder")},
        {QStringLiteral("bz2"), QStringLiteral("archive-folder")},
        {QStringLiteral("xz"), QStringLiteral("archive-folder")},
        {QStringLiteral("7z"), QStringLiteral("archive-folder")},
        {QStringLiteral("rar"), QStringLiteral("archive-folder")},
        {QStringLiteral("txt"), QStringLiteral("document")},
        {QStringLiteral("md"), QStringLiteral("document")},
        {QStringLiteral("rtf"), QStringLiteral("document")},
        {QStringLiteral("odt"), QStringLiteral("document")},
        {QStringLiteral("doc"), QStringLiteral("document")},
        {QStringLiteral("docx"), QStringLiteral("document")},
        {QStringLiteral("epub"), QStringLiteral("document")},
    };
    return kinds.value(QFileInfo(name).suffix().toLower());
}

// The kind of file a MIME type says, or empty when it says nothing useful.
QString kindOfType(const QString &mimeType)
{
    const QString type = mimeType.toLower();
    const QString major = type.section(QLatin1Char('/'), 0, 0);
    if (type == QLatin1String("application/pdf")) {
        return QStringLiteral("pdf");
    }
    if (major == QLatin1String("image") || major == QLatin1String("audio") ||
        major == QLatin1String("video")) {
        return major;
    }
    if (type == QLatin1String("application/vnd.android.package-archive")) {
        return QStringLiteral("apk");
    }
    if (type == QLatin1String("application/x-rpm")) {
        return QStringLiteral("rpm");
    }
    if (type == QLatin1String("text/vcard") || type == QLatin1String("text/x-vcard")) {
        return QStringLiteral("vcard");
    }
    if (type.contains(QLatin1String("spreadsheet")) || type.contains(QLatin1String("ms-excel")) ||
        type == QLatin1String("text/csv")) {
        return QStringLiteral("spreadsheet");
    }
    if (type.contains(QLatin1String("presentation")) ||
        type.contains(QLatin1String("ms-powerpoint"))) {
        return QStringLiteral("presentation");
    }
    static const QStringList archives = {
        QStringLiteral("application/zip"),
        QStringLiteral("application/x-tar"),
        QStringLiteral("application/gzip"),
        QStringLiteral("application/x-gzip"),
        QStringLiteral("application/x-bzip2"),
        QStringLiteral("application/x-xz"),
        QStringLiteral("application/x-7z-compressed"),
        QStringLiteral("application/vnd.rar"),
        QStringLiteral("application/x-rar-compressed"),
    };
    if (archives.contains(type)) {
        return QStringLiteral("archive-folder");
    }
    if (major == QLatin1String("text") || type.contains(QLatin1String("wordprocessing")) ||
        type == QLatin1String("application/msword") || type == QLatin1String("application/rtf") ||
        type == QLatin1String("application/epub+zip")) {
        return QStringLiteral("document");
    }
    return {};
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
    case Role::Retryable:
        return canRetry(download);
    case Role::FileExists:
        return download.status == Done && !download.path.isEmpty() &&
               QFileInfo(download.path).isFile();
    case Role::Icon:
        return iconFor(download.mimeType, download.name);
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
        {roleId(Role::Retryable), QByteArrayLiteral("retryable")},
        {roleId(Role::FileExists), QByteArrayLiteral("fileExists")},
        {roleId(Role::Icon), QByteArrayLiteral("icon")},
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

QString DownloadModel::directoryUrl() const
{
    return QUrl::fromLocalFile(m_directory).toString();
}

int DownloadModel::runningCount() const
{
    return m_runningCount;
}

int DownloadModel::runningProgress() const
{
    return m_runningProgress;
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
        // A download the engine starts again -- retried after it failed, or after it
        // was stopped -- keeps its id, and its row. One fetched anew for a row from an
        // earlier run has a new id, and goes where that row's file went.
        if (row >= 0) {
            restart(row, engineId);
            return;
        }
        const int refetched =
            rowRefetching(m_downloads, message.value(QStringLiteral("targetPath")).toString());
        if (refetched >= 0) {
            restart(refetched, engineId);
        } else {
            start(engineId, message);
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
    // One still coming may be forgotten; the engine goes on with it, unheard.
    if (recountRunning(m_downloads, m_runningCount, m_runningProgress)) {
        emit runningChanged();
    }
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
    clearSince(std::numeric_limits<double>::lowest());
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

QVariantMap DownloadModel::details(int downloadId) const
{
    const int row = rowOf(downloadId);
    if (row < 0) {
        return {};
    }
    QVariantMap roles;
    const QHash<int, QByteArray> names = roleNames();
    for (auto it = names.cbegin(); it != names.cend(); ++it) {
        roles.insert(QString::fromLatin1(it.value()), data(index(row, 0), it.key()));
    }
    return roles;
}

void DownloadModel::stop(int row)
{
    if (row < 0 || row >= m_downloads.count()) {
        return;
    }
    const Download &download = m_downloads.at(row);
    if (download.status != Running || download.engineId == 0) {
        return;
    }
    emit engineRequest(CommandTopic, QVariantMap{
                                         {QStringLiteral("msg"), QStringLiteral("cancelDownload")},
                                         {QStringLiteral("id"), download.engineId},
                                     });
}

void DownloadModel::retry(int row)
{
    if (row < 0 || row >= m_downloads.count() || !canRetry(m_downloads.at(row))) {
        return;
    }
    Download &download = m_downloads[row];
    if (download.engineId != 0) {
        emit engineRequest(CommandTopic,
                           QVariantMap{
                               {QStringLiteral("msg"), QStringLiteral("retryDownload")},
                               {QStringLiteral("id"), download.engineId},
                           });
        return;
    }
    download.refetching = true;
    emit engineRequest(CommandTopic, QVariantMap{
                                         {QStringLiteral("msg"), QStringLiteral("addDownload")},
                                         {QStringLiteral("from"), download.url},
                                         {QStringLiteral("to"), download.path},
                                     });
}

bool DownloadModel::deleteFile(int row)
{
    if (row < 0 || row >= m_downloads.count()) {
        return false;
    }
    const QString path = m_downloads.at(row).path;
    // The engine stops writing it, and takes away what it had written.
    stop(row);
    if (!path.isEmpty() && QFileInfo(path).isFile() && !QFile::remove(path)) {
        qWarning() << "DownloadModel: cannot delete" << path;
        return false;
    }
    remove(row);
    return true;
}

void DownloadModel::refresh()
{
    if (m_downloads.isEmpty()) {
        return;
    }
    emit dataChanged(index(0, 0), index(m_downloads.count() - 1, 0), {roleId(Role::FileExists)});
}

bool DownloadModel::canRetry(const Download &download)
{
    if (download.status != Failed && download.status != Canceled) {
        return false;
    }
    if (download.engineId != 0) {
        return true;
    }
    // Forgotten by the engine, it can only be fetched again from an address that still
    // means something without the page that started it: not a blob: or data: URL.
    const QString scheme = QUrl(download.url).scheme();
    return !download.path.isEmpty() &&
           (scheme == QLatin1String("http") || scheme == QLatin1String("https"));
}

QString DownloadModel::iconFor(const QString &mimeType, const QString &name)
{
    QString kind = kindOfType(mimeType);
    if (kind.isEmpty()) {
        kind = kindOfExtension(name);
    }
    if (kind.isEmpty()) {
        kind = QStringLiteral("other");
    }
    return QStringLiteral("image://theme/icon-m-file-") + kind;
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
    emit downloadStarted(download.id);
}

void DownloadModel::restart(int row, int engineId)
{
    Download &download = m_downloads[row];
    download.engineId = engineId;
    download.refetching = false;
    if (download.status == Running) {
        return;
    }
    // From the start: the engine kept nothing of the file when it stopped.
    download.status = Running;
    QVector<int> roles{roleId(Role::Status)};
    if (download.progress != 0) {
        download.progress = 0;
        roles.append(roleId(Role::Progress));
    }
    store(download);
    changed(row, roles);
    emit downloadStarted(download.id);
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
    const bool arrived = roles.contains(roleId(Role::Status));
    store(download);
    changed(row, roles);
    if (arrived) {
        emit downloadEnded(download.id, Done);
    }
}

void DownloadModel::setStatus(int row, Status status)
{
    Download &download = m_downloads[row];
    if (download.status == status) {
        return;
    }
    download.status = status;
    store(download);
    changed(row, {roleId(Role::Status)});
    emit downloadEnded(download.id, status);
}

void DownloadModel::changed(int row, const QVector<int> &roles)
{
    // What is worked out of a row changes with what it is worked out of: whether it can
    // be fetched again with its status, and whether its file is there with that and its
    // path.
    QVector<int> all = roles;
    const bool status = roles.contains(roleId(Role::Status));
    if (status) {
        all.append(roleId(Role::Retryable));
    }
    if (status || roles.contains(roleId(Role::Path))) {
        all.append(roleId(Role::FileExists));
    }
    const QModelIndex modelIndex = index(row, 0);
    emit dataChanged(modelIndex, modelIndex, all);
    // Every change to a row's status or progress comes through here.
    if (recountRunning(m_downloads, m_runningCount, m_runningProgress)) {
        emit runningChanged();
    }
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
