// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "downloads/DownloadModel.h"
#include "storage/Storage.h"

#include <QDateTime>
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
    void startedAndEnded();
    void stop();
    void retry();
    void refetch();
    void retryable();
    void deleteFile();
    void fileExists();
    void icons();
    void details();
    void withoutDatabase();
};

namespace {

const QString Topic = QStringLiteral("embed:download");
const QString Downloads = QStringLiteral("/home/defaultuser/Downloads/");

// What EmbedliteDownloadManager.js sends as a download starts, as qtmozembed hands it
// over once the device's Qt 5.6 has read the JSON: every number a double.
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

// A change of status, and what is worked out of it.
const QVector<int> StatusRoles{roleId(DownloadModel::Role::Status),
                               roleId(DownloadModel::Role::Retryable),
                               roleId(DownloadModel::Role::FileExists)};

QVector<int> changedRoles(const QSignalSpy &spy)
{
    return spy.last().at(2).value<QVector<int>>();
}

// What the model asked the engine for last: its topic, and the message as a map.
QString requestTopic(const QSignalSpy &spy)
{
    return spy.last().at(0).toString();
}

QVariantMap request(const QSignalSpy &spy)
{
    return spy.last().at(1).toMap();
}

// A file of a few bytes, made where a download would have put it.
QString makeFile(const QTemporaryDir &dir, const QString &name)
{
    QFile file(dir.filePath(name));
    if (!file.open(QIODevice::WriteOnly)) {
        return {};
    }
    file.write("data");
    return file.fileName();
}

// A start for a file in a folder of the test's own, which can be made to exist.
QVariantMap startIn(const QTemporaryDir &dir, int id, const QString &file)
{
    QVariantMap start = startMessage(id, file);
    start.insert(QStringLiteral("targetPath"), dir.filePath(file));
    return start;
}

} // namespace

void tst_downloadmodel::topicRolesAndStatuses()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    DownloadModel model(storage, dir.path());
    // The topic sailfish-browser's DownloadManager listens on for the same messages.
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
    QCOMPARE(roles.value(roleId(DownloadModel::Role::Retryable)), QByteArray("retryable"));
    QCOMPARE(roles.value(roleId(DownloadModel::Role::FileExists)), QByteArray("fileExists"));
    QCOMPARE(roles.value(roleId(DownloadModel::Role::Icon)), QByteArray("icon"));
    QCOMPARE(roles.count(), 12);

    // QML compares a row's status with these by name, and the database keeps them as
    // numbers: neither may move.
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

    // The next one goes above it, with an id of its own.
    model.observe(Topic, startMessage(2, QStringLiteral("b.zip")));
    QCOMPARE(model.count(), 2);
    QCOMPARE(countSpy.count(), 2);
    QCOMPARE(insertSpy.last().at(1).toInt(), 0);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Name)).toString(), QStringLiteral("b.zip"));
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::DownloadId)).toInt(), 2);
    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::Name)).toString(), QStringLiteral("a.pdf"));

    // A size the engine does not know yet is 0; one that makes no sense is too.
    QVariantMap unsized = startMessage(3, QStringLiteral("c.bin"));
    unsized.insert(QStringLiteral("size"), 0.0);
    model.observe(Topic, unsized);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Size)).toLongLong(), 0LL);
    QVariantMap negative = startMessage(4, QStringLiteral("d.bin"));
    negative.insert(QStringLiteral("size"), -5.0);
    model.observe(Topic, negative);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Size)).toLongLong(), 0LL);
    // Nor is a size that is not a number, though QVariant would read one out of it.
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

    // Engine id 1 is the older download, in the second row.
    model.observe(Topic, progressMessage(1, 42.0));
    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::Progress)).toInt(), 42);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Progress)).toInt(), 0);
    QCOMPARE(changeSpy.count(), 1);
    QCOMPARE(changeSpy.last().at(0).value<QModelIndex>().row(), 1);
    QCOMPARE(changedRoles(changeSpy), QVector<int>{roleId(DownloadModel::Role::Progress)});

    // The same figure again changes nothing.
    model.observe(Topic, progressMessage(1, 42.0));
    QCOMPARE(changeSpy.count(), 1);

    // Held to a percentage whatever the message says.
    model.observe(Topic, progressMessage(1, 250.0));
    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::Progress)).toInt(), 100);
    model.observe(Topic, progressMessage(1, -3.0));
    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::Progress)).toInt(), 0);
    model.observe(Topic, progressMessage(1, 1e30));
    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::Progress)).toInt(), 100);
    model.observe(Topic, progressMessage(1, 66.6));
    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::Progress)).toInt(), 67);
    QCOMPARE(changeSpy.count(), 5);

    // A message with no figure in it says nothing; nor does one whose figure is not a
    // number, though QVariant would read 42 out of the one and 1 out of the other.
    model.observe(Topic, progressMessage(1, QStringLiteral("most of it")));
    model.observe(Topic, progressMessage(1, QVariant()));
    model.observe(Topic, message(QStringLiteral("dl-progress"), 1));
    model.observe(Topic, progressMessage(1, QStringLiteral("42")));
    model.observe(Topic, progressMessage(1, true));
    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::Progress)).toInt(), 67);
    QCOMPARE(changeSpy.count(), 5);
}

