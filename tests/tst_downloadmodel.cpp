// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "downloads/DownloadModel.h"
#include "storage/Storage.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMetaEnum>
#include <QSignalSpy>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QUrl>
#include <QtTest>

using Salama::DownloadModel;
using Salama::Storage;

class tst_downloadmodel : public QObject
{
    Q_OBJECT

private slots:
    void topicRolesAndStatuses();
    void startsAtTheTop();
    void nameFallsBackToTheFile();
    void progress();
    void running();
    void wholeNumbers();
    void done();
    void failAndCancel();
    void restart();
    void ignoresWhatItDoesNotKnow();
    void persists();
    void runningFailsOnReload();
    void limit();
    void remove();
    void clear();
    void clearSince();
    void fileUrl();
    void rowOf();
    void directory();
    void withoutDatabase();
    void pauseAndResume();
    void downloadAgain();
    void saveKeepsTheFilesName();
    void removeAndClearPause();
    void clearFinished();
    void deleteFile();
    void deleteFileStaysInDownloads();
    void fileExists();
    void formatSize_data();
    void formatSize();
    void tray();
    void trayDismissed();
    void finished();
};

namespace {

const QString Topic = QStringLiteral("embed:download");
const QString Downloads = QStringLiteral("/home/defaultuser/Downloads/");

// EmbedliteDownloadManager.js start message via qtmozembed on device Qt 5.6: every number double.
QVariantMap startMessage(int id, const QString &file)
{
    return {
        {QStringLiteral("msg"), QStringLiteral("dl-start")},
        {QStringLiteral("id"), static_cast<double>(id)},
        {QStringLiteral("saveAsPdf"), false},
        {QStringLiteral("displayName"), file},
        {QStringLiteral("sourceUrl"), QStringLiteral("https://files.example/") + file},
        {QStringLiteral("targetPath"), Downloads + file},
        {QStringLiteral("mimeType"), QStringLiteral("application/pdf")},
        {QStringLiteral("size"), 2048.0},
    };
}

QVariantMap message(const QString &msg, int id)
{
    return {{QStringLiteral("msg"), msg}, {QStringLiteral("id"), static_cast<double>(id)}};
}

QVariantMap progressMessage(int id, const QVariant &percent)
{
    QVariantMap progress = message(QStringLiteral("dl-progress"), id);
    progress.insert(QStringLiteral("percent"), percent);
    return progress;
}

QVariant role(const DownloadModel &model, int row, int role)
{
    return model.data(model.index(row, 0), role);
}

int rowsInDatabase(const Storage &storage)
{
    QSqlQuery query(storage.database());
    query.exec(QStringLiteral("SELECT COUNT(*) FROM download"));
    return query.next() ? query.value(0).toInt() : -1;
}

int storedStatus(const Storage &storage, int downloadId)
{
    QSqlQuery query(storage.database());
    query.prepare(QStringLiteral("SELECT status FROM download WHERE id = ?"));
    query.addBindValue(downloadId);
    query.exec();
    return query.next() ? query.value(0).toInt() : -1;
}

QVector<int> statusRoles()
{
    return {roleId(DownloadModel::Role::Status), roleId(DownloadModel::Role::Resumable)};
}

QVector<int> changedRoles(const QSignalSpy &spy)
{
    return spy.last().at(2).value<QVector<int>>();
}

} // namespace

void tst_downloadmodel::topicRolesAndStatuses()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    DownloadModel model(storage, dir.path());
    // Same topic sailfish-browser's DownloadManager listens on.
    QCOMPARE(model.topic(), Topic);
    QCOMPARE(model.count(), 0);

    const QHash<int, QByteArray> roles = model.roleNames();
    QCOMPARE(roles.value(roleId(DownloadModel::Role::DownloadId)), QByteArray("downloadId"));
    QCOMPARE(roles.value(roleId(DownloadModel::Role::Name)), QByteArray("name"));
    QCOMPARE(roles.value(roleId(DownloadModel::Role::Url)), QByteArray("url"));
    QCOMPARE(roles.value(roleId(DownloadModel::Role::Path)), QByteArray("path"));
    QCOMPARE(roles.value(roleId(DownloadModel::Role::MimeType)), QByteArray("mimeType"));
    QCOMPARE(roles.value(roleId(DownloadModel::Role::Size)), QByteArray("size"));
    QCOMPARE(roles.value(roleId(DownloadModel::Role::Progress)), QByteArray("progress"));
    QCOMPARE(roles.value(roleId(DownloadModel::Role::Status)), QByteArray("status"));
    QCOMPARE(roles.value(roleId(DownloadModel::Role::Started)), QByteArray("started"));
    QCOMPARE(roles.value(roleId(DownloadModel::Role::Resumable)), QByteArray("resumable"));
    QCOMPARE(roles.value(roleId(DownloadModel::Role::FileExists)), QByteArray("fileExists"));
    QCOMPARE(roles.count(), 11);

    // QML compares status by name, DB stores numbers: neither may move.
    const QMetaEnum status = QMetaEnum::fromType<DownloadModel::Status>();
    QCOMPARE(status.keyCount(), 4);
    QCOMPARE(status.keyToValue("Running"), 0);
    QCOMPARE(status.keyToValue("Done"), 1);
    QCOMPARE(status.keyToValue("Failed"), 2);
    QCOMPARE(status.keyToValue("Canceled"), 3);

    model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
    QCOMPARE(model.rowCount(model.index(0, 0)), 0);
    QVERIFY(!role(model, 1, roleId(DownloadModel::Role::Name)).isValid());
    QVERIFY(!role(model, -1, roleId(DownloadModel::Role::Name)).isValid());
    QVERIFY(!role(model, 0, Qt::DisplayRole).isValid());
}

void tst_downloadmodel::startsAtTheTop()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    DownloadModel model(storage, dir.path());
    QSignalSpy countSpy(&model, &DownloadModel::countChanged);
    QSignalSpy insertSpy(&model, &DownloadModel::rowsInserted);

    const qint64 before = QDateTime::currentMSecsSinceEpoch();
    model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
    const qint64 after = QDateTime::currentMSecsSinceEpoch();
    QCOMPARE(model.count(), 1);
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(countSpy.count(), 1);
    QCOMPARE(insertSpy.count(), 1);
    QCOMPARE(insertSpy.last().at(1).toInt(), 0);

    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::DownloadId)).toInt(), 1);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Name)).toString(), QStringLiteral("a.pdf"));
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Url)).toString(),
             QStringLiteral("https://files.example/a.pdf"));
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Path)).toString(),
             Downloads + QStringLiteral("a.pdf"));
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::MimeType)).toString(),
             QStringLiteral("application/pdf"));
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Size)).toLongLong(), 2048LL);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Progress)).toInt(), 0);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Status)).toInt(),
             static_cast<int>(DownloadModel::Running));
    const qint64 started = role(model, 0, roleId(DownloadModel::Role::Started)).toLongLong();
    QVERIFY(started >= before && started <= after);
    QCOMPARE(rowsInDatabase(storage), 1);
    QCOMPARE(storedStatus(storage, 1), static_cast<int>(DownloadModel::Running));

    model.observe(Topic, startMessage(2, QStringLiteral("b.zip")));
    QCOMPARE(model.count(), 2);
    QCOMPARE(countSpy.count(), 2);
    QCOMPARE(insertSpy.last().at(1).toInt(), 0);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Name)).toString(), QStringLiteral("b.zip"));
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::DownloadId)).toInt(), 2);
    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::Name)).toString(), QStringLiteral("a.pdf"));

    QVariantMap unsized = startMessage(3, QStringLiteral("c.bin"));
    unsized.insert(QStringLiteral("size"), 0.0);
    model.observe(Topic, unsized);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Size)).toLongLong(), 0LL);
    QVariantMap negative = startMessage(4, QStringLiteral("d.bin"));
    negative.insert(QStringLiteral("size"), -5.0);
    model.observe(Topic, negative);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Size)).toLongLong(), 0LL);
    // Non-number size too, though QVariant would parse one.
    QVariantMap spelled = startMessage(5, QStringLiteral("e.bin"));
    spelled.insert(QStringLiteral("size"), QStringLiteral("2048"));
    model.observe(Topic, spelled);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Size)).toLongLong(), 0LL);
}

