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
    void fileUrl();
    void rowOf();
    void directory();
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
    // The topic sailfish-browser's DownloadManager listens on for the same messages.
    QCOMPARE(model.topic(), Topic);
    QCOMPARE(model.count(), 0);

    const QHash<int, QByteArray> roles = model.roleNames();
    QCOMPARE(roles.value(DownloadModel::DownloadIdRole), QByteArray("downloadId"));
    QCOMPARE(roles.value(DownloadModel::NameRole), QByteArray("name"));
    QCOMPARE(roles.value(DownloadModel::UrlRole), QByteArray("url"));
    QCOMPARE(roles.value(DownloadModel::PathRole), QByteArray("path"));
    QCOMPARE(roles.value(DownloadModel::MimeTypeRole), QByteArray("mimeType"));
    QCOMPARE(roles.value(DownloadModel::SizeRole), QByteArray("size"));
    QCOMPARE(roles.value(DownloadModel::ProgressRole), QByteArray("progress"));
    QCOMPARE(roles.value(DownloadModel::StatusRole), QByteArray("status"));
    QCOMPARE(roles.value(DownloadModel::StartedRole), QByteArray("started"));
    QCOMPARE(roles.count(), 9);

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
    QVERIFY(!role(model, 1, DownloadModel::NameRole).isValid());
    QVERIFY(!role(model, -1, DownloadModel::NameRole).isValid());
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

    QCOMPARE(role(model, 0, DownloadModel::DownloadIdRole).toInt(), 1);
    QCOMPARE(role(model, 0, DownloadModel::NameRole).toString(), QStringLiteral("a.pdf"));
    QCOMPARE(role(model, 0, DownloadModel::UrlRole).toString(),
             QStringLiteral("https://files.example/a.pdf"));
    QCOMPARE(role(model, 0, DownloadModel::PathRole).toString(),
             Downloads + QStringLiteral("a.pdf"));
    QCOMPARE(role(model, 0, DownloadModel::MimeTypeRole).toString(),
             QStringLiteral("application/pdf"));
    QCOMPARE(role(model, 0, DownloadModel::SizeRole).toLongLong(), 2048LL);
    QCOMPARE(role(model, 0, DownloadModel::ProgressRole).toInt(), 0);
    QCOMPARE(role(model, 0, DownloadModel::StatusRole).toInt(),
             static_cast<int>(DownloadModel::Running));
    const qint64 started = role(model, 0, DownloadModel::StartedRole).toLongLong();
    QVERIFY(started >= before && started <= after);
    QCOMPARE(rowsInDatabase(storage), 1);
    QCOMPARE(storedStatus(storage, 1), static_cast<int>(DownloadModel::Running));

    // The next one goes above it, with an id of its own.
    model.observe(Topic, startMessage(2, QStringLiteral("b.zip")));
    QCOMPARE(model.count(), 2);
    QCOMPARE(countSpy.count(), 2);
    QCOMPARE(insertSpy.last().at(1).toInt(), 0);
    QCOMPARE(role(model, 0, DownloadModel::NameRole).toString(), QStringLiteral("b.zip"));
    QCOMPARE(role(model, 0, DownloadModel::DownloadIdRole).toInt(), 2);
    QCOMPARE(role(model, 1, DownloadModel::NameRole).toString(), QStringLiteral("a.pdf"));

    // A size the engine does not know yet is 0; one that makes no sense is too.
    QVariantMap unsized = startMessage(3, QStringLiteral("c.bin"));
    unsized.insert(QStringLiteral("size"), 0.0);
    model.observe(Topic, unsized);
    QCOMPARE(role(model, 0, DownloadModel::SizeRole).toLongLong(), 0LL);
    QVariantMap negative = startMessage(4, QStringLiteral("d.bin"));
    negative.insert(QStringLiteral("size"), -5.0);
    model.observe(Topic, negative);
    QCOMPARE(role(model, 0, DownloadModel::SizeRole).toLongLong(), 0LL);
    // Nor is a size that is not a number, though QVariant would read one out of it.
    QVariantMap spelled = startMessage(5, QStringLiteral("e.bin"));
    spelled.insert(QStringLiteral("size"), QStringLiteral("2048"));
    model.observe(Topic, spelled);
    QCOMPARE(role(model, 0, DownloadModel::SizeRole).toLongLong(), 0LL);
}