// How many downloads are still coming and how far along they are together: what the
// menu's ring says. The mean of their percentages, each counted alike whatever its size.
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
    // A second, of no known size, counts as much as the first: half of 40 and 0.
    QVariantMap unsized = startMessage(2, QStringLiteral("b.iso"));
    unsized.insert(QStringLiteral("size"), 0.0);
    model.observe(Topic, unsized);
    QCOMPARE(model.runningCount(), 2);
    QCOMPARE(model.runningProgress(), 20);
    model.observe(Topic, progressMessage(2, 45.0));
    QCOMPARE(model.runningProgress(), 43);
    const int said = runningSpy.count();
    // The same figure again says nothing, and nor does a change that leaves both as
    // they were.
    model.observe(Topic, progressMessage(2, 45.0));
    QCOMPARE(runningSpy.count(), said);

    // Done, failed or cancelled, a download is no longer coming, and the rest are what
    // is left of the mean.
    model.observe(Topic, message(QStringLiteral("dl-done"), 1));
    QCOMPARE(model.runningCount(), 1);
    QCOMPARE(model.runningProgress(), 45);
    model.observe(Topic, message(QStringLiteral("dl-fail"), 2));
    QCOMPARE(model.runningCount(), 0);
    QCOMPARE(model.runningProgress(), 0);
    // Started again, it is coming again, from the start.
    model.observe(Topic, startMessage(2, QStringLiteral("b.iso")));
    QCOMPARE(model.runningCount(), 1);
    QCOMPARE(model.runningProgress(), 0);
    model.observe(Topic, message(QStringLiteral("dl-cancel"), 2));
    QCOMPARE(model.runningCount(), 0);

    // Forgotten while it is coming, it is not counted. Clearing what has ended leaves
    // the downloads still coming, and they still count; clearing everything does not.
    model.observe(Topic, startMessage(3, QStringLiteral("c.pdf")));
    model.observe(Topic, progressMessage(3, 10.0));
    QCOMPARE(model.runningCount(), 1);
    model.remove(0);
    QCOMPARE(model.runningCount(), 0);
    QCOMPARE(model.runningProgress(), 0);
    model.observe(Topic, startMessage(4, QStringLiteral("d.pdf")));
    model.observe(Topic, progressMessage(4, 70.0));
    QCOMPARE(model.runningProgress(), 70);
    model.clearEnded();
    QCOMPARE(model.runningCount(), 1);
    QCOMPARE(model.runningProgress(), 70);
    model.clear();
    QCOMPARE(model.runningCount(), 0);
    QCOMPARE(model.runningProgress(), 0);

    // Read back after a restart, nothing is coming: the engine forgot them all.
    model.observe(Topic, startMessage(5, QStringLiteral("e.pdf")));
    QCOMPARE(model.runningCount(), 1);
    DownloadModel reloaded(storage, dir.path());
    QCOMPARE(reloaded.runningCount(), 0);
    QCOMPARE(reloaded.runningProgress(), 0);
}

// The same messages with their whole numbers as other readers give them: a qlonglong,
// as Qt reads the engine's JSON from 5.15 on, and an int, as QML hands one over.
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

    // Done, and where the file ended up.
    QVariantMap done = message(QStringLiteral("dl-done"), 1);
    done.insert(QStringLiteral("targetPath"), Downloads + QStringLiteral("a(1).pdf"));
    model.observe(Topic, done);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Status)).toInt(),
             static_cast<int>(DownloadModel::Done));
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Progress)).toInt(), 100);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Path)).toString(),
             Downloads + QStringLiteral("a(1).pdf"));
    QCOMPARE(changeSpy.count(), 1);
    // And what is worked out of those: whether it can be fetched again, and whether
    // its file is there.
    QCOMPARE(
        changedRoles(changeSpy),
        QVector<int>({roleId(DownloadModel::Role::Status), roleId(DownloadModel::Role::Progress),
                      roleId(DownloadModel::Role::Path), roleId(DownloadModel::Role::Retryable),
                      roleId(DownloadModel::Role::FileExists)}));
    QCOMPARE(storedStatus(storage, 1), static_cast<int>(DownloadModel::Done));

    // Said twice, it changes nothing the second time.
    model.observe(Topic, done);
    QCOMPARE(changeSpy.count(), 1);

    // Without a path, the one the download started with stands.
    model.observe(Topic, startMessage(2, QStringLiteral("b.pdf")));
    model.observe(Topic, progressMessage(2, 100.0));
    model.observe(Topic, message(QStringLiteral("dl-done"), 2));
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Path)).toString(),
             Downloads + QStringLiteral("b.pdf"));
    QCOMPARE(changedRoles(changeSpy), StatusRoles);
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
    QCOMPARE(changedRoles(changeSpy), StatusRoles);
    QCOMPARE(storedStatus(storage, 1), static_cast<int>(DownloadModel::Failed));

    model.observe(Topic, message(QStringLiteral("dl-cancel"), 2));
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Status)).toInt(),
             static_cast<int>(DownloadModel::Canceled));
    QCOMPARE(changeSpy.count(), 2);
    QCOMPARE(changeSpy.last().at(0).value<QModelIndex>().row(), 0);
    QCOMPARE(changedRoles(changeSpy), StatusRoles);
    QCOMPARE(storedStatus(storage, 2), static_cast<int>(DownloadModel::Canceled));

    model.observe(Topic, message(QStringLiteral("dl-fail"), 1));
    model.observe(Topic, message(QStringLiteral("dl-cancel"), 2));
    QCOMPARE(changeSpy.count(), 2);
}