void tst_downloadmodel::nameFallsBackToTheFile()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    DownloadModel model(storage, dir.path());

    QVariantMap nameless = startMessage(1, QStringLiteral("report.pdf"));
    nameless.remove(QStringLiteral("displayName"));
    model.observe(Topic, nameless);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Name)).toString(),
             QStringLiteral("report.pdf"));

    QVariantMap blank = startMessage(2, QStringLiteral("notes.txt"));
    blank.insert(QStringLiteral("displayName"), QString());
    model.observe(Topic, blank);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Name)).toString(),
             QStringLiteral("notes.txt"));
}

void tst_downloadmodel::progress()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    DownloadModel model(storage, dir.path());
    model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
    model.observe(Topic, startMessage(2, QStringLiteral("b.pdf")));
    QSignalSpy changeSpy(&model, &DownloadModel::dataChanged);

    model.observe(Topic, progressMessage(1, 42.0));
    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::Progress)).toInt(), 42);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Progress)).toInt(), 0);
    QCOMPARE(changeSpy.count(), 1);
    QCOMPARE(changeSpy.last().at(0).value<QModelIndex>().row(), 1);
    QCOMPARE(changedRoles(changeSpy), QVector<int>{roleId(DownloadModel::Role::Progress)});

    model.observe(Topic, progressMessage(1, 42.0));
    QCOMPARE(changeSpy.count(), 1);

    model.observe(Topic, progressMessage(1, 250.0));
    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::Progress)).toInt(), 100);
    model.observe(Topic, progressMessage(1, -3.0));
    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::Progress)).toInt(), 0);
    model.observe(Topic, progressMessage(1, 1e30));
    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::Progress)).toInt(), 100);
    model.observe(Topic, progressMessage(1, 66.6));
    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::Progress)).toInt(), 67);
    QCOMPARE(changeSpy.count(), 5);

    // Missing or non-number figure ignored (QVariant would read 42 / 1 from them).
    model.observe(Topic, progressMessage(1, QStringLiteral("most of it")));
    model.observe(Topic, progressMessage(1, QVariant()));
    model.observe(Topic, message(QStringLiteral("dl-progress"), 1));
    model.observe(Topic, progressMessage(1, QStringLiteral("42")));
    model.observe(Topic, progressMessage(1, true));
    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::Progress)).toInt(), 67);
    QCOMPARE(changeSpy.count(), 5);
}

void tst_downloadmodel::running()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    DownloadModel model(storage, dir.path());
    QSignalSpy runningSpy(&model, &DownloadModel::runningChanged);
    QCOMPARE(model.runningCount(), 0);
    QCOMPARE(model.runningProgress(), 0);

    model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
    QCOMPARE(model.runningCount(), 1);
    QCOMPARE(model.runningProgress(), 0);
    QCOMPARE(runningSpy.count(), 1);
    model.observe(Topic, progressMessage(1, 40.0));
    QCOMPARE(model.runningProgress(), 40);
    QCOMPARE(runningSpy.count(), 2);
    QVariantMap unsized = startMessage(2, QStringLiteral("b.iso"));
    unsized.insert(QStringLiteral("size"), 0.0);
    model.observe(Topic, unsized);
    QCOMPARE(model.runningCount(), 2);
    QCOMPARE(model.runningProgress(), 20);
    model.observe(Topic, progressMessage(2, 45.0));
    QCOMPARE(model.runningProgress(), 43);
    const int said = runningSpy.count();
    model.observe(Topic, progressMessage(2, 45.0));
    QCOMPARE(runningSpy.count(), said);

    model.observe(Topic, message(QStringLiteral("dl-done"), 1));
    QCOMPARE(model.runningCount(), 1);
    QCOMPARE(model.runningProgress(), 45);
    model.observe(Topic, message(QStringLiteral("dl-fail"), 2));
    QCOMPARE(model.runningCount(), 0);
    QCOMPARE(model.runningProgress(), 0);
    model.observe(Topic, startMessage(2, QStringLiteral("b.iso")));
    QCOMPARE(model.runningCount(), 1);
    QCOMPARE(model.runningProgress(), 45);
    model.observe(Topic, message(QStringLiteral("dl-cancel"), 2));
    QCOMPARE(model.runningCount(), 0);

    model.observe(Topic, startMessage(3, QStringLiteral("c.pdf")));
    model.observe(Topic, progressMessage(3, 10.0));
    QCOMPARE(model.runningCount(), 1);
    model.remove(0);
    QCOMPARE(model.runningCount(), 0);
    QCOMPARE(model.runningProgress(), 0);
    model.observe(Topic, startMessage(4, QStringLiteral("d.pdf")));
    model.observe(Topic, progressMessage(4, 70.0));
    QCOMPARE(model.runningProgress(), 70);
    model.clear();
    QCOMPARE(model.runningCount(), 0);
    QCOMPARE(model.runningProgress(), 0);

    // After restart nothing running: engine forgot all.
    model.observe(Topic, startMessage(5, QStringLiteral("e.pdf")));
    QCOMPARE(model.runningCount(), 1);
    DownloadModel reloaded(storage, dir.path());
    QCOMPARE(reloaded.runningCount(), 0);
    QCOMPARE(reloaded.runningProgress(), 0);
}

// Whole numbers as qlonglong (Qt 5.15+ JSON) and int (from QML).
void tst_downloadmodel::wholeNumbers()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    DownloadModel model(storage, dir.path());

    QVariantMap start = startMessage(1, QStringLiteral("a.iso"));
    start.insert(QStringLiteral("id"), QVariant(1LL));
    start.insert(QStringLiteral("size"), QVariant(5000000000LL));
    model.observe(Topic, start);
    QCOMPARE(model.count(), 1);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Size)).toLongLong(), 5000000000LL);
    QVariantMap progress = progressMessage(1, QVariant(40LL));
    progress.insert(QStringLiteral("id"), QVariant(1LL));
    model.observe(Topic, progress);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Progress)).toInt(), 40);

    const QVariantMap fromQml{
        {QStringLiteral("msg"), QStringLiteral("dl-start")},
        {QStringLiteral("id"), 2},
        {QStringLiteral("displayName"), QStringLiteral("b.pdf")},
        {QStringLiteral("size"), 10},
    };
    model.observe(Topic, fromQml);
    QCOMPARE(model.count(), 2);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Size)).toLongLong(), 10LL);
    model.observe(Topic, QVariantMap{{QStringLiteral("msg"), QStringLiteral("dl-progress")},
                                     {QStringLiteral("id"), 2},
                                     {QStringLiteral("percent"), 55}});
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Progress)).toInt(), 55);
    model.observe(Topic, QVariantMap{{QStringLiteral("msg"), QStringLiteral("dl-done")},
                                     {QStringLiteral("id"), 2}});
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Status)).toInt(),
             static_cast<int>(DownloadModel::Done));
    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::Status)).toInt(),
             static_cast<int>(DownloadModel::Running));
}

