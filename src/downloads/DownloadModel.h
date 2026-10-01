// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "ModelRoles.h"

#include <QAbstractListModel>
#include <QList>
#include <QSqlDatabase>
#include <QString>
#include <QVariant>
#include <QVector>

namespace Salama {

class Storage;

// The browser's own list of downloads, newest first.
//
// The platform keeps a list of transfers, Settings > Transfers, but it is not this
// application's to use. A Harbour application under Sailjail may not open it: only the
// platform's own applications' profiles may call com.jolla.settings.ui.showTransfers
// (sailjail-permissions permissions/sailfish-browser.profile, jolla-contacts.profile),
// and no permission a Harbour application may ask for grants it. And nothing would be in
// it if it could: nothing registers a Sailfish.WebView application's downloads there.
// sailfish-browser creates those transfers itself, in apps/core/downloadmanager.cpp,
// from the engine's "embed:download" observer notifications, which
// sailfish-components-webview leaves alone. So this model listens to the same
// notifications, and keeps what they say.
//
// What the engine sends is embedlite-components jscomps/EmbedliteDownloadManager.js:
// topic "embed:download", and a map whose "msg" is one of
//
//  * dl-start {id, displayName, sourceUrl, targetPath, mimeType, size, saveAsPdf}
//  * dl-progress {id, percent}
//  * dl-done {id, targetPath}
//  * dl-fail {id}
//  * dl-cancel {id}
//
// The id is the engine's, counted from 1 each time the engine starts, so it names a
// download only for as long as this process runs; the rows carry an id of their own,
// which lasts. A dl-start for an id already seen is the same download started again.
//
// What the engine is told back goes on topic "embedui:download", which the same file
// observes: "cancelDownload" {id} and "retryDownload" {id} for a download of this run,
// and "addDownload" {from, to} to fetch a file anew. The engine is not this model's to
// reach -- WebEngine is the browsing page's (docs/ARCHITECTURE.md) -- so the model says
// what to send with engineRequest(), and the browsing page sends it
// (docs/DECISIONS/0038-download-controls.md).
class DownloadModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    // The observer topic to subscribe to, for WebEngine.addObserver().
    Q_PROPERTY(QString topic READ topic CONSTANT)
    // The folder the engine is told to save into, for WebEngineSettings.downloadDir.
    Q_PROPERTY(QString directory READ directory CONSTANT)
    // The same folder as a URL, for opening it in the file manager.
    Q_PROPERTY(QString directoryUrl READ directoryUrl CONSTANT)
    // How many downloads are still coming, and how far along they are together, as a
    // percentage: what the ring round the menu's Downloads says at a glance
    // (docs/DECISIONS/0021-menu-sheet.md). Together is the mean of their percentages,
    // not of their bytes: the engine's size is 0 while it does not know it, and a
    // download of unknown size would count for nothing in a sum of bytes. 0 while
    // nothing is coming.
    Q_PROPERTY(int runningCount READ runningCount NOTIFY runningChanged)
    Q_PROPERTY(int runningProgress READ runningProgress NOTIFY runningChanged)

public:
    // Unscoped, as TabModel::MediaState is: QML reads `DownloadModel.Running`.
    enum Status // NOSONAR(cpp:S3642) QML on Qt 5.6 reads no scoped enum
    {
        Running,
        Done,
        Failed,
        Canceled
    };
    Q_ENUM(Status)

    enum class Role
    {
        DownloadId = Qt::UserRole + 1,
        Name,
        Url,
        Path,
        MimeType,
        Size,
        Progress,
        Status,
        Started,
        // A failed or stopped download can be fetched again (canRetry()).
        Retryable,
        // The file is where the engine saved it: a download that arrived, and has not
        // been moved or deleted since.
        FileExists,
        // The theme's icon for the kind of file it is (iconFor()).
        Icon
    };

    // The oldest go beyond this many, from the list and from the database.
    static const int Limit = 50;

    struct Download
    {
        int id = 0;
        // The engine's id for it; zero for a download from an earlier run, which no
        // message will name again.
        int engineId = 0;
        QString name;
        QString url;
        QString path;
        QString mimeType;
        qint64 size = 0;
        int progress = 0;
        Status status = Running;
        // Milliseconds since the epoch.
        qint64 started = 0;
        // Asked for again with addDownload, and waiting for the engine to start it:
        // the dl-start that does names a new id, and this row's path (observe()).
        bool refetching = false;
    };