// The engine starts a download again -- retried after it failed or was stopped -- with
// the id it had and a second dl-start. The row is the same one, from the start again.
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
    QSignalSpy startedSpy(&model, &DownloadModel::downloadStarted);

    model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
    QCOMPARE(model.count(), 1);
    QCOMPARE(countSpy.count(), 0);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::DownloadId)).toInt(), 1);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Status)).toInt(),
             static_cast<int>(DownloadModel::Running));
    // The engine kept nothing of it when it stopped: it comes from the start.
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Progress)).toInt(), 0);
    QCOMPARE(changeSpy.count(), 1);
    QCOMPARE(changedRoles(changeSpy), QVector<int>({roleId(DownloadModel::Role::Status),
                                                    roleId(DownloadModel::Role::Progress),
                                                    roleId(DownloadModel::Role::Retryable),
                                                    roleId(DownloadModel::Role::FileExists)}));
    QCOMPARE(storedStatus(storage, 1), static_cast<int>(DownloadModel::Running));
    QCOMPARE(startedSpy.count(), 1);
    QCOMPARE(startedSpy.last().at(0).toInt(), 1);

    // Running already, a repeated start changes nothing.
    model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
    QCOMPARE(changeSpy.count(), 1);
    QCOMPARE(startedSpy.count(), 1);

    // Failed before it had come any way at all, only its status changes.
    model.observe(Topic, message(QStringLiteral("dl-fail"), 1));
    model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
    QCOMPARE(changedRoles(changeSpy), StatusRoles);
    QCOMPARE(startedSpy.count(), 2);
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

    // Another topic, with a message that would otherwise start a download.
    model.observe(QStringLiteral("media-decoder-info"), startMessage(1, QStringLiteral("a.pdf")));
    model.observe(QString(), startMessage(1, QStringLiteral("a.pdf")));
    // Data that is not a message at all.
    model.observe(Topic, QVariant());
    model.observe(Topic, QStringLiteral("dl-start"));
    model.observe(Topic, QVariantList{1, 2});
    // Starts without an id the engine would give -- among them ones QVariant would read
    // an id out of: 1 from true and from a string, 1 rounded from 1.4, and a number
    // past what an int holds.
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
    // Messages about a download it never saw start.
    for (const QString &msg : {QStringLiteral("dl-progress"), QStringLiteral("dl-done"),
                               QStringLiteral("dl-fail"), QStringLiteral("dl-cancel")}) {
        model.observe(Topic, message(msg, 9));
    }
    QCOMPARE(model.count(), 0);
    QCOMPARE(insertSpy.count(), 0);
    QCOMPARE(rowsInDatabase(storage), 0);

    // And messages it does not know, about one it did.
    model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
    QSignalSpy changeSpy(&model, &DownloadModel::dataChanged);
    model.observe(Topic, message(QStringLiteral("dl-pause"), 1));
    model.observe(Topic, message(QString(), 1));
    model.observe(Topic, message(QStringLiteral("retryDownload"), 1));
    // Nor does a message reach it by something QVariant would read its id out of.
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
    // Newest first, as they were.
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::DownloadId)).toInt(), 2);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Name)).toString(), QStringLiteral("b.zip"));
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Status)).toInt(),
             static_cast<int>(DownloadModel::Canceled));
    // How far a download got is not kept.
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

    // The engine's ids do not outlive it: its next download is its id 1 again, and is
    // not the row that was id 1 before.
    model.observe(Topic, startMessage(1, QStringLiteral("c.txt")));
    QCOMPARE(model.count(), 3);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Name)).toString(), QStringLiteral("c.txt"));
    // And the rows' own ids carry on from where they were.
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::DownloadId)).toInt(), 3);
    model.observe(Topic, message(QStringLiteral("dl-fail"), 2));
    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::Status)).toInt(),
             static_cast<int>(DownloadModel::Canceled));
}