void tst_downloadmodel::done()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    DownloadModel model(storage, dir.path());
    model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
    model.observe(Topic, progressMessage(1, 40.0));
    QSignalSpy changeSpy(&model, &DownloadModel::dataChanged);

    QVariantMap done = message(QStringLiteral("dl-done"), 1);
    done.insert(QStringLiteral("targetPath"), Downloads + QStringLiteral("a(1).pdf"));
    model.observe(Topic, done);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Status)).toInt(),
             static_cast<int>(DownloadModel::Done));
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Progress)).toInt(), 100);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Path)).toString(),
             Downloads + QStringLiteral("a(1).pdf"));
    QCOMPARE(changeSpy.count(), 1);
    QCOMPARE(
        changedRoles(changeSpy),
        QVector<int>({roleId(DownloadModel::Role::Status), roleId(DownloadModel::Role::Resumable),
                      roleId(DownloadModel::Role::FileExists),
                      roleId(DownloadModel::Role::Progress), roleId(DownloadModel::Role::Path)}));
    QCOMPARE(storedStatus(storage, 1), static_cast<int>(DownloadModel::Done));

    model.observe(Topic, done);
    QCOMPARE(changeSpy.count(), 1);

    model.observe(Topic, startMessage(2, QStringLiteral("b.pdf")));
    model.observe(Topic, progressMessage(2, 100.0));
    model.observe(Topic, message(QStringLiteral("dl-done"), 2));
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Path)).toString(),
             Downloads + QStringLiteral("b.pdf"));
    QCOMPARE(changedRoles(changeSpy), QVector<int>({roleId(DownloadModel::Role::Status),
                                                    roleId(DownloadModel::Role::Resumable),
                                                    roleId(DownloadModel::Role::FileExists)}));
    QCOMPARE(storedStatus(storage, 2), static_cast<int>(DownloadModel::Done));
}

void tst_downloadmodel::failAndCancel()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    DownloadModel model(storage, dir.path());
    model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
    model.observe(Topic, startMessage(2, QStringLiteral("b.pdf")));
    QSignalSpy changeSpy(&model, &DownloadModel::dataChanged);

    model.observe(Topic, message(QStringLiteral("dl-fail"), 1));
    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::Status)).toInt(),
             static_cast<int>(DownloadModel::Failed));
    QCOMPARE(changeSpy.count(), 1);
    QCOMPARE(changeSpy.last().at(0).value<QModelIndex>().row(), 1);
    QCOMPARE(changedRoles(changeSpy), statusRoles());
    QCOMPARE(storedStatus(storage, 1), static_cast<int>(DownloadModel::Failed));

    model.observe(Topic, message(QStringLiteral("dl-cancel"), 2));
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Status)).toInt(),
             static_cast<int>(DownloadModel::Canceled));
    QCOMPARE(changeSpy.count(), 2);
    QCOMPARE(changeSpy.last().at(0).value<QModelIndex>().row(), 0);
    QCOMPARE(changedRoles(changeSpy), statusRoles());
    QCOMPARE(storedStatus(storage, 2), static_cast<int>(DownloadModel::Canceled));

    model.observe(Topic, message(QStringLiteral("dl-fail"), 1));
    model.observe(Topic, message(QStringLiteral("dl-cancel"), 2));
    QCOMPARE(changeSpy.count(), 2);
}

// Engine restarts (retry after fail, resume after cancel) with same id + second dl-start:
// same row.
void tst_downloadmodel::restart()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    DownloadModel model(storage, dir.path());
    model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
    model.observe(Topic, progressMessage(1, 30.0));
    model.observe(Topic, message(QStringLiteral("dl-cancel"), 1));
    QSignalSpy countSpy(&model, &DownloadModel::countChanged);
    QSignalSpy changeSpy(&model, &DownloadModel::dataChanged);

    model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
    QCOMPARE(model.count(), 1);
    QCOMPARE(countSpy.count(), 0);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::DownloadId)).toInt(), 1);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Status)).toInt(),
             static_cast<int>(DownloadModel::Running));
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Progress)).toInt(), 30);
    QCOMPARE(changeSpy.count(), 1);
    QCOMPARE(changedRoles(changeSpy), statusRoles());
    QCOMPARE(storedStatus(storage, 1), static_cast<int>(DownloadModel::Running));

    model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
    QCOMPARE(changeSpy.count(), 1);

    model.observe(Topic, message(QStringLiteral("dl-fail"), 1));
    model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
    model.observe(Topic, message(QStringLiteral("dl-done"), 1));
    QCOMPARE(model.count(), 1);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Status)).toInt(),
             static_cast<int>(DownloadModel::Done));
}

void tst_downloadmodel::ignoresWhatItDoesNotKnow()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    DownloadModel model(storage, dir.path());
    QSignalSpy insertSpy(&model, &DownloadModel::rowsInserted);

    model.observe(QStringLiteral("media-decoder-info"), startMessage(1, QStringLiteral("a.pdf")));
    model.observe(QString(), startMessage(1, QStringLiteral("a.pdf")));
    model.observe(Topic, QVariant());
    model.observe(Topic, QStringLiteral("dl-start"));
    model.observe(Topic, QVariantList{1, 2});
    // Starts without valid engine id, incl. ones QVariant would coerce: true, "1", 1.4, > int max.
    for (const QVariant &id :
         {QVariant(), QVariant(0.0), QVariant(-2.0), QVariant(QStringLiteral("first")),
          QVariant(true), QVariant(QStringLiteral("1")), QVariant(1.4), QVariant(3e9),
          QVariant(qQNaN())}) {
        QVariantMap start = startMessage(1, QStringLiteral("a.pdf"));
        start.insert(QStringLiteral("id"), id);
        model.observe(Topic, start);
    }
    QVariantMap idless = startMessage(1, QStringLiteral("a.pdf"));
    idless.remove(QStringLiteral("id"));
    model.observe(Topic, idless);
    for (const QString &msg : {QStringLiteral("dl-progress"), QStringLiteral("dl-done"),
                               QStringLiteral("dl-fail"), QStringLiteral("dl-cancel")}) {
        model.observe(Topic, message(msg, 9));
    }
    QCOMPARE(model.count(), 0);
    QCOMPARE(insertSpy.count(), 0);
    QCOMPARE(rowsInDatabase(storage), 0);

    model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
    QSignalSpy changeSpy(&model, &DownloadModel::dataChanged);
    model.observe(Topic, message(QStringLiteral("dl-pause"), 1));
    model.observe(Topic, message(QString(), 1));
    model.observe(Topic, message(QStringLiteral("retryDownload"), 1));
    for (const QVariant &id : {QVariant(true), QVariant(QStringLiteral("1")), QVariant(1.4)}) {
        QVariantMap fail = message(QStringLiteral("dl-fail"), 1);
        fail.insert(QStringLiteral("id"), id);
        model.observe(Topic, fail);
    }
    QCOMPARE(model.count(), 1);
    QCOMPARE(changeSpy.count(), 0);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Status)).toInt(),
             static_cast<int>(DownloadModel::Running));
}