void tst_downloadmodel::nameFallsBackToTheFile()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    DownloadModel model(storage, dir.path());

    QVariantMap nameless = startMessage(1, QStringLiteral("report.pdf"));
    nameless.remove(QStringLiteral("displayName"));
    model.observe(Topic, nameless);
    QCOMPARE(role(model, 0, DownloadModel::NameRole).toString(), QStringLiteral("report.pdf"));

    QVariantMap blank = startMessage(2, QStringLiteral("notes.txt"));
    blank.insert(QStringLiteral("displayName"), QString());
    model.observe(Topic, blank);
    QCOMPARE(role(model, 0, DownloadModel::NameRole).toString(), QStringLiteral("notes.txt"));
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
    QCOMPARE(role(model, 1, DownloadModel::ProgressRole).toInt(), 42);
    QCOMPARE(role(model, 0, DownloadModel::ProgressRole).toInt(), 0);
    QCOMPARE(changeSpy.count(), 1);
    QCOMPARE(changeSpy.last().at(0).value<QModelIndex>().row(), 1);
    QCOMPARE(changedRoles(changeSpy), QVector<int>{DownloadModel::ProgressRole});

    // The same figure again changes nothing.
    model.observe(Topic, progressMessage(1, 42.0));
    QCOMPARE(changeSpy.count(), 1);

    // Held to a percentage whatever the message says.
    model.observe(Topic, progressMessage(1, 250.0));
    QCOMPARE(role(model, 1, DownloadModel::ProgressRole).toInt(), 100);
    model.observe(Topic, progressMessage(1, -3.0));
    QCOMPARE(role(model, 1, DownloadModel::ProgressRole).toInt(), 0);
    model.observe(Topic, progressMessage(1, 1e30));
    QCOMPARE(role(model, 1, DownloadModel::ProgressRole).toInt(), 100);
    model.observe(Topic, progressMessage(1, 66.6));
    QCOMPARE(role(model, 1, DownloadModel::ProgressRole).toInt(), 67);
    QCOMPARE(changeSpy.count(), 5);

    // A message with no figure in it says nothing; nor does one whose figure is not a
    // number, though QVariant would read 42 out of the one and 1 out of the other.
    model.observe(Topic, progressMessage(1, QStringLiteral("most of it")));
    model.observe(Topic, progressMessage(1, QVariant()));
    model.observe(Topic, message(QStringLiteral("dl-progress"), 1));
    model.observe(Topic, progressMessage(1, QStringLiteral("42")));
    model.observe(Topic, progressMessage(1, true));
    QCOMPARE(role(model, 1, DownloadModel::ProgressRole).toInt(), 67);
    QCOMPARE(changeSpy.count(), 5);
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
    QCOMPARE(role(model, 0, DownloadModel::SizeRole).toLongLong(), 5000000000LL);
    QVariantMap progress = progressMessage(1, QVariant(40LL));
    progress.insert(QStringLiteral("id"), QVariant(1LL));
    model.observe(Topic, progress);
    QCOMPARE(role(model, 0, DownloadModel::ProgressRole).toInt(), 40);

    const QVariantMap fromQml{
        {QStringLiteral("msg"), QStringLiteral("dl-start")},
        {QStringLiteral("id"), 2},
        {QStringLiteral("displayName"), QStringLiteral("b.pdf")},
        {QStringLiteral("size"), 10},
    };
    model.observe(Topic, fromQml);
    QCOMPARE(model.count(), 2);
    QCOMPARE(role(model, 0, DownloadModel::SizeRole).toLongLong(), 10LL);
    model.observe(Topic, QVariantMap{{QStringLiteral("msg"), QStringLiteral("dl-progress")},
                                     {QStringLiteral("id"), 2},
                                     {QStringLiteral("percent"), 55}});
    QCOMPARE(role(model, 0, DownloadModel::ProgressRole).toInt(), 55);
    model.observe(Topic, QVariantMap{{QStringLiteral("msg"), QStringLiteral("dl-done")},
                                     {QStringLiteral("id"), 2}});
    QCOMPARE(role(model, 0, DownloadModel::StatusRole).toInt(),
             static_cast<int>(DownloadModel::Done));
    QCOMPARE(role(model, 1, DownloadModel::StatusRole).toInt(),
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
    QCOMPARE(role(model, 0, DownloadModel::StatusRole).toInt(),
             static_cast<int>(DownloadModel::Done));
    QCOMPARE(role(model, 0, DownloadModel::ProgressRole).toInt(), 100);
    QCOMPARE(role(model, 0, DownloadModel::PathRole).toString(),
             Downloads + QStringLiteral("a(1).pdf"));
    QCOMPARE(changeSpy.count(), 1);
    QCOMPARE(changedRoles(changeSpy),
             QVector<int>({DownloadModel::StatusRole, DownloadModel::ProgressRole,
                           DownloadModel::PathRole}));
    QCOMPARE(storedStatus(storage, 1), static_cast<int>(DownloadModel::Done));

    // Said twice, it changes nothing the second time.
    model.observe(Topic, done);
    QCOMPARE(changeSpy.count(), 1);

    // Without a path, the one the download started with stands.
    model.observe(Topic, startMessage(2, QStringLiteral("b.pdf")));
    model.observe(Topic, progressMessage(2, 100.0));
    model.observe(Topic, message(QStringLiteral("dl-done"), 2));
    QCOMPARE(role(model, 0, DownloadModel::PathRole).toString(),
             Downloads + QStringLiteral("b.pdf"));
    QCOMPARE(changedRoles(changeSpy), QVector<int>{DownloadModel::StatusRole});
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
    QCOMPARE(role(model, 1, DownloadModel::StatusRole).toInt(),
             static_cast<int>(DownloadModel::Failed));
    QCOMPARE(changeSpy.count(), 1);
    QCOMPARE(changeSpy.last().at(0).value<QModelIndex>().row(), 1);
    QCOMPARE(changedRoles(changeSpy), QVector<int>{DownloadModel::StatusRole});
    QCOMPARE(storedStatus(storage, 1), static_cast<int>(DownloadModel::Failed));

    model.observe(Topic, message(QStringLiteral("dl-cancel"), 2));
    QCOMPARE(role(model, 0, DownloadModel::StatusRole).toInt(),
             static_cast<int>(DownloadModel::Canceled));
    QCOMPARE(changeSpy.count(), 2);
    QCOMPARE(changeSpy.last().at(0).value<QModelIndex>().row(), 0);
    QCOMPARE(changedRoles(changeSpy), QVector<int>{DownloadModel::StatusRole});
    QCOMPARE(storedStatus(storage, 2), static_cast<int>(DownloadModel::Canceled));

    model.observe(Topic, message(QStringLiteral("dl-fail"), 1));
    model.observe(Topic, message(QStringLiteral("dl-cancel"), 2));
    QCOMPARE(changeSpy.count(), 2);
}