// The engine forgets every download when it starts, so one that was running when the
// application stopped will never be heard of again: it failed.
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
        // A status this build does not know, as a later one might have written.
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

    // Written back, so the database says what the list does.
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
        // Only while the list grew did its length change.
        QCOMPARE(countSpy.count(), DownloadModel::Limit);
        QCOMPARE(removeSpy.count(), 5);
        QCOMPARE(removeSpy.last().at(1).toInt(), DownloadModel::Limit);
        QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Name)).toString(),
                 QStringLiteral("f%1.bin").arg(DownloadModel::Limit + 5));
        QCOMPARE(
            role(model, DownloadModel::Limit - 1, roleId(DownloadModel::Role::Name)).toString(),
            QStringLiteral("f6.bin"));
        QCOMPARE(rowsInDatabase(storage), DownloadModel::Limit);
        // The oldest are gone from the database too, not only from the list.
        QCOMPARE(storedStatus(storage, 5), -1);
        QCOMPARE(storedStatus(storage, 6), static_cast<int>(DownloadModel::Running));

        // A download dropped off the end is forgotten with its row.
        model.observe(Topic, message(QStringLiteral("dl-done"), 1));
        QCOMPARE(model.count(), DownloadModel::Limit);

        // More rows than the list holds, as a build with a longer list might have left.
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

    // The engine's messages about it go unheard once it is forgotten; a download the
    // engine starts again comes back as a new row.
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
    model.clearEnded();
    QCOMPARE(countSpy.count(), 0);

    model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
    model.observe(Topic, startMessage(2, QStringLiteral("b.pdf")));
    model.observe(Topic, startMessage(3, QStringLiteral("c.pdf")));
    model.observe(Topic, message(QStringLiteral("dl-done"), 1));
    model.observe(Topic, message(QStringLiteral("dl-fail"), 2));
    QCOMPARE(countSpy.count(), 3);
    // What has ended goes; the one still coming stays, which forgotten would go on with
    // nothing to stop it by.
    model.clearEnded();
    QCOMPARE(model.count(), 1);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Name)).toString(), QStringLiteral("c.pdf"));
    QCOMPARE(countSpy.count(), 4);
    QCOMPARE(rowsInDatabase(storage), 1);
    // Nothing more to take: nothing said.
    model.clearEnded();
    QCOMPARE(countSpy.count(), 4);

    // Clearing everything, as the history is cleared on close, takes that one too.
    model.observe(Topic, startMessage(4, QStringLiteral("d.pdf")));
    model.observe(Topic, message(QStringLiteral("dl-done"), 4));
    model.clear();
    QCOMPARE(model.count(), 0);
    QCOMPARE(model.runningCount(), 0);
    QCOMPARE(countSpy.count(), 6);
    QCOMPARE(rowsInDatabase(storage), 0);

    DownloadModel reloaded(storage, dir.path());
    QCOMPARE(reloaded.count(), 0);
}

// The rows of downloads started at a time or since, as clearing the history takes
// them; none still coming.
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
    // What would go is counted as it would be taken: none still coming.
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

    // Nothing more to take: nothing said.
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

    // A name that means something in a URL still names the file once the URL is read
    // back, which is what opening it does.
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

// The engine saves into the folder only if it is already there, so the model makes it,
// and the folders above it, before the engine is told of it.
// A download found again by its own lasting id, after rows have come and gone around
// it: what the address bar keeps to open it by (docs/DECISIONS/0027-omnibar.md). The
// rows themselves, as kept, are what it searches.
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
    model.observe(Topic, message(QStringLiteral("dl-done"), 1));
    model.clear();
    QCOMPARE(model.rowOf(a), -1);
}

void tst_downloadmodel::directory()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    const QString folder = dir.filePath(QStringLiteral("Downloads/Salama"));
    QVERIFY(!QFileInfo::exists(dir.filePath(QStringLiteral("Downloads"))));
    DownloadModel model(storage, folder);
    QCOMPARE(model.directory(), folder);
    QVERIFY(QFileInfo(folder).isDir());

    // One that cannot be made -- a file stands where it would go -- is still the one
    // the engine is told of: the engine saves into ~/Downloads instead.
    QFile file(dir.filePath(QStringLiteral("file")));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();
    const QString blocked = file.fileName() + QStringLiteral("/Salama");
    DownloadModel unmade(storage, blocked);
    QCOMPARE(unmade.directory(), blocked);
    QVERIFY(!QFileInfo::exists(blocked));
}

// What a list other than the model's own -- the bar over the page, the platform's
// notifications -- hears of a download: that it started, or started again, and how it
// ended, by the id that lasts, once each.
void tst_downloadmodel::startedAndEnded()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    DownloadModel model(storage, dir.path());
    QSignalSpy startedSpy(&model, &DownloadModel::downloadStarted);
    QSignalSpy endedSpy(&model, &DownloadModel::downloadEnded);

    model.observe(Topic, startMessage(7, QStringLiteral("a.pdf")));
    QCOMPARE(startedSpy.count(), 1);
    const int a = role(model, 0, roleId(DownloadModel::Role::DownloadId)).toInt();
    QCOMPARE(startedSpy.last().at(0).toInt(), a);
    model.observe(Topic, progressMessage(7, 50.0));
    QCOMPARE(endedSpy.count(), 0);

    model.observe(Topic, message(QStringLiteral("dl-done"), 7));
    QCOMPARE(endedSpy.count(), 1);
    QCOMPARE(endedSpy.last().at(0).toInt(), a);
    QCOMPARE(endedSpy.last().at(1).toInt(), static_cast<int>(DownloadModel::Done));
    // Said twice, it ended once.
    model.observe(Topic, message(QStringLiteral("dl-done"), 7));
    QCOMPARE(endedSpy.count(), 1);

    model.observe(Topic, startMessage(8, QStringLiteral("b.pdf")));
    const int b = role(model, 0, roleId(DownloadModel::Role::DownloadId)).toInt();
    QCOMPARE(startedSpy.last().at(0).toInt(), b);
    model.observe(Topic, message(QStringLiteral("dl-fail"), 8));
    QCOMPARE(endedSpy.last().at(0).toInt(), b);
    QCOMPARE(endedSpy.last().at(1).toInt(), static_cast<int>(DownloadModel::Failed));
    model.observe(Topic, startMessage(8, QStringLiteral("b.pdf")));
    QCOMPARE(startedSpy.count(), 3);
    QCOMPARE(startedSpy.last().at(0).toInt(), b);
    model.observe(Topic, message(QStringLiteral("dl-cancel"), 8));
    QCOMPARE(endedSpy.count(), 3);
    QCOMPARE(endedSpy.last().at(1).toInt(), static_cast<int>(DownloadModel::Canceled));
    model.observe(Topic, message(QStringLiteral("dl-cancel"), 8));
    QCOMPARE(endedSpy.count(), 3);
}

