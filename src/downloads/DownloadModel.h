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
class DownloadModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    // The observer topic to subscribe to, for WebEngine.addObserver().
    Q_PROPERTY(QString topic READ topic CONSTANT)
    // The folder the engine is told to save into, for WebEngineSettings.downloadDir.
    Q_PROPERTY(QString directory READ directory CONSTANT)
    // How many downloads are still coming, and how far along they are together, as a
    // percentage: what the ring round the menu's Downloads says at a glance
    // (docs/DECISIONS/0021-menu-sheet.md). Together is the mean of their percentages,
    // not of their bytes: the engine's size is 0 while it does not know it, and a
    // download of unknown size would count for nothing in a sum of bytes. 0 while
    // nothing is coming.
    Q_PROPERTY(int runningCount READ runningCount NOTIFY runningChanged)
    Q_PROPERTY(int runningProgress READ runningProgress NOTIFY runningChanged)
    // How many downloads ended while the ones still coming have been coming, and how
    // many of those failed: what the strip over the page counts beside them
    // (docs/DECISIONS/0038-download-notice.md). A batch, not the fifty the list keeps,
    // and 0 while nothing is coming, when the strip is away.
    Q_PROPERTY(int finishedCount READ finishedCount NOTIFY finishedChanged)
    Q_PROPERTY(int failedCount READ failedCount NOTIFY finishedChanged)
    // How long the downloads still coming will take, rounded to whole seconds, at the
    // pace they have been going: what the strip over the page says under its ring
    // (docs/DECISIONS/0038-download-notice.md). Working from their sizes would be
    // surer, but the engine reports a percentage alone (EmbedliteDownloadManager.js);
    // a pace is two percentages and the time between them. -1 while it cannot be said:
    // nothing is coming, or nothing has moved yet. Not time: the estimate moves with
    // progress, and is only asked of.
    Q_PROPERTY(qint64 etaSeconds READ etaSeconds NOTIFY etaChanged)
    // The one download still coming, by name: what the strip shows instead of how many
    // when there is no asking which. Empty while none or several are coming.
    Q_PROPERTY(QString runningName READ runningName NOTIFY runningChanged)

public:
    // Unscoped, as TabModel::MediaState is: QML reads `DownloadModel.Running`.
    enum Status // NOSONAR(cpp:S3642) QML on Qt 5.6 reads no scoped enum
    {
        Running,
        Done,
        Failed,
        Canceled,
        // The engine's cancel is a pause: the partial file stays, so the same download
        // can start again (docs/DECISIONS/0038-download-notice.md). This status is for
        // the list's sake, remembered here as long as the row is.
        Paused
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
        Started
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
        // When it ended, in milliseconds since the epoch, or 0 while it is coming or
        // from an earlier run. Not written down: it is only true for this process.
        qint64 finished = 0;
        // How fast it has been coming, and when it last said so: the pace the estimate
        // of the time left works from, in percent of the file per second, over the
        // milliseconds since the epoch the last progress report was at. Like finished,
        // only true for this process, and reset when the download starts over.
        qreal rate = 0;
        qint64 progressAt = 0;
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
    int runningCount() const;
    int runningProgress() const;
    // Downloads that ended while the ones still coming have been coming, and how many
    // of those failed. A download forgotten is not among them: it is gone from the list.
    int finishedCount() const;
    int failedCount() const;
    qint64 etaSeconds() const;
    QString runningName() const;
    // The rows as the list shows them, newest first, for the address bar's suggestions
    // (docs/DECISIONS/0027-omnibar.md).
    const QList<Download> &downloads() const;

    // What WebEngine.recvObserve() delivered: the data is the engine's JSON, already
    // read into a map (qtmozembed src/qmozcontext.cpp). Any other topic, a message it
    // does not know, and an id it has not seen start are ignored, and so is an id or a
    // percentage that is not a number as the engine sends one (engine/EngineData.h).
    Q_INVOKABLE void observe(const QString &topic, const QVariant &data);

    // Forget rows. The files stay where they are.
    Q_INVOKABLE void remove(int row);
    Q_INVOKABLE void clear();
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

    // What a row's menu asks of a download, by the id that lasts: pause -- the engine's
    // cancel, which keeps the partial file, so resuming can start again from it;
    // resume -- the engine's retry; cancel -- stop, and leave the row to say so. The
    // engine is told, and the row's status is the list's to keep. A row that is not
    // where the menu sees it -- already gone -- is left alone.
    Q_INVOKABLE void pause(int downloadId);
    Q_INVOKABLE void resume(int downloadId);
    Q_INVOKABLE void cancel(int downloadId);
    // The saved file of a download that has arrived: whether it is still there, and
    // removing it from the folder. The row stays, and no longer has a file to open.
    Q_INVOKABLE bool hasFile(int downloadId);
    Q_INVOKABLE void deleteFile(int downloadId);

signals:
    void countChanged();
    void runningChanged();
    // What finishedCount and failedCount stand for has moved: a download ended, or one
    // that did was forgotten. Said beside runningChanged, which a download ending moves
    // too (docs/DECISIONS/0038-download-notice.md).
    void finishedChanged();
    // What the estimate of the time left reads has moved. Said with the changes that
    // can move the estimate: a download's progress, and which rows there are.
    void etaChanged();
    // Something the engine must be told, as NotificationPermissions::engineRequest:
    // the topic to observe and the payload to send. The browsing page, which has the
    // engine, sends it (docs/ARCHITECTURE.md).
    void engineRequest(const QString &topic, const QVariant &payload);

private:
    void start(int engineId, const QVariantMap &message);
    void setProgress(int row, const QVariant &percent);
    void finish(int row, const QString &path);
    void setStatus(int row, Status status);
    void changed(int row, const QVector<int> &roles);
    void dropOldest();
    // When the earliest download still coming started, or 0 when nothing is: what the
    // finished counts are measured from.
    qint64 earliestRunning() const;
    // How long the downloads still coming will take, at the pace of the one that has
    // the furthest to go, in whole seconds, or -1 where no pace is known.
    qint64 estimate() const;

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