void tst_downloadmodel::persists()
{
    QTemporaryDir dir;
    qint64 started = 0;
    {
        Storage storage(dir.path());
        DownloadModel model(storage, dir.path());
        model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
        QVariantMap done = message(QStringLiteral("dl-done"), 1);
        done.insert(QStringLiteral("targetPath"), Downloads + QStringLiteral("a(1).pdf"));
        model.observe(Topic, done);
        model.observe(Topic, startMessage(2, QStringLiteral("b.zip")));
        model.observe(Topic, progressMessage(2, 55.0));
        model.observe(Topic, message(QStringLiteral("dl-cancel"), 2));
        started = role(model, 1, roleId(DownloadModel::Role::Started)).toLongLong();
    }

    Storage storage(dir.path());
    DownloadModel model(storage, dir.path());
    QCOMPARE(model.count(), 2);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::DownloadId)).toInt(), 2);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Name)).toString(), QStringLiteral("b.zip"));
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Status)).toInt(),
             static_cast<int>(DownloadModel::Canceled));
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Progress)).toInt(), 0);

    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::DownloadId)).toInt(), 1);
    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::Name)).toString(), QStringLiteral("a.pdf"));
    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::Url)).toString(),
             QStringLiteral("https://files.example/a.pdf"));
    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::Path)).toString(),
             Downloads + QStringLiteral("a(1).pdf"));
    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::MimeType)).toString(),
             QStringLiteral("application/pdf"));
    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::Size)).toLongLong(), 2048LL);
    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::Status)).toInt(),
             static_cast<int>(DownloadModel::Done));
    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::Progress)).toInt(), 100);
    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::Started)).toLongLong(), started);

    // Engine ids don't outlive it: next id 1 is new row.
    model.observe(Topic, startMessage(1, QStringLiteral("c.txt")));
    QCOMPARE(model.count(), 3);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Name)).toString(), QStringLiteral("c.txt"));
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::DownloadId)).toInt(), 3);
    model.observe(Topic, message(QStringLiteral("dl-fail"), 2));
    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::Status)).toInt(),
             static_cast<int>(DownloadModel::Canceled));
}

// Engine forgets downloads on start: running at app stop = failed.
void tst_downloadmodel::runningFailsOnReload()
{
    QTemporaryDir dir;
    {
        Storage storage(dir.path());
        DownloadModel model(storage, dir.path());
        model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
        model.observe(Topic, progressMessage(1, 80.0));
        model.observe(Topic, startMessage(2, QStringLiteral("b.pdf")));
        model.observe(Topic, message(QStringLiteral("dl-done"), 2));
        QSqlQuery query(storage.database());
        QVERIFY(query.exec(QStringLiteral("INSERT INTO download (id, name, status, started) "
                                          "VALUES (7, 'odd', 9, 1)")));
    }

    Storage storage(dir.path());
    DownloadModel model(storage, dir.path());
    QCOMPARE(model.count(), 3);
    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::Name)).toString(), QStringLiteral("a.pdf"));
    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::Status)).toInt(),
             static_cast<int>(DownloadModel::Failed));
    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::Progress)).toInt(), 0);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Status)).toInt(),
             static_cast<int>(DownloadModel::Done));
    QCOMPARE(role(model, 2, roleId(DownloadModel::Role::Name)).toString(), QStringLiteral("odd"));
    QCOMPARE(role(model, 2, roleId(DownloadModel::Role::Status)).toInt(),
             static_cast<int>(DownloadModel::Failed));

    QCOMPARE(storedStatus(storage, 1), static_cast<int>(DownloadModel::Failed));
    QCOMPARE(storedStatus(storage, 2), static_cast<int>(DownloadModel::Done));
    QCOMPARE(storedStatus(storage, 7), static_cast<int>(DownloadModel::Failed));
}

void tst_downloadmodel::limit()
{
    QTemporaryDir dir;
    {
        Storage storage(dir.path());
        DownloadModel model(storage, dir.path());
        QSignalSpy countSpy(&model, &DownloadModel::countChanged);
        QSignalSpy removeSpy(&model, &DownloadModel::rowsRemoved);
        for (int i = 1; i <= DownloadModel::Limit + 5; ++i) {
            model.observe(Topic, startMessage(i, QStringLiteral("f%1.bin").arg(i)));
        }
        QCOMPARE(model.count(), DownloadModel::Limit);
        QCOMPARE(countSpy.count(), DownloadModel::Limit);
        QCOMPARE(removeSpy.count(), 5);
        QCOMPARE(removeSpy.last().at(1).toInt(), DownloadModel::Limit);
        QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Name)).toString(),
                 QStringLiteral("f%1.bin").arg(DownloadModel::Limit + 5));
        QCOMPARE(
            role(model, DownloadModel::Limit - 1, roleId(DownloadModel::Role::Name)).toString(),
            QStringLiteral("f6.bin"));
        QCOMPARE(rowsInDatabase(storage), DownloadModel::Limit);
        QCOMPARE(storedStatus(storage, 5), -1);
        QCOMPARE(storedStatus(storage, 6), static_cast<int>(DownloadModel::Running));

        model.observe(Topic, message(QStringLiteral("dl-done"), 1));
        QCOMPARE(model.count(), DownloadModel::Limit);

        QSqlQuery query(storage.database());
        for (int i = 0; i < 3; ++i) {
            query.prepare(QStringLiteral("INSERT INTO download (id, name, status, started) "
                                         "VALUES (?, 'old', 1, ?)"));
            query.addBindValue(1000 + i);
            query.addBindValue(i);
            QVERIFY(query.exec());
        }
        QCOMPARE(rowsInDatabase(storage), DownloadModel::Limit + 3);
    }

    Storage storage(dir.path());
    DownloadModel model(storage, dir.path());
    QCOMPARE(model.count(), DownloadModel::Limit);
    QCOMPARE(rowsInDatabase(storage), DownloadModel::Limit);
    QCOMPARE(storedStatus(storage, 1000), -1);
    QCOMPARE(role(model, DownloadModel::Limit - 1, roleId(DownloadModel::Role::Name)).toString(),
             QStringLiteral("f6.bin"));
}

void tst_downloadmodel::remove()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    DownloadModel model(storage, dir.path());
    model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
    model.observe(Topic, startMessage(2, QStringLiteral("b.pdf")));
    QSignalSpy countSpy(&model, &DownloadModel::countChanged);

    model.remove(-1);
    model.remove(2);
    QCOMPARE(model.count(), 2);
    QCOMPARE(countSpy.count(), 0);

    model.remove(1);
    QCOMPARE(model.count(), 1);
    QCOMPARE(countSpy.count(), 1);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Name)).toString(), QStringLiteral("b.pdf"));
    QCOMPARE(rowsInDatabase(storage), 1);
    QCOMPARE(storedStatus(storage, 1), -1);

    QSignalSpy changeSpy(&model, &DownloadModel::dataChanged);
    model.observe(Topic, message(QStringLiteral("dl-fail"), 1));
    QCOMPARE(changeSpy.count(), 0);
    model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
    QCOMPARE(model.count(), 2);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::DownloadId)).toInt(), 3);
}

void tst_downloadmodel::clear()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    DownloadModel model(storage, dir.path());
    QSignalSpy countSpy(&model, &DownloadModel::countChanged);
    model.clear();
    QCOMPARE(countSpy.count(), 0);

    model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
    model.observe(Topic, startMessage(2, QStringLiteral("b.pdf")));
    QCOMPARE(countSpy.count(), 2);
    model.clear();
    QCOMPARE(model.count(), 0);
    QCOMPARE(countSpy.count(), 3);
    QCOMPARE(rowsInDatabase(storage), 0);

    DownloadModel reloaded(storage, dir.path());
    QCOMPARE(reloaded.count(), 0);
}

void tst_downloadmodel::clearSince()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    {
        QSqlQuery insert(storage.database());
        QVERIFY(insert.exec(QStringLiteral("INSERT INTO download (id, name, status, started) "
                                           "VALUES (1, 'old.pdf', 1, 5)")));
    }
    DownloadModel model(storage, dir.path());
    const qint64 before = QDateTime::currentMSecsSinceEpoch();
    model.observe(Topic, startMessage(1, QStringLiteral("done.pdf")));
    model.observe(Topic, message(QStringLiteral("dl-done"), 1));
    model.observe(Topic, startMessage(2, QStringLiteral("coming.pdf")));
    QCOMPARE(model.count(), 3);
    QSignalSpy countSpy(&model, &DownloadModel::countChanged);
    QCOMPARE(model.countSince(double(before)), 1);
    QCOMPARE(model.countSince(0), 2);

    model.clearSince(double(before));
    QCOMPARE(model.count(), 2);
    QCOMPARE(countSpy.count(), 1);
    QCOMPARE(model.data(model.index(0, 0), roleId(DownloadModel::Role::Name)).toString(),
             QStringLiteral("coming.pdf"));
    QCOMPARE(model.data(model.index(1, 0), roleId(DownloadModel::Role::Name)).toString(),
             QStringLiteral("old.pdf"));
    QCOMPARE(rowsInDatabase(storage), 2);

    model.clearSince(double(before));
    QCOMPARE(countSpy.count(), 1);
    model.clearSince(0);
    QCOMPARE(model.count(), 1);
    QCOMPARE(model.data(model.index(0, 0), roleId(DownloadModel::Role::Status)).toInt(),
             int(DownloadModel::Running));
}