// A download still coming is stopped by the engine, which is asked for it by its own
// id; the row says it stopped when the engine does.
void tst_downloadmodel::stop()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    DownloadModel model(storage, dir.path());
    QSignalSpy requestSpy(&model, &DownloadModel::engineRequest);
    model.observe(Topic, startMessage(4, QStringLiteral("a.pdf")));
    model.observe(Topic, startMessage(5, QStringLiteral("b.pdf")));

    model.stop(1);
    QCOMPARE(requestSpy.count(), 1);
    QCOMPARE(requestTopic(requestSpy), QStringLiteral("embedui:download"));
    // As EmbedliteDownloadManager.js compares it, with ===: a number.
    QCOMPARE(request(requestSpy),
             (QVariantMap{{QStringLiteral("msg"), QStringLiteral("cancelDownload")},
                          {QStringLiteral("id"), 4}}));
    QCOMPARE(request(requestSpy).value(QStringLiteral("id")).userType(), int(QMetaType::Int));
    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::Status)).toInt(),
             static_cast<int>(DownloadModel::Running));
    model.observe(Topic, message(QStringLiteral("dl-cancel"), 4));
    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::Status)).toInt(),
             static_cast<int>(DownloadModel::Canceled));

    // Nothing to stop: one stopped already, one arrived, one from an earlier run, and
    // rows that are not there.
    model.stop(1);
    model.observe(Topic, message(QStringLiteral("dl-done"), 5));
    model.stop(0);
    model.stop(-1);
    model.stop(2);
    QCOMPARE(requestSpy.count(), 1);
    {
        QSqlQuery insert(storage.database());
        QVERIFY(insert.exec(QStringLiteral("INSERT INTO download (id, name, status, started) "
                                           "VALUES (9, 'old.pdf', 0, 1)")));
    }
    DownloadModel reloaded(storage, dir.path());
    QSignalSpy reloadedSpy(&reloaded, &DownloadModel::engineRequest);
    for (int row = 0; row < reloaded.count(); ++row) {
        reloaded.stop(row);
    }
    QCOMPARE(reloadedSpy.count(), 0);
}

// A download of this run that failed or was stopped is started again by the engine,
// which still has it: asked by its id, it says dl-start for that id again.
void tst_downloadmodel::retry()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    DownloadModel model(storage, dir.path());
    QSignalSpy requestSpy(&model, &DownloadModel::engineRequest);
    model.observe(Topic, startMessage(3, QStringLiteral("a.pdf")));
    model.observe(Topic, progressMessage(3, 60.0));

    // Not while it is coming.
    model.retry(0);
    QCOMPARE(requestSpy.count(), 0);

    model.observe(Topic, message(QStringLiteral("dl-fail"), 3));
    model.retry(0);
    QCOMPARE(requestSpy.count(), 1);
    QCOMPARE(requestTopic(requestSpy), QStringLiteral("embedui:download"));
    QCOMPARE(request(requestSpy),
             (QVariantMap{{QStringLiteral("msg"), QStringLiteral("retryDownload")},
                          {QStringLiteral("id"), 3}}));
    model.observe(Topic, startMessage(3, QStringLiteral("a.pdf")));
    QCOMPARE(model.count(), 1);
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Status)).toInt(),
             static_cast<int>(DownloadModel::Running));

    model.observe(Topic, message(QStringLiteral("dl-cancel"), 3));
    model.retry(0);
    QCOMPARE(requestSpy.count(), 2);
    QCOMPARE(request(requestSpy).value(QStringLiteral("msg")).toString(),
             QStringLiteral("retryDownload"));

    // Nor once it has arrived, nor for a row that is not there.
    model.observe(Topic, startMessage(3, QStringLiteral("a.pdf")));
    model.observe(Topic, message(QStringLiteral("dl-done"), 3));
    model.retry(0);
    model.retry(1);
    model.retry(-1);
    QCOMPARE(requestSpy.count(), 2);
}