// The engine starts a download again -- retried after it failed, resumed after it was
// canceled -- with the id it had and a second dl-start. The row is the same one.
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
    QCOMPARE(role(model, 0, DownloadModel::DownloadIdRole).toInt(), 1);
    QCOMPARE(role(model, 0, DownloadModel::StatusRole).toInt(),
             static_cast<int>(DownloadModel::Running));
    // Resumed from where it stopped; the engine says so if it starts over.
    QCOMPARE(role(model, 0, DownloadModel::ProgressRole).toInt(), 30);
    QCOMPARE(changeSpy.count(), 1);
    QCOMPARE(changedRoles(changeSpy), QVector<int>{DownloadModel::StatusRole});
    QCOMPARE(storedStatus(storage, 1), static_cast<int>(DownloadModel::Running));

    // Running already, a repeated start changes nothing.
    model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
    QCOMPARE(changeSpy.count(), 1);

    model.observe(Topic, message(QStringLiteral("dl-fail"), 1));
    model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
    model.observe(Topic, message(QStringLiteral("dl-done"), 1));
    QCOMPARE(model.count(), 1);
    QCOMPARE(role(model, 0, DownloadModel::StatusRole).toInt(),
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
    QCOMPARE(role(model, 0, DownloadModel::StatusRole).toInt(),
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
        started = role(model, 1, DownloadModel::StartedRole).toLongLong();
    }

    Storage storage(dir.path());
    DownloadModel model(storage, dir.path());
    QCOMPARE(model.count(), 2);
    // Newest first, as they were.
    QCOMPARE(role(model, 0, DownloadModel::DownloadIdRole).toInt(), 2);
    QCOMPARE(role(model, 0, DownloadModel::NameRole).toString(), QStringLiteral("b.zip"));
    QCOMPARE(role(model, 0, DownloadModel::StatusRole).toInt(),
             static_cast<int>(DownloadModel::Canceled));
    // How far a download got is not kept.
    QCOMPARE(role(model, 0, DownloadModel::ProgressRole).toInt(), 0);

    QCOMPARE(role(model, 1, DownloadModel::DownloadIdRole).toInt(), 1);
    QCOMPARE(role(model, 1, DownloadModel::NameRole).toString(), QStringLiteral("a.pdf"));
    QCOMPARE(role(model, 1, DownloadModel::UrlRole).toString(),
             QStringLiteral("https://files.example/a.pdf"));
    QCOMPARE(role(model, 1, DownloadModel::PathRole).toString(),
             Downloads + QStringLiteral("a(1).pdf"));
    QCOMPARE(role(model, 1, DownloadModel::MimeTypeRole).toString(),
             QStringLiteral("application/pdf"));
    QCOMPARE(role(model, 1, DownloadModel::SizeRole).toLongLong(), 2048LL);
    QCOMPARE(role(model, 1, DownloadModel::StatusRole).toInt(),
             static_cast<int>(DownloadModel::Done));
    QCOMPARE(role(model, 1, DownloadModel::ProgressRole).toInt(), 100);
    QCOMPARE(role(model, 1, DownloadModel::StartedRole).toLongLong(), started);

    // The engine's ids do not outlive it: its next download is its id 1 again, and is
    // not the row that was id 1 before.
    model.observe(Topic, startMessage(1, QStringLiteral("c.txt")));
    QCOMPARE(model.count(), 3);
    QCOMPARE(role(model, 0, DownloadModel::NameRole).toString(), QStringLiteral("c.txt"));
    // And the rows' own ids carry on from where they were.
    QCOMPARE(role(model, 0, DownloadModel::DownloadIdRole).toInt(), 3);
    model.observe(Topic, message(QStringLiteral("dl-fail"), 2));
    QCOMPARE(role(model, 1, DownloadModel::StatusRole).toInt(),
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
    QCOMPARE(role(model, 1, DownloadModel::NameRole).toString(), QStringLiteral("a.pdf"));
    QCOMPARE(role(model, 1, DownloadModel::StatusRole).toInt(),
             static_cast<int>(DownloadModel::Failed));
    QCOMPARE(role(model, 1, DownloadModel::ProgressRole).toInt(), 0);
    QCOMPARE(role(model, 0, DownloadModel::StatusRole).toInt(),
             static_cast<int>(DownloadModel::Done));
    QCOMPARE(role(model, 2, DownloadModel::NameRole).toString(), QStringLiteral("odd"));
    QCOMPARE(role(model, 2, DownloadModel::StatusRole).toInt(),
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
        QCOMPARE(role(model, 0, DownloadModel::NameRole).toString(),
                 QStringLiteral("f%1.bin").arg(DownloadModel::Limit + 5));
        QCOMPARE(role(model, DownloadModel::Limit - 1, DownloadModel::NameRole).toString(),
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
    QCOMPARE(role(model, DownloadModel::Limit - 1, DownloadModel::NameRole).toString(),
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
    QCOMPARE(role(model, 0, DownloadModel::NameRole).toString(), QStringLiteral("b.pdf"));
    QCOMPARE(rowsInDatabase(storage), 1);
    QCOMPARE(storedStatus(storage, 1), -1);

    // The engine's messages about it go unheard once it is forgotten; a download the
    // engine starts again comes back as a new row.
    QSignalSpy changeSpy(&model, &DownloadModel::dataChanged);
    model.observe(Topic, message(QStringLiteral("dl-fail"), 1));
    QCOMPARE(changeSpy.count(), 0);
    model.observe(Topic, startMessage(1, QStringLiteral("a.pdf")));
    QCOMPARE(model.count(), 2);
    QCOMPARE(role(model, 0, DownloadModel::DownloadIdRole).toInt(), 3);
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
    const int a = role(model, 0, DownloadModel::DownloadIdRole).toInt();
    model.observe(Topic, startMessage(2, QStringLiteral("b.pdf")));
    const int b = role(model, 0, DownloadModel::DownloadIdRole).toInt();
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
    QCOMPARE(role(model, 0, DownloadModel::StatusRole).toInt(),
             static_cast<int>(DownloadModel::Failed));
    model.remove(0);
    QCOMPARE(model.count(), 0);
}

QTEST_GUILESS_MAIN(tst_downloadmodel)
#include "tst_downloadmodel.moc"