    // The directory is made here, parents and all, if it is missing: the engine saves
    // into it only if it is already there, and into ~/Downloads otherwise
    // (docs/DECISIONS/0025-downloads-folder.md).
    DownloadModel(const Storage &storage, QString directory, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;
    QString topic() const;
    QString directory() const;
    QString directoryUrl() const;
    int runningCount() const;
    int runningProgress() const;
    // The rows as the list shows them, newest first, for the address bar's suggestions
    // (docs/DECISIONS/0027-omnibar.md).
    const QList<Download> &downloads() const;

    // What WebEngine.recvObserve() delivered: the data is the engine's JSON, already
    // read into a map (qtmozembed src/qmozcontext.cpp). Any other topic, a message it
    // does not know, and an id it has not seen start are ignored, and so is an id or a
    // percentage that is not a number as the engine sends one (engine/EngineData.h).
    Q_INVOKABLE void observe(const QString &topic, const QVariant &data);

    // Forget rows. The files stay where they are. clear() forgets every one, as clearing
    // the history on close does; clearEnded() leaves those still coming, which would
    // otherwise go on with nothing left to stop them by: what the list's pulley does.
    Q_INVOKABLE void remove(int row);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void clearEnded();
    // The rows of downloads started at or after a time, in milliseconds since the
    // epoch, as HistoryModel::clearSince() takes it, but for any still coming: what
    // clearing the history takes of the list of downloads.
    Q_INVOKABLE void clearSince(double since);
    // How many rows clearSince() would take for the same time: what the dialog that
    // clears says goes of the list.
    Q_INVOKABLE int countSince(double since) const;

    // The file as a URL to open it by, or empty when there is no such row or no file.
    Q_INVOKABLE QString fileUrl(int row) const;
    // The row a download is on, by the id of its own that lasts, or -1 once it has
    // gone: what a list other than this one keeps to find it again by.
    Q_INVOKABLE int rowOf(int downloadId) const;
    // A download's roles by name, as a delegate reads them, by the id that lasts, or
    // nothing once it has gone: for what shows one download outside the list.
    Q_INVOKABLE QVariantMap details(int downloadId) const;

    // Stop a download still coming. The engine drops what it has of the file, and says
    // dl-cancel; there is no pausing one (docs/DECISIONS/0038-download-controls.md).
    Q_INVOKABLE void stop(int row);
    // Fetch a failed or stopped download again, from the start. One of this run the
    // engine still has, and starts again as it was asked for; one from an earlier run
    // it has forgotten, and is asked to fetch from where it came from to where it went.
    Q_INVOKABLE void retry(int row);
    // Delete the file and forget the row; one still coming is stopped first. False,
    // with the row kept, when there is a file and it could not be deleted.
    Q_INVOKABLE bool deleteFile(int row);
    // The files may have been moved or deleted while the list was not looked at.
    Q_INVOKABLE void refresh();

    static bool canRetry(const Download &download);
    // "image://theme/icon-m-file-…" for a type, as the platform's file manager shows
    // one; by the name's extension when the type says nothing useful.
    static QString iconFor(const QString &mimeType, const QString &name);

signals:
    void countChanged();
    void runningChanged();
    // A download started, or started again; one ended -- arrived, failed or stopped --
    // with the Status it ended in. By the id that lasts.
    void downloadStarted(int downloadId);
    void downloadEnded(int downloadId, int status);
    // What the engine is to be told, for WebEngine.notifyObservers().
    void engineRequest(const QString &topic, const QVariant &data);

private:
    void start(int engineId, const QVariantMap &message);
    void restart(int row, int engineId);
    void setProgress(int row, const QVariant &percent);
    void finish(int row, const QString &path);
    void setStatus(int row, Status status);
    void changed(int row, const QVector<int> &roles);
    void dropOldest();

    void load();
    void insert(const Download &download) const;
    void store(const Download &download) const;
    void erase(int id) const;

    QSqlDatabase m_db;
    QString m_directory;
    QList<Download> m_downloads;
    int m_nextId = 1;
    int m_runningCount = 0;
    int m_runningProgress = 0;
};

} // namespace Salama