// One from an earlier run the engine has forgotten, and is asked to fetch from where it
// came from to where it went. What it starts has an id of its own, and goes where the
// row's file went: that is the row, started again, not a new one.
void tst_downloadmodel::refetch()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    {
        DownloadModel earlier(storage, dir.path());
        earlier.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
        earlier.observe(Topic, startMessage(2, QStringLiteral("b.pdf")));
        earlier.observe(Topic, message(QStringLiteral("dl-fail"), 2));
    }
    DownloadModel model(storage, dir.path());
    QSignalSpy requestSpy(&model, &DownloadModel::engineRequest);
    QSignalSpy startedSpy(&model, &DownloadModel::downloadStarted);
    QCOMPARE(model.count(), 2);
    const int b = role(model, 0, roleId(DownloadModel::Role::DownloadId)).toInt();
    const int a = role(model, 1, roleId(DownloadModel::Role::DownloadId)).toInt();

    model.retry(1);
    QCOMPARE(requestSpy.count(), 1);
    QCOMPARE(requestTopic(requestSpy), QStringLiteral("embedui:download"));
    QCOMPARE(request(requestSpy),
             (QVariantMap{{QStringLiteral("msg"), QStringLiteral("addDownload")},
                          {QStringLiteral("from"), QStringLiteral("https://files.example/a.pdf")},
                          {QStringLiteral("to"), Downloads + QStringLiteral("a.pdf")}}));
    // Nothing changes until the engine says it started.
    QCOMPARE(role(model, 1, roleId(DownloadModel::Role::Status)).toInt(),
             static_cast<int>(DownloadModel::Failed));

    // Something else the engine starts meanwhile is a row of its own.
    model.observe(Topic, startMessage(1, QStringLiteral("c.pdf")));
    QCOMPARE(model.count(), 3);
    model.observe(Topic, startMessage(2, QStringLiteral("a.pdf")));
    QCOMPARE(model.count(), 3);
    QCOMPARE(model.rowOf(a), 2);
    QCOMPARE(role(model, 2, roleId(DownloadModel::Role::Status)).toInt(),
             static_cast<int>(DownloadModel::Running));
    QCOMPARE(storedStatus(storage, a), static_cast<int>(DownloadModel::Running));
    QCOMPARE(startedSpy.last().at(0).toInt(), a);
    // And what the engine says of it by the new id is the row's.
    model.observe(Topic, progressMessage(2, 30.0));
    QCOMPARE(role(model, 2, roleId(DownloadModel::Role::Progress)).toInt(), 30);
    model.observe(Topic, message(QStringLiteral("dl-done"), 2));
    QCOMPARE(role(model, 2, roleId(DownloadModel::Role::Status)).toInt(),
             static_cast<int>(DownloadModel::Done));
    // Started again, a second start for the same file is a new row: no row waits for
    // it any more.
    model.observe(Topic, startMessage(3, QStringLiteral("a.pdf")));
    QCOMPARE(model.count(), 4);
    QCOMPARE(model.rowOf(b), 2);
}

// Which downloads can be fetched again: one that failed or was stopped, either still
// known to the engine, or with an address of its own and somewhere to save it.
void tst_downloadmodel::retryable()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    DownloadModel::Download download;
    download.url = QStringLiteral("https://files.example/a.pdf");
    download.path = Downloads + QStringLiteral("a.pdf");
    download.status = DownloadModel::Failed;
    QVERIFY(DownloadModel::canRetry(download));
    download.status = DownloadModel::Canceled;
    QVERIFY(DownloadModel::canRetry(download));
    download.url = QStringLiteral("http://files.example/a.pdf");
    QVERIFY(DownloadModel::canRetry(download));
    for (const DownloadModel::Status status : {DownloadModel::Running, DownloadModel::Done}) {
        download.status = status;
        QVERIFY(!DownloadModel::canRetry(download));
        download.engineId = 1;
        QVERIFY(!DownloadModel::canRetry(download));
        download.engineId = 0;
    }

    // An address that means nothing without the page that made it, or none, can be
    // fetched again only by the engine that still has it.
    download.status = DownloadModel::Failed;
    for (const QString &url : {QStringLiteral("blob:https://files.example/1234"),
                               QStringLiteral("data:text/plain,hello"), QString()}) {
        download.url = url;
        QVERIFY(!DownloadModel::canRetry(download));
        download.engineId = 2;
        QVERIFY(DownloadModel::canRetry(download));
        download.engineId = 0;
    }
    download.url = QStringLiteral("https://files.example/a.pdf");
    download.path.clear();
    QVERIFY(!DownloadModel::canRetry(download));

    // And as the list reads it.
    DownloadModel model(storage, dir.path());
    model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Retryable)).toBool(), false);
    model.observe(Topic, message(QStringLiteral("dl-fail"), 1));
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Retryable)).toBool(), true);
}