void tst_downloadmodel::fileUrl()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    DownloadModel model(storage, dir.path());
    model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
    QCOMPARE(model.fileUrl(0), QStringLiteral("file:///home/defaultuser/Downloads/a.pdf"));

    // URL-special names survive url round trip (open does that).
    const QStringList awkward{QStringLiteral("two words.pdf"), QStringLiteral("a#b.pdf"),
                              QStringLiteral("100%.pdf"), QStringLiteral("q?x=1.pdf")};
    for (int i = 0; i < awkward.count(); ++i) {
        model.observe(Topic, startMessage(i + 2, awkward.at(i)));
        QCOMPARE(QUrl(model.fileUrl(0)).toLocalFile(), Downloads + awkward.at(i));
    }

    QVariantMap nowhere = startMessage(9, QStringLiteral("x"));
    nowhere.remove(QStringLiteral("targetPath"));
    model.observe(Topic, nowhere);
    QVERIFY(model.fileUrl(0).isEmpty());
    QVERIFY(model.fileUrl(model.count()).isEmpty());
    QVERIFY(model.fileUrl(-1).isEmpty());
}

void tst_downloadmodel::rowOf()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    DownloadModel model(storage, dir.path());
    QVERIFY(model.downloads().isEmpty());
    QCOMPARE(model.rowOf(1), -1);

    model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
    const int a = role(model, 0, roleId(DownloadModel::Role::DownloadId)).toInt();
    model.observe(Topic, startMessage(2, QStringLiteral("b.pdf")));
    const int b = role(model, 0, roleId(DownloadModel::Role::DownloadId)).toInt();
    QCOMPARE(model.rowOf(b), 0);
    QCOMPARE(model.rowOf(a), 1);
    QCOMPARE(model.rowOf(0), -1);
    QCOMPARE(model.rowOf(b + 1), -1);

    QCOMPARE(model.downloads().count(), 2);
    QCOMPARE(model.downloads().at(0).id, b);
    QCOMPARE(model.downloads().at(0).name, QStringLiteral("b.pdf"));
    QCOMPARE(model.downloads().at(0).url, QStringLiteral("https://files.example/b.pdf"));
    QCOMPARE(model.downloads().at(0).status, DownloadModel::Running);
    QVERIFY(model.downloads().at(0).started > 0);

    model.observe(Topic, startMessage(3, QStringLiteral("c.pdf")));
    QCOMPARE(model.rowOf(a), 2);
    model.remove(model.rowOf(b));
    QCOMPARE(model.rowOf(b), -1);
    QCOMPARE(model.rowOf(a), 1);
    QCOMPARE(model.fileUrl(model.rowOf(a)),
             QUrl::fromLocalFile(Downloads + QStringLiteral("a.pdf")).toString());
    model.clear();
    QCOMPARE(model.rowOf(a), -1);
}

// Engine saves only into existing folder: model creates it and parents before telling engine.
void tst_downloadmodel::directory()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    const QString folder = dir.filePath(QStringLiteral("Downloads/Salama"));
    QVERIFY(!QFileInfo::exists(dir.filePath(QStringLiteral("Downloads"))));
    DownloadModel model(storage, folder);
    QCOMPARE(model.directory(), folder);
    QVERIFY(QFileInfo(folder).isDir());

    // Uncreatable (file in the way): still told to engine, which falls back to ~/Downloads.
    QFile file(dir.filePath(QStringLiteral("file")));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();
    const QString blocked = file.fileName() + QStringLiteral("/Salama");
    DownloadModel unmade(storage, blocked);
    QCOMPARE(unmade.directory(), blocked);
    QVERIFY(!QFileInfo::exists(blocked));
}

void tst_downloadmodel::withoutDatabase()
{
    QTemporaryDir dir;
    Storage storage{QString()};
    QVERIFY(!storage.isOpen());
    DownloadModel model(storage, dir.path());
    QCOMPARE(model.count(), 0);
    model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
    model.observe(Topic, message(QStringLiteral("dl-fail"), 1));
    QCOMPARE(model.count(), 1);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Status)).toInt(),
             static_cast<int>(DownloadModel::Failed));
    model.remove(0);
    QCOMPARE(model.count(), 0);
}

namespace {

const QString RequestTopic = QStringLiteral("embedui:download");

QVariantMap lastRequest(const QSignalSpy &spy)
{
    return spy.last().at(1).toMap();
}

QVariantMap engineRequest(const QString &msg, int id)
{
    return {{QStringLiteral("msg"), msg}, {QStringLiteral("id"), id}};
}

void writeFile(const QString &path)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("data");
}

void arrive(DownloadModel &model, int engineId, const QString &path)
{
    QVariantMap start = startMessage(engineId, QFileInfo(path).fileName());
    start.insert(QStringLiteral("targetPath"), path);
    model.observe(Topic, start);
    QVariantMap done = message(QStringLiteral("dl-done"), engineId);
    done.insert(QStringLiteral("targetPath"), path);
    model.observe(Topic, done);
}

} // namespace

// Engine pauses by cancel, resumes by restart; model asks by engine id, shows what engine says.
void tst_downloadmodel::pauseAndResume()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    DownloadModel model(storage, dir.path());
    QSignalSpy requests(&model, &DownloadModel::engineRequest);
    model.observe(Topic, startMessage(4, QStringLiteral("a.pdf")));
    model.observe(Topic, startMessage(9, QStringLiteral("b.pdf")));
    QVERIFY(!role(model, 0, roleId(DownloadModel::Role::Resumable)).toBool());

    model.pause(-1);
    model.pause(2);
    model.resume(-1);
    model.resume(2);
    model.resume(0);
    QCOMPARE(requests.count(), 0);

    model.pause(1);
    QCOMPARE(requests.count(), 1);
    QCOMPARE(requests.last().at(0).toString(), RequestTopic);
    QCOMPARE(lastRequest(requests), engineRequest(QStringLiteral("cancelDownload"), 4));
    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::Status)).toInt(),
             static_cast<int>(DownloadModel::Running));
    model.observe(Topic, progressMessage(4, 30.0));
    model.observe(Topic, message(QStringLiteral("dl-cancel"), 4));
    QVERIFY(role(model, 1, roleId(DownloadModel::Role::Resumable)).toBool());
    model.pause(1);
    QCOMPARE(requests.count(), 1);

    model.resume(1);
    QCOMPARE(requests.count(), 2);
    QCOMPARE(lastRequest(requests), engineRequest(QStringLiteral("retryDownload"), 4));
    model.observe(Topic, startMessage(4, QStringLiteral("a.pdf")));
    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::Status)).toInt(),
             static_cast<int>(DownloadModel::Running));
    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::Progress)).toInt(), 30);
    QCOMPARE(model.count(), 2);

    model.observe(Topic, message(QStringLiteral("dl-fail"), 9));
    QVERIFY(role(model, 0, roleId(DownloadModel::Role::Resumable)).toBool());
    model.resume(0);
    QCOMPARE(lastRequest(requests), engineRequest(QStringLiteral("retryDownload"), 9));

    model.observe(Topic, message(QStringLiteral("dl-done"), 4));
    QVERIFY(!role(model, 1, roleId(DownloadModel::Role::Resumable)).toBool());
    model.pause(1);
    model.resume(1);
    QCOMPARE(requests.count(), 3);
}

