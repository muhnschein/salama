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

// Own downloads list, from "embed:download". Platform Transfers unusable: Sailjail blocks
// showTransfers, WebView downloads never land there. Engine id restarts at 1 per process; rows
// have own lasting id. Pause = engine cancel (keeps data), resume = retry.
class DownloadModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QString topic READ topic CONSTANT)
    Q_PROPERTY(QString directory READ directory CONSTANT)
    // Mean of percents, not bytes: engine size 0 while unknown.
    Q_PROPERTY(int runningCount READ runningCount NOTIFY runningChanged)
    Q_PROPERTY(int runningProgress READ runningProgress NOTIFY runningChanged)
    // This run's unarrived, unswiped downloads. Progress excludes failed.
    Q_PROPERTY(int trayCount READ trayCount NOTIFY trayChanged)
    Q_PROPERTY(int trayFailed READ trayFailed NOTIFY trayChanged)
    Q_PROPERTY(int trayPaused READ trayPaused NOTIFY trayChanged)
    Q_PROPERTY(int trayProgress READ trayProgress NOTIFY trayChanged)
    Q_PROPERTY(QStringList trayNames READ trayNames NOTIFY trayChanged)
    Q_PROPERTY(int trayStatus READ trayStatus NOTIFY trayChanged)
    Q_PROPERTY(double traySize READ traySize NOTIFY trayChanged)

public:
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
        Resumable,
        FileExists
    };

    static const int Limit = 50;

    struct Download
    {
        int id = 0;
        // 0 = earlier run.
        int engineId = 0;
        QString name;
        QString url;
        QString path;
        QString mimeType;
        qint64 size = 0;
        int progress = 0;
        Status status = Running;
        // ms since epoch.
        qint64 started = 0;
        // Not stored.
        bool dismissed = false;
    };

    // Creates dir: engine falls back to ~/Downloads if missing.
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
    const QList<Download> &downloads() const;

    Q_INVOKABLE void observe(const QString &topic, const QVariant &data);

    // Running download paused first so nothing arrives untracked.
    Q_INVOKABLE void remove(int row);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void clearFinished();

    Q_INVOKABLE void pause(int row);
    Q_INVOKABLE void resume(int row);

    // Numbered "map(1).pdf" on clash, like Firefox. http(s) only. Returns path or "".
    Q_INVOKABLE QString save(const QString &url, const QString &contentType);

    // Only arrived files under ~/Downloads (Downloads permission scope).
    Q_INVOKABLE bool deleteFile(int row);

    Q_INVOKABLE void refreshFiles();

    Q_INVOKABLE void dismissTray();
    Q_INVOKABLE void clearSince(double since);
    Q_INVOKABLE int countSince(double since) const;

    Q_INVOKABLE QString fileUrl(int row) const;
    Q_INVOKABLE int rowOf(int downloadId) const;
    Q_INVOKABLE static QString formatSize(double bytes);

signals:
    void countChanged();
    void runningChanged();
    void trayChanged();
    void engineRequest(const QString &topic, const QVariantMap &data);
    void finished(int downloadId, const QString &name);

private:
    void start(int engineId, const QVariantMap &message);
    void setProgress(int row, const QVariant &percent);
    void finish(int row, const QString &path);
    void setStatus(int row, Status status);
    void changed(int row, const QVector<int> &roles);
    void recount();
    void request(const QVariantMap &data);
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
    // Requested; engine may not have started yet.
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