// Deleting the file goes with forgetting the row; one still coming is stopped first,
// and the engine takes away what it had written.
void tst_downloadmodel::deleteFile()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    DownloadModel model(storage, dir.path());
    QSignalSpy requestSpy(&model, &DownloadModel::engineRequest);
    const QString path = makeFile(dir, QStringLiteral("a.pdf"));
    QVERIFY(QFileInfo::exists(path));
    model.observe(Topic, startIn(dir, 1, QStringLiteral("a.pdf")));
    model.observe(Topic, message(QStringLiteral("dl-done"), 1));
    model.observe(Topic, startIn(dir, 2, QStringLiteral("b.pdf")));
    QCOMPARE(model.count(), 2);

    QVERIFY(model.deleteFile(1));
    QVERIFY(!QFileInfo::exists(path));
    QCOMPARE(model.count(), 1);
    QCOMPARE(rowsInDatabase(storage), 1);
    QCOMPARE(requestSpy.count(), 0);

    QVERIFY(model.deleteFile(0));
    QCOMPARE(model.count(), 0);
    QCOMPARE(model.runningCount(), 0);
    QCOMPARE(requestSpy.count(), 1);
    QCOMPARE(request(requestSpy),
             (QVariantMap{{QStringLiteral("msg"), QStringLiteral("cancelDownload")},
                          {QStringLiteral("id"), 2}}));
    // What the engine says of it after is about a row no longer there.
    model.observe(Topic, message(QStringLiteral("dl-cancel"), 2));
    QCOMPARE(model.count(), 0);

    // A file no longer there leaves only the row to forget; a folder where the file
    // was is not the file, and is left alone.
    model.observe(Topic, startIn(dir, 3, QStringLiteral("gone.pdf")));
    model.observe(Topic, message(QStringLiteral("dl-done"), 3));
    QVERIFY(model.deleteFile(0));
    QCOMPARE(model.count(), 0);
    QVERIFY(QDir(dir.path()).mkdir(QStringLiteral("folder")));
    model.observe(Topic, startIn(dir, 4, QStringLiteral("folder")));
    model.observe(Topic, message(QStringLiteral("dl-done"), 4));
    QVERIFY(model.deleteFile(0));
    QVERIFY(QFileInfo(dir.filePath(QStringLiteral("folder"))).isDir());

    QVERIFY(!model.deleteFile(0));
    QVERIFY(!model.deleteFile(-1));
}

// Whether the file is there is asked of the disk, for a download that arrived; and
// the list is told to ask again when it may have changed under it.
void tst_downloadmodel::fileExists()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    DownloadModel model(storage, dir.path());
    QSignalSpy changeSpy(&model, &DownloadModel::dataChanged);
    model.refresh();
    QCOMPARE(changeSpy.count(), 0);

    const QString path = makeFile(dir, QStringLiteral("a.pdf"));
    model.observe(Topic, startIn(dir, 1, QStringLiteral("a.pdf")));
    // Coming, it is not there yet, whatever is in its place.
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::FileExists)).toBool(), false);
    model.observe(Topic, message(QStringLiteral("dl-done"), 1));
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::FileExists)).toBool(), true);
    QVERIFY(QFile::remove(path));
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::FileExists)).toBool(), false);
    model.observe(Topic, startIn(dir, 2, QStringLiteral("b.pdf")));

    const int before = changeSpy.count();
    model.refresh();
    QCOMPARE(changeSpy.count(), before + 1);
    QCOMPARE(changeSpy.last().at(0).value<QModelIndex>().row(), 0);
    QCOMPARE(changeSpy.last().at(1).value<QModelIndex>().row(), 1);
    QCOMPARE(changedRoles(changeSpy), QVector<int>{roleId(DownloadModel::Role::FileExists)});
}