// Save link / Save image: url's own name in downloads folder; never invented, never
// overwrites existing file.
void tst_downloadmodel::saveKeepsTheFilesName()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    const QString folder = dir.filePath(QStringLiteral("Downloads/Salama"));
    DownloadModel model(storage, folder);
    QSignalSpy requests(&model, &DownloadModel::engineRequest);
    const auto in = [&folder](const QString &name) { return QDir(folder).filePath(name); };

    const QString map = QStringLiteral("https://files.example/maps/ridge%20loop.pdf?v=2");
    QCOMPARE(model.save(map, QString()), in(QStringLiteral("ridge loop.pdf")));
    QCOMPARE(requests.count(), 1);
    QCOMPARE(requests.last().at(0).toString(), RequestTopic);
    QCOMPARE(lastRequest(requests),
             QVariantMap({{QStringLiteral("msg"), QStringLiteral("addDownload")},
                          {QStringLiteral("from"), map},
                          {QStringLiteral("to"), in(QStringLiteral("ridge loop.pdf"))}}));
    QCOMPARE(model.save(map, QString()), in(QStringLiteral("ridge loop(1).pdf")));
    QCOMPARE(model.save(map, QString()), in(QStringLiteral("ridge loop(2).pdf")));
    QFile photo(in(QStringLiteral("photo.jpg")));
    QVERIFY(photo.open(QIODevice::WriteOnly));
    photo.close();
    QCOMPARE(
        model.save(QStringLiteral("https://cdn.example/photo.jpg"), QStringLiteral("image/jpeg")),
        in(QStringLiteral("photo(1).jpg")));
    QVariantMap report = startMessage(1, QStringLiteral("report.pdf"));
    report.insert(QStringLiteral("targetPath"), in(QStringLiteral("report.pdf")));
    model.observe(Topic, report);
    QCOMPARE(model.save(QStringLiteral("https://files.example/report.pdf"), QString()),
             in(QStringLiteral("report(1).pdf")));

    QCOMPARE(
        model.save(QStringLiteral("https://cdn.example/image/12345"), QStringLiteral("image/png")),
        in(QStringLiteral("12345.png")));
    QCOMPARE(model.save(QStringLiteral("https://cdn.example/blob"),
                        QStringLiteral("application/x-nothing-known")),
             in(QStringLiteral("blob")));
    QCOMPARE(model.save(QStringLiteral("https://www.trails.example/"), QString()),
             in(QStringLiteral("www.trails.example")));
    QCOMPARE(model.save(QStringLiteral("https:///"), QString()), in(QStringLiteral("download")));
    QCOMPARE(model.save(QStringLiteral("https://x.example/a%5Cb.txt"), QString()),
             in(QStringLiteral("a_b.txt")));
    QCOMPARE(model.save(QStringLiteral("https://x.example/.profile"), QString()),
             in(QStringLiteral("profile")));
    const QString longName = QString(300, QLatin1Char('a')) + QStringLiteral(".pdf");
    const QString cut = model.save(QStringLiteral("https://x.example/") + longName, QString());
    QVERIFY(QFileInfo(cut).fileName().toUtf8().size() <= 240);
    QVERIFY(QFileInfo(cut).fileName().startsWith(QStringLiteral("aaaa")));
    QVERIFY(cut.endsWith(QStringLiteral(".pdf")));
    const int asked = requests.count();

    QCOMPARE(model.save(QStringLiteral("ftp://x.example/a.txt"), QString()), QString());
    QCOMPARE(model.save(QStringLiteral("data:text/plain,hi"), QString()), QString());
    QCOMPARE(model.save(QString(), QString()), QString());
    QCOMPARE(requests.count(), asked);
}

void tst_downloadmodel::downloadAgain()
{
    QTemporaryDir dir;
    {
        Storage storage(dir.path());
        DownloadModel model(storage, dir.path());
        model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
        model.observe(Topic, startMessage(2, QStringLiteral("b.pdf")));
        model.observe(Topic, message(QStringLiteral("dl-cancel"), 2));
        QVariantMap nowhere = startMessage(3, QStringLiteral("c.pdf"));
        nowhere.remove(QStringLiteral("targetPath"));
        model.observe(Topic, nowhere);
        QVariantMap unsourced = startMessage(4, QStringLiteral("d.pdf"));
        unsourced.remove(QStringLiteral("sourceUrl"));
        model.observe(Topic, unsourced);
        arrive(model, 5, dir.filePath(QStringLiteral("e.pdf")));
    }
    Storage storage(dir.path());
    DownloadModel model(storage, dir.path());
    QSignalSpy requests(&model, &DownloadModel::engineRequest);
    QSignalSpy countSpy(&model, &DownloadModel::countChanged);
    QCOMPARE(model.count(), 5);
    for (int row = 0; row < model.count(); ++row) {
        QVERIFY(!role(model, row, roleId(DownloadModel::Role::Resumable)).toBool());
    }

    model.resume(0);
    model.resume(1);
    QCOMPARE(requests.count(), 0);
    QCOMPARE(model.count(), 5);

    QCOMPARE(role(model, 3, roleId(DownloadModel::Role::Name)).toString(), QStringLiteral("b.pdf"));
    model.resume(3);
    QCOMPARE(requests.count(), 1);
    QCOMPARE(requests.last().at(0).toString(), RequestTopic);
    QCOMPARE(lastRequest(requests),
             QVariantMap({{QStringLiteral("msg"), QStringLiteral("addDownload")},
                          {QStringLiteral("from"), QStringLiteral("https://files.example/b.pdf")},
                          {QStringLiteral("to"), Downloads + QStringLiteral("b.pdf")}}));
    QCOMPARE(model.count(), 4);
    QCOMPARE(countSpy.count(), 1);
    QCOMPARE(model.rowOf(2), -1);
    QCOMPARE(storedStatus(storage, 2), -1);

    QCOMPARE(role(model, 2, roleId(DownloadModel::Role::Name)).toString(), QStringLiteral("c.pdf"));
    model.resume(2);
    QCOMPARE(lastRequest(requests).value(QStringLiteral("to")).toString(),
             QDir(dir.path()).filePath(QStringLiteral("c.pdf")));
    QCOMPARE(model.count(), 3);

    model.observe(Topic, startMessage(1, QStringLiteral("b.pdf")));
    QCOMPARE(model.count(), 4);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Name)).toString(), QStringLiteral("b.pdf"));
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Status)).toInt(),
             static_cast<int>(DownloadModel::Running));
}

// Removing running download pauses it first: no orphan arrivals.
void tst_downloadmodel::removeAndClearPause()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    DownloadModel model(storage, dir.path());
    QSignalSpy requests(&model, &DownloadModel::engineRequest);
    model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
    model.observe(Topic, startMessage(2, QStringLiteral("b.pdf")));
    model.observe(Topic, message(QStringLiteral("dl-fail"), 2));

    model.remove(0);
    QCOMPARE(requests.count(), 0);
    model.remove(0);
    QCOMPARE(requests.count(), 1);
    QCOMPARE(lastRequest(requests), engineRequest(QStringLiteral("cancelDownload"), 1));
    QCOMPARE(model.count(), 0);

    model.observe(Topic, startMessage(3, QStringLiteral("c.pdf")));
    model.observe(Topic, startMessage(4, QStringLiteral("d.pdf")));
    model.observe(Topic, message(QStringLiteral("dl-done"), 4));
    model.observe(Topic, startMessage(5, QStringLiteral("e.pdf")));
    model.clear();
    QCOMPARE(requests.count(), 3);
    QCOMPARE(requests.at(1).at(1).toMap(), engineRequest(QStringLiteral("cancelDownload"), 5));
    QCOMPARE(requests.at(2).at(1).toMap(), engineRequest(QStringLiteral("cancelDownload"), 3));
}

