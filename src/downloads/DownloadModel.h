// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "ModelRoles.h"

#include <QAbstractListModel>
#include <QList>
#include <QSet>
#include <QSqlDatabase>
#include <QString>
#include <QStringList>
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
// What the engine is told goes the other way, on topic "embedui:download", and the
// model only says it -- engineRequest() -- for the browsing page to hand to WebEngine,
// which is kept to that page (docs/DECISIONS/0038-download-status.md):
//
//  * cancelDownload {id}: Gecko's Download.cancel(), which keeps what has arrived
//  * retryDownload {id}: Download.start(), which goes on from there where it can
//  * addDownload {from, to}: a new download of the same file, for one the engine forgot
//
// So a download is paused by cancelling it and resumed by starting it again; the engine
// has no stop that is not also a pause, and the list says "Paused" for what it sends as
// dl-cancel.
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
    // The downloads the browsing page's banner speaks for: the ones of this run that
    // have not arrived -- coming, paused or failed -- and have not been swiped away
    // (dismissTray()), newest first. How many, how many of those failed and how many
    // are paused, their names, and the newest one's state and size, which is what the
    // banner shows when it is the only one. Progress is the mean of their percentages,
    // as runningProgress is, failed ones left out.
    Q_PROPERTY(int trayCount READ trayCount NOTIFY trayChanged)
    Q_PROPERTY(int trayFailed READ trayFailed NOTIFY trayChanged)
    Q_PROPERTY(int trayPaused READ trayPaused NOTIFY trayChanged)
    Q_PROPERTY(int trayProgress READ trayProgress NOTIFY trayChanged)
    Q_PROPERTY(QStringList trayNames READ trayNames NOTIFY trayChanged)
    Q_PROPERTY(int trayStatus READ trayStatus NOTIFY trayChanged)
    Q_PROPERTY(double traySize READ traySize NOTIFY trayChanged)

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
        // The engine still has it, so pausing or resuming it goes on from where it was:
        // one of this run's that has not arrived.
        Resumable,
        // It has arrived, and its file is still where it was saved.
        FileExists
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
        // Swiped off the banner; it comes back when its state changes. Not stored.
        bool dismissed = false;
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
    int trayCount() const;
    int trayFailed() const;
    int trayPaused() const;
    int trayProgress() const;
    QStringList trayNames() const;
    int trayStatus() const;
    double traySize() const;
    // The rows as the list shows them, newest first, for the address bar's suggestions
    // (docs/DECISIONS/0027-omnibar.md).
    const QList<Download> &downloads() const;

    // What WebEngine.recvObserve() delivered: the data is the engine's JSON, already
    // read into a map (qtmozembed src/qmozcontext.cpp). Any other topic, a message it
    // does not know, and an id it has not seen start are ignored, and so is an id or a
    // percentage that is not a number as the engine sends one (engine/EngineData.h).
    Q_INVOKABLE void observe(const QString &topic, const QVariant &data);

    // Forget rows. The files stay where they are; a download still coming is paused
    // first, so that nothing goes on arriving that no list knows of.
    Q_INVOKABLE void remove(int row);
    Q_INVOKABLE void clear();
    // Forget the rows of the downloads that have arrived.
    Q_INVOKABLE void clearFinished();

    // Pause a download that is coming, and resume one that is paused or failed: from
    // where it was when the engine still has it, and otherwise anew, in a row of its
    // own that takes this one's place. Anything else is left as it is.
    Q_INVOKABLE void pause(int row);
    Q_INVOKABLE void resume(int row);

    // A new download of a link or a picture: the link sheet's Save link and Save image
    // (docs/DECISIONS/0046-link-menu.md). The engine is asked to fetch the address into
    // the downloads folder, under the name its path ends in -- the host's, for a path
    // that ends in none -- with the usual ending for the type added to a name that has
    // none, and numbered before the ending, "map(1).pdf", when a file, a row or a save
    // asked for earlier already has the name, as Firefox numbers them. Only http and
    // https. Answers the path asked for, or empty when nothing was asked.
    Q_INVOKABLE QString save(const QString &url, const QString &contentType);

    // Delete a download's file and forget its row. Only a file that has arrived, and
    // only under the downloads folder's own parent -- ~/Downloads, where the engine
    // saves when the folder is gone -- which is where the Downloads permission lets
    // the application write. Answers whether the row went: a file already gone takes
    // its row with it, one that cannot be deleted leaves both.
    Q_INVOKABLE bool deleteFile(int row);

    // Say again whether each file is still there: something else may have deleted one.
    Q_INVOKABLE void refreshFiles();

    // Take what the banner shows off it, until a download starts or changes state.
    Q_INVOKABLE void dismissTray();
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
    // A number of bytes as people read one: "512 B", "7.4 MB", "12 MB", in steps of
    // 1024 and with a decimal below ten, in the locale's own digits.
    Q_INVOKABLE static QString formatSize(double bytes);

signals:
    void countChanged();
    void runningChanged();
    void trayChanged();
    // Something to tell the engine, for WebEngine.notifyObservers().
    void engineRequest(const QString &topic, const QVariantMap &data);
    // A download arrived: what the banner says for a moment.
    void finished(int downloadId, const QString &name);

private:
    void start(int engineId, const QVariantMap &message);
    void setProgress(int row, const QVariant &percent);
    void finish(int row, const QString &path);
    void setStatus(int row, Status status);
    void changed(int row, const QVector<int> &roles);
    void recount();
    void request(const QVariantMap &data);
    // A path in the downloads folder for this name that no file, row or save has.
    QString freePath(const QString &name) const;
    void removeRow(int row);
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
    // The paths save() has asked the engine for, which it may not have started yet.
    QSet<QString> m_saved;

    struct Tray
    {
        int count = 0;
        int failed = 0;
        int paused = 0;
        int progress = 0;
        QStringList names;
        int status = Running;
        qint64 size = 0;

        bool operator==(const Tray &other) const;
    };
    Tray m_tray;
};

} // namespace Salama
