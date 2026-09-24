// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

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

public:
    enum Status
    {
        Running,
        Done,
        Failed,
        Canceled
    };
    Q_ENUM(Status)

    enum Role
    {
        DownloadIdRole = Qt::UserRole + 1,
        NameRole,
        UrlRole,
        PathRole,
        MimeTypeRole,
        SizeRole,
        ProgressRole,
        StatusRole,
        StartedRole
    };

    // The oldest go beyond this many, from the list and from the database.
    static const int Limit = 50;

    // The directory is made here, parents and all, if it is missing: the engine saves
    // into it only if it is already there, and into ~/Downloads otherwise
    // (docs/DECISIONS/0025-downloads-folder.md).
    DownloadModel(Storage &storage, QString directory, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;
    QString topic() const;
    QString directory() const;

    // What WebEngine.recvObserve() delivered: the data is the engine's JSON, already
    // read into a map (qtmozembed src/qmozcontext.cpp). Any other topic, a message it
    // does not know, and an id it has not seen start are ignored, and so is an id or a
    // percentage that is not a number as the engine sends one (engine/EngineData.h).
    Q_INVOKABLE void observe(const QString &topic, const QVariant &data);

    // Forget rows. The files stay where they are.
    Q_INVOKABLE void remove(int row);
    Q_INVOKABLE void clear();

    // The file as a URL to open it by, or empty when there is no such row or no file.
    Q_INVOKABLE QString fileUrl(int row) const;

signals:
    void countChanged();

private:
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
    };

    void start(int engineId, const QVariantMap &message);
    void setProgress(int row, const QVariant &percent);
    void finish(int row, const QString &path);
    void setStatus(int row, Status status);
    void changed(int row, const QVector<int> &roles);
    int rowForEngineId(int engineId) const;
    void dropOldest();

    void load();
    void insert(const Download &download);
    void store(const Download &download);
    void erase(int id);

    QSqlDatabase m_db;
    QString m_directory;
    QList<Download> m_downloads;
    int m_nextId = 1;
};

} // namespace Salama