void tst_downloadmodel::clearFinished()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    DownloadModel model(storage, dir.path());
    QSignalSpy countSpy(&model, &DownloadModel::countChanged);
    model.clearFinished();
    QCOMPARE(countSpy.count(), 0);

    model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
    model.observe(Topic, message(QStringLiteral("dl-done"), 1));
    model.observe(Topic, startMessage(2, QStringLiteral("b.pdf")));
    model.observe(Topic, message(QStringLiteral("dl-fail"), 2));
    model.observe(Topic, startMessage(3, QStringLiteral("c.pdf")));
    model.observe(Topic, message(QStringLiteral("dl-done"), 3));
    model.observe(Topic, startMessage(4, QStringLiteral("d.pdf")));
    QCOMPARE(countSpy.count(), 4);
    QSignalSpy runningSpy(&model, &DownloadModel::runningChanged);
    QSignalSpy traySpy(&model, &DownloadModel::trayChanged);

    model.clearFinished();
    QCOMPARE(model.count(), 2);
    QCOMPARE(countSpy.count(), 5);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Name)).toString(), QStringLiteral("d.pdf"));
    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::Name)).toString(), QStringLiteral("b.pdf"));
    QCOMPARE(rowsInDatabase(storage), 2);
    QCOMPARE(storedStatus(storage, 1), -1);
    QCOMPARE(storedStatus(storage, 3), -1);
    QCOMPARE(runningSpy.count(), 0);
    QCOMPARE(model.trayCount(), 2);
    QCOMPARE(model.trayNames(), QStringList({QStringLiteral("d.pdf"), QStringLiteral("b.pdf")}));
    QCOMPARE(traySpy.count(), 0);

    model.clearFinished();
    QCOMPARE(countSpy.count(), 5);
}

void tst_downloadmodel::deleteFile()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    const QString folder = dir.filePath(QStringLiteral("Downloads/Salama"));
    DownloadModel model(storage, folder);
    const QString kept = folder + QStringLiteral("/kept.pdf");
    const QString fallback = dir.filePath(QStringLiteral("Downloads/fallback.pdf"));
    const QString gone = folder + QStringLiteral("/gone.pdf");
    writeFile(kept);
    writeFile(fallback);
    arrive(model, 1, gone);
    arrive(model, 2, fallback);
    arrive(model, 3, kept);
    model.observe(Topic, startMessage(4, QStringLiteral("coming.pdf")));
    QSignalSpy countSpy(&model, &DownloadModel::countChanged);

    QVERIFY(!model.deleteFile(-1));
    QVERIFY(!model.deleteFile(model.count()));
    QVERIFY(!model.deleteFile(0));
    QCOMPARE(model.count(), 4);

    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::Name)).toString(),
             QStringLiteral("kept.pdf"));
    QVERIFY(model.deleteFile(1));
    QVERIFY(!QFileInfo::exists(kept));
    QCOMPARE(model.count(), 3);
    QCOMPARE(countSpy.count(), 1);
    QCOMPARE(storedStatus(storage, 3), -1);

    QVERIFY(model.deleteFile(1));
    QVERIFY(!QFileInfo::exists(fallback));

    QVERIFY(model.deleteFile(1));
    QCOMPARE(model.count(), 1);
}

void tst_downloadmodel::deleteFileStaysInDownloads()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    const QString folder = dir.filePath(QStringLiteral("Downloads/Salama"));
    DownloadModel model(storage, folder);
    const QString outside = dir.filePath(QStringLiteral("secret.txt"));
    writeFile(outside);
    const QString climbing = folder + QStringLiteral("/../../secret.txt");
    const QString link = folder + QStringLiteral("/link.txt");
    QVERIFY(QFile::link(outside, link));
    const QString lookalike = dir.filePath(QStringLiteral("DownloadsElsewhere/x.txt"));
    writeFile(lookalike);
    const QString inner = folder + QStringLiteral("/inner");
    QVERIFY(QDir().mkpath(inner));
    arrive(model, 1, outside);
    arrive(model, 2, climbing);
    arrive(model, 3, link);
    arrive(model, 4, lookalike);
    arrive(model, 5, inner);

    for (int row = 0; row < 5; ++row) {
        QVERIFY2(!model.deleteFile(row), qPrintable(QString::number(row)));
    }
    QCOMPARE(model.count(), 5);
    QVERIFY(QFileInfo::exists(outside));
    QVERIFY(QFileInfo(link).isSymLink());
    QVERIFY(QFileInfo::exists(lookalike));
    QVERIFY(QFileInfo(inner).isDir());

    DownloadModel rootless(storage, QString());
    arrive(rootless, 6, outside);
    QVERIFY(!rootless.deleteFile(0));
    QVERIFY(QFileInfo::exists(outside));
}

void tst_downloadmodel::fileExists()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    DownloadModel model(storage, dir.path());
    const QString path = dir.filePath(QStringLiteral("a.pdf"));
    QVariantMap start = startMessage(1, QStringLiteral("a.pdf"));
    start.insert(QStringLiteral("targetPath"), path);
    model.observe(Topic, start);
    writeFile(path);
    QVERIFY(!role(model, 0, roleId(DownloadModel::Role::FileExists)).toBool());
    model.observe(Topic, message(QStringLiteral("dl-done"), 1));
    QVERIFY(role(model, 0, roleId(DownloadModel::Role::FileExists)).toBool());

    QSignalSpy changeSpy(&model, &DownloadModel::dataChanged);
    QVERIFY(QFile::remove(path));
    model.refreshFiles();
    QCOMPARE(changeSpy.count(), 1);
    QCOMPARE(changeSpy.last().at(0).value<QModelIndex>().row(), 0);
    QCOMPARE(changeSpy.last().at(1).value<QModelIndex>().row(), 0);
    QCOMPARE(changedRoles(changeSpy), QVector<int>{roleId(DownloadModel::Role::FileExists)});
    QVERIFY(!role(model, 0, roleId(DownloadModel::Role::FileExists)).toBool());

    QVariantMap nowhere = startMessage(2, QStringLiteral("x"));
    nowhere.remove(QStringLiteral("targetPath"));
    model.observe(Topic, nowhere);
    model.observe(Topic, message(QStringLiteral("dl-done"), 2));
    QVERIFY(!role(model, 0, roleId(DownloadModel::Role::FileExists)).toBool());

    DownloadModel empty(storage, dir.path());
    empty.clear();
    QSignalSpy emptySpy(&empty, &DownloadModel::dataChanged);
    empty.refreshFiles();
    QCOMPARE(emptySpy.count(), 0);
}

void tst_downloadmodel::formatSize_data()
{
    QTest::addColumn<double>("bytes");
    QTest::addColumn<QString>("text");
    QTest::newRow("nothing") << 0.0 << QStringLiteral("0 B");
    QTest::newRow("bytes") << 512.0 << QStringLiteral("512 B");
    QTest::newRow("almost a kB") << 1023.0 << QStringLiteral("1023 B");
    QTest::newRow("a kB") << 1024.0 << QStringLiteral("1.0 kB");
    QTest::newRow("fraction") << 1536.0 << QStringLiteral("1.5 kB");
    QTest::newRow("tens") << 12.0 * 1024 * 1024 << QStringLiteral("12 MB");
    QTest::newRow("megabytes") << 7.4 * 1024 * 1024 << QStringLiteral("7.4 MB");
    QTest::newRow("gigabytes") << 3.0 * 1024 * 1024 * 1024 << QStringLiteral("3.0 GB");
    QTest::newRow("past the last unit")
        << 2048.0 * 1024 * 1024 * 1024 * 1024 << QStringLiteral("2048 TB");
    QTest::newRow("negative") << -5.0 << QStringLiteral("0 B");
    QTest::newRow("not a number") << qQNaN() << QStringLiteral("0 B");
    QTest::newRow("infinite") << qInf() << QStringLiteral("0 B");
}