// The theme's icon for the kind of file, as the platform's file manager draws one: by
// its type, and by its name where the type says nothing.
void tst_downloadmodel::icons()
{
    const auto icon = [](const char *kind) {
        return QStringLiteral("image://theme/icon-m-file-") + QLatin1String(kind);
    };
    const QString none;
    QCOMPARE(DownloadModel::iconFor(QStringLiteral("application/pdf"), none), icon("pdf"));
    QCOMPARE(DownloadModel::iconFor(QStringLiteral("image/png"), none), icon("image"));
    QCOMPARE(DownloadModel::iconFor(QStringLiteral("audio/ogg"), none), icon("audio"));
    QCOMPARE(DownloadModel::iconFor(QStringLiteral("VIDEO/MP4"), none), icon("video"));
    QCOMPARE(
        DownloadModel::iconFor(QStringLiteral("application/vnd.android.package-archive"), none),
        icon("apk"));
    QCOMPARE(DownloadModel::iconFor(QStringLiteral("application/x-rpm"), none), icon("rpm"));
    QCOMPARE(DownloadModel::iconFor(QStringLiteral("text/x-vcard"), none), icon("vcard"));
    QCOMPARE(DownloadModel::iconFor(QStringLiteral("text/vcard"), none), icon("vcard"));
    QCOMPARE(DownloadModel::iconFor(
                 QStringLiteral("application/vnd.oasis.opendocument.spreadsheet"), none),
             icon("spreadsheet"));
    QCOMPARE(DownloadModel::iconFor(QStringLiteral("application/vnd.ms-excel"), none),
             icon("spreadsheet"));
    QCOMPARE(DownloadModel::iconFor(QStringLiteral("text/csv"), none), icon("spreadsheet"));
    QCOMPARE(DownloadModel::iconFor(QStringLiteral("application/vnd.openxmlformats-officedocument."
                                                   "presentationml.presentation"),
                                    none),
             icon("presentation"));
    QCOMPARE(DownloadModel::iconFor(QStringLiteral("application/vnd.ms-powerpoint"), none),
             icon("presentation"));
    QCOMPARE(DownloadModel::iconFor(QStringLiteral("application/zip"), none),
             icon("archive-folder"));
    QCOMPARE(DownloadModel::iconFor(QStringLiteral("application/x-7z-compressed"), none),
             icon("archive-folder"));
    QCOMPARE(DownloadModel::iconFor(QStringLiteral("text/plain"), none), icon("document"));
    QCOMPARE(DownloadModel::iconFor(QStringLiteral("application/msword"), none), icon("document"));
    QCOMPARE(DownloadModel::iconFor(QStringLiteral("application/vnd.openxmlformats-officedocument."
                                                   "wordprocessingml.document"),
                                    none),
             icon("document"));
    QCOMPARE(DownloadModel::iconFor(QStringLiteral("application/epub+zip"), none),
             icon("document"));

    // A type that says nothing useful, or none: the name's extension, whatever its case.
    for (const QString &type : {QString(), QStringLiteral("application/octet-stream"),
                                QStringLiteral("binary/octet-stream")}) {
        QCOMPARE(DownloadModel::iconFor(type, QStringLiteral("report.PDF")), icon("pdf"));
        QCOMPARE(DownloadModel::iconFor(type, QStringLiteral("photo.jpeg")), icon("image"));
        QCOMPARE(DownloadModel::iconFor(type, QStringLiteral("song.flac")), icon("audio"));
        QCOMPARE(DownloadModel::iconFor(type, QStringLiteral("clip.webm")), icon("video"));
        QCOMPARE(DownloadModel::iconFor(type, QStringLiteral("app.apk")), icon("apk"));
        QCOMPARE(DownloadModel::iconFor(type, QStringLiteral("pkg.rpm")), icon("rpm"));
        QCOMPARE(DownloadModel::iconFor(type, QStringLiteral("card.vcf")), icon("vcard"));
        QCOMPARE(DownloadModel::iconFor(type, QStringLiteral("sheet.xlsx")), icon("spreadsheet"));
        QCOMPARE(DownloadModel::iconFor(type, QStringLiteral("deck.odp")), icon("presentation"));
        QCOMPARE(DownloadModel::iconFor(type, QStringLiteral("src.tar.gz")),
                 icon("archive-folder"));
        QCOMPARE(DownloadModel::iconFor(type, QStringLiteral("notes.txt")), icon("document"));
        QCOMPARE(DownloadModel::iconFor(type, QStringLiteral("thing.xyz")), icon("other"));
        QCOMPARE(DownloadModel::iconFor(type, QStringLiteral("README")), icon("other"));
    }
    // The type comes first: a name that says otherwise does not change it.
    QCOMPARE(DownloadModel::iconFor(QStringLiteral("image/png"), QStringLiteral("a.pdf")),
             icon("image"));

    // And as the list reads it.
    QTemporaryDir dir;
    Storage storage(dir.path());
    DownloadModel model(storage, dir.path());
    model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
    QCOMPARE(role(model, 0, roleId(DownloadModel::Role::Icon)).toString(), icon("pdf"));
}

// One download's roles by name, for what shows one outside the list.
void tst_downloadmodel::details()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    DownloadModel model(storage, dir.path());
    QVERIFY(model.details(1).isEmpty());
    QCOMPARE(model.directoryUrl(), QUrl::fromLocalFile(dir.path()).toString());

    model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
    model.observe(Topic, progressMessage(1, 25.0));
    model.observe(Topic, startMessage(2, QStringLiteral("b.zip")));
    const int a = role(model, 1, roleId(DownloadModel::Role::DownloadId)).toInt();
    const QVariantMap details = model.details(a);
    QCOMPARE(details.count(), model.roleNames().count());
    QCOMPARE(details.value(QStringLiteral("downloadId")).toInt(), a);
    QCOMPARE(details.value(QStringLiteral("name")).toString(), QStringLiteral("a.pdf"));
    QCOMPARE(details.value(QStringLiteral("progress")).toInt(), 25);
    QCOMPARE(details.value(QStringLiteral("size")).toLongLong(), 2048LL);
    QCOMPARE(details.value(QStringLiteral("status")).toInt(),
             static_cast<int>(DownloadModel::Running));
    QCOMPARE(details.value(QStringLiteral("icon")).toString(),
             QStringLiteral("image://theme/icon-m-file-pdf"));
    QCOMPARE(details.value(QStringLiteral("retryable")).toBool(), false);

    model.remove(model.rowOf(a));
    QVERIFY(model.details(a).isEmpty());
}

// A database that would not open costs the list its memory, not its use.
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

QTEST_GUILESS_MAIN(tst_downloadmodel)
#include "tst_downloadmodel.moc"