void tst_downloadmodel::formatSize()
{
    QFETCH(double, bytes);
    QFETCH(QString, text);
    QCOMPARE(DownloadModel::formatSize(bytes), text);
}

void tst_downloadmodel::tray()
{
    QTemporaryDir dir;
    {
        Storage storage(dir.path());
        DownloadModel model(storage, dir.path());
        model.observe(Topic, startMessage(1, QStringLiteral("old.pdf")));
        model.observe(Topic, message(QStringLiteral("dl-fail"), 1));
        QCOMPARE(model.trayCount(), 1);
    }
    Storage storage(dir.path());
    DownloadModel model(storage, dir.path());
    QSignalSpy traySpy(&model, &DownloadModel::trayChanged);
    QCOMPARE(model.trayCount(), 0);
    QCOMPARE(model.trayNames(), QStringList());

    model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
    model.observe(Topic, progressMessage(1, 40.0));
    QCOMPARE(model.trayCount(), 1);
    QCOMPARE(model.trayStatus(), static_cast<int>(DownloadModel::Running));
    QCOMPARE(model.traySize(), 2048.0);
    QCOMPARE(model.trayProgress(), 40);
    QCOMPARE(model.trayNames(), QStringList{QStringLiteral("a.pdf")});
    QCOMPARE(traySpy.count(), 2);

    QVariantMap unsized = startMessage(2, QStringLiteral("b.iso"));
    unsized.insert(QStringLiteral("size"), 0.0);
    model.observe(Topic, unsized);
    model.observe(Topic, progressMessage(2, 20.0));
    model.observe(Topic, startMessage(3, QStringLiteral("c.zip")));
    model.observe(Topic, progressMessage(3, 90.0));
    QCOMPARE(model.trayCount(), 3);
    QCOMPARE(model.trayProgress(), 50);
    QCOMPARE(model.trayNames(), QStringList({QStringLiteral("c.zip"), QStringLiteral("b.iso"),
                                             QStringLiteral("a.pdf")}));
    model.observe(Topic, message(QStringLiteral("dl-fail"), 3));
    QCOMPARE(model.trayFailed(), 1);
    QCOMPARE(model.trayProgress(), 30);
    QCOMPARE(model.trayStatus(), static_cast<int>(DownloadModel::Failed));
    QCOMPARE(model.traySize(), 2048.0);
    model.remove(0);
    QCOMPARE(model.trayNames().value(0), QStringLiteral("b.iso"));
    QCOMPARE(model.trayStatus(), static_cast<int>(DownloadModel::Running));
    QCOMPARE(model.traySize(), 0.0);
    model.observe(Topic, startMessage(3, QStringLiteral("c.zip")));
    model.observe(Topic, progressMessage(3, 90.0));
    model.observe(Topic, message(QStringLiteral("dl-fail"), 3));
    model.observe(Topic, message(QStringLiteral("dl-cancel"), 1));
    QCOMPARE(model.trayPaused(), 1);
    QCOMPARE(model.trayFailed(), 1);
    QCOMPARE(model.trayProgress(), 30);
    QCOMPARE(model.trayCount(), 3);

    model.observe(Topic, message(QStringLiteral("dl-done"), 1));
    QCOMPARE(model.trayCount(), 2);
    QCOMPARE(model.trayPaused(), 0);
    const int said = traySpy.count();
    model.observe(Topic, progressMessage(2, 20.0));
    QCOMPARE(traySpy.count(), said);

    model.observe(Topic, startMessage(4, QStringLiteral("d.pdf")));
    model.observe(Topic, message(QStringLiteral("dl-done"), 4));
    const int arrived = traySpy.count();
    model.remove(0);
    QCOMPARE(traySpy.count(), arrived);
    QCOMPARE(model.trayNames(), QStringList({QStringLiteral("c.zip"), QStringLiteral("b.iso")}));
    model.remove(0);
    QCOMPARE(model.trayCount(), 1);
    QCOMPARE(model.trayNames(), QStringList{QStringLiteral("b.iso")});
    model.observe(Topic, message(QStringLiteral("dl-fail"), 2));
    model.clearSince(0);
    QCOMPARE(model.trayCount(), 0);
    QCOMPARE(model.trayNames(), QStringList());
    QCOMPARE(model.trayProgress(), 0);
}

void tst_downloadmodel::trayDismissed()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    DownloadModel model(storage, dir.path());
    QSignalSpy traySpy(&model, &DownloadModel::trayChanged);
    model.dismissTray();
    QCOMPARE(traySpy.count(), 0);

    model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
    model.observe(Topic, startMessage(2, QStringLiteral("b.pdf")));
    model.observe(Topic, message(QStringLiteral("dl-fail"), 2));
    model.observe(Topic, startMessage(3, QStringLiteral("c.pdf")));
    model.observe(Topic, message(QStringLiteral("dl-done"), 3));
    QCOMPARE(model.trayCount(), 2);
    model.dismissTray();
    QCOMPARE(model.trayCount(), 0);
    QCOMPARE(model.trayNames(), QStringList());

    model.observe(Topic, progressMessage(1, 50.0));
    QCOMPARE(model.trayCount(), 0);
    model.observe(Topic, startMessage(4, QStringLiteral("d.pdf")));
    QCOMPARE(model.trayCount(), 1);
    QCOMPARE(model.trayNames(), QStringList{QStringLiteral("d.pdf")});
    model.observe(Topic, message(QStringLiteral("dl-cancel"), 1));
    QCOMPARE(model.trayCount(), 2);
    QCOMPARE(model.trayPaused(), 1);
    model.observe(Topic, startMessage(2, QStringLiteral("b.pdf")));
    QCOMPARE(model.trayCount(), 3);
    model.dismissTray();
    model.observe(Topic, message(QStringLiteral("dl-done"), 4));
    QCOMPARE(model.trayCount(), 0);
}

void tst_downloadmodel::finished()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    DownloadModel model(storage, dir.path());
    QSignalSpy finishedSpy(&model, &DownloadModel::finished);
    int trayAtSignal = -1;
    connect(&model, &DownloadModel::finished, this,
            [&model, &trayAtSignal]() { trayAtSignal = model.trayCount(); });
    model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
    model.observe(Topic, startMessage(2, QStringLiteral("b.pdf")));
    model.observe(Topic, message(QStringLiteral("dl-fail"), 2));
    model.observe(Topic, message(QStringLiteral("dl-cancel"), 1));
    QCOMPARE(finishedSpy.count(), 0);

    model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
    model.observe(Topic, message(QStringLiteral("dl-done"), 1));
    QCOMPARE(finishedSpy.count(), 1);
    QCOMPARE(finishedSpy.last().at(0).toInt(), model.downloads().at(1).id);
    QCOMPARE(finishedSpy.last().at(1).toString(), QStringLiteral("a.pdf"));
    QCOMPARE(trayAtSignal, 2);
    QVariantMap moved = message(QStringLiteral("dl-done"), 1);
    moved.insert(QStringLiteral("targetPath"), Downloads + QStringLiteral("a(1).pdf"));
    model.observe(Topic, moved);
    model.observe(Topic, message(QStringLiteral("dl-done"), 1));
    QCOMPARE(finishedSpy.count(), 1);
}

QTEST_GUILESS_MAIN(tst_downloadmodel)
#include "tst_downloadmodel.moc"
