// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "history/HistoryModel.h"
#include "storage/Storage.h"

#include <QDateTime>
#include <QSignalSpy>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>
#include <cmath>

using Salama::HistoryModel;
using Salama::Storage;

class tst_historymodel : public QObject
{
    Q_OBJECT

private slots:
    void visitsAndCounts();
    void ignoresUnrecordableUrls();
    void search();
    void titles();
    void removeAndClear();
    void prunesAndLimits();
    void wholeTable();
    void clearSince();
    void rangeStart();
    void learnsWhatWasTyped();
    void learningWearsDown();
    void learningGoesWithThePage();
};

namespace {

QVariant role(const HistoryModel &model, int row, int role)
{
    return model.data(model.index(row, 0), role);
}

// A page of the history as the table would hold it, last visited at this time.
void addHistory(const Storage &storage, const QString &url, qint64 date)
{
    QSqlQuery insert(storage.database());
    insert.prepare(QStringLiteral("INSERT INTO browser_history (url, date) VALUES (?, ?)"));
    insert.addBindValue(url);
    insert.addBindValue(date);
    QVERIFY(insert.exec());
}

int inputsInDatabase(const Storage &storage)
{
    QSqlQuery query(storage.database());
    query.exec(QStringLiteral("SELECT COUNT(*) FROM input_history"));
    return query.next() ? query.value(0).toInt() : -1;
}

int rowsInDatabase(const Storage &storage)
{
    QSqlQuery query(storage.database());
    query.exec(QStringLiteral("SELECT COUNT(*) FROM browser_history"));
    return query.next() ? query.value(0).toInt() : -1;
}

} // namespace

void tst_historymodel::visitsAndCounts()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    HistoryModel model(storage);
    QSignalSpy countSpy(&model, &HistoryModel::countChanged);
    QCOMPARE(model.count(), 0);
    QCOMPARE(model.rowCount(model.index(0, 0)), 0);

    model.visit(QStringLiteral("https://a.example/"), QStringLiteral("A"));
    model.visit(QStringLiteral("https://b.example/"));
    QCOMPARE(model.count(), 2);
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(countSpy.count(), 2);
    QCOMPARE(role(model, 0, HistoryModel::UrlRole).toString(),
             QStringLiteral("https://b.example/"));
    QCOMPARE(role(model, 0, HistoryModel::TitleRole).toString(),
             QStringLiteral("https://b.example/"));
    QCOMPARE(role(model, 1, HistoryModel::TitleRole).toString(), QStringLiteral("A"));
    QCOMPARE(role(model, 1, HistoryModel::VisitCountRole).toInt(), 1);
    QVERIFY(role(model, 1, HistoryModel::DateRole).toDateTime().isValid());
    QVERIFY(!role(model, 1, Qt::DisplayRole).isValid());
    QVERIFY(!role(model, 5, HistoryModel::UrlRole).isValid());
    QCOMPARE(model.roleNames().value(HistoryModel::VisitCountRole),
             QByteArrayLiteral("visitCount"));

    model.visit(QStringLiteral("https://a.example/"));
    QCOMPARE(model.count(), 2);
    QCOMPARE(role(model, 0, HistoryModel::UrlRole).toString(),
             QStringLiteral("https://a.example/"));
    QCOMPARE(role(model, 0, HistoryModel::VisitCountRole).toInt(), 2);
    QCOMPARE(role(model, 0, HistoryModel::TitleRole).toString(), QStringLiteral("A"));

    model.visit(QStringLiteral("https://a.example/"), QStringLiteral("A again"));
    QCOMPARE(role(model, 0, HistoryModel::TitleRole).toString(), QStringLiteral("A again"));
    QCOMPARE(role(model, 0, HistoryModel::VisitCountRole).toInt(), 3);
}

void tst_historymodel::ignoresUnrecordableUrls()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    HistoryModel model(storage);
    model.visit(QString());
    model.visit(QStringLiteral("about:blank"));
    model.updateTitle(QStringLiteral("about:blank"), QStringLiteral("Blank"));
    QCOMPARE(model.count(), 0);
}

void tst_historymodel::search()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    HistoryModel model(storage);
    model.visit(QStringLiteral("https://sailfishos.org/"), QStringLiteral("Sailfish OS"));
    model.visit(QStringLiteral("https://example.org/"), QStringLiteral("Example"));
    model.visit(QStringLiteral("https://duckduckgo.com/?q=sailfish"), QStringLiteral("Search"));

    QSignalSpy termSpy(&model, &HistoryModel::searchTermChanged);
    model.setSearchTerm(QStringLiteral("sailfish"));
    QCOMPARE(termSpy.count(), 1);
    QCOMPARE(model.searchTerm(), QStringLiteral("sailfish"));
    QCOMPARE(model.count(), 2);
    model.setSearchTerm(QStringLiteral("sailfish"));
    QCOMPARE(termSpy.count(), 1);

    model.setSearchTerm(QStringLiteral("EXAMPLE"));
    QCOMPARE(model.count(), 1);
    QCOMPARE(role(model, 0, HistoryModel::TitleRole).toString(), QStringLiteral("Example"));

    model.visit(QStringLiteral("https://another.example/"));
    QCOMPARE(model.count(), 2);

    model.setSearchTerm(QString());
    QCOMPARE(model.count(), 4);
}

void tst_historymodel::titles()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    HistoryModel model(storage);
    model.visit(QStringLiteral("https://a.example/"));
    model.visit(QStringLiteral("https://b.example/"));
    QSignalSpy rowSpy(&model, &HistoryModel::dataChanged);

    model.updateTitle(QStringLiteral("https://a.example/"), QStringLiteral("Alpha"));
    QCOMPARE(rowSpy.count(), 1);
    QCOMPARE(role(model, 1, HistoryModel::TitleRole).toString(), QStringLiteral("Alpha"));
    model.updateTitle(QStringLiteral("https://a.example/"), QStringLiteral("Alpha"));
    model.updateTitle(QStringLiteral("https://a.example/"), QString());
    model.updateTitle(QStringLiteral("https://missing.example/"), QStringLiteral("Nope"));
    QCOMPARE(rowSpy.count(), 1);

    HistoryModel reloaded(storage);
    QCOMPARE(role(reloaded, 1, HistoryModel::TitleRole).toString(), QStringLiteral("Alpha"));
}

void tst_historymodel::removeAndClear()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    HistoryModel model(storage);
    model.visit(QStringLiteral("https://a.example/"));
    model.visit(QStringLiteral("https://b.example/"));
    model.visit(QStringLiteral("https://c.example/"));
    QSignalSpy countSpy(&model, &HistoryModel::countChanged);

    model.remove(1);
    QCOMPARE(model.count(), 2);
    QCOMPARE(countSpy.count(), 1);
    QCOMPARE(role(model, 1, HistoryModel::UrlRole).toString(),
             QStringLiteral("https://a.example/"));
    QCOMPARE(rowsInDatabase(storage), 2);

    model.remove(-1);
    model.remove(2);
    QCOMPARE(model.count(), 2);

    model.clear();
    QCOMPARE(model.count(), 0);
    QCOMPARE(rowsInDatabase(storage), 0);
}

void tst_historymodel::prunesAndLimits()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    {
        QSqlQuery query(storage.database());
        QVERIFY(query.exec(QStringLiteral("BEGIN")));
        for (int i = 0; i < HistoryModel::MaxEntries + 25; ++i) {
            QSqlQuery insert(storage.database());
            insert.prepare(
                QStringLiteral("INSERT INTO browser_history (url, title, date) VALUES (?, '', ?)"));
            insert.addBindValue(QStringLiteral("https://site%1.example/").arg(i));
            insert.addBindValue(i);
            QVERIFY(insert.exec());
        }
        QVERIFY(query.exec(QStringLiteral("COMMIT")));
    }
    HistoryModel model(storage);
    QCOMPARE(rowsInDatabase(storage), HistoryModel::MaxEntries);
    QCOMPARE(model.count(), HistoryModel::DisplayLimit);
    // Newest first, oldest pruned.
    QCOMPARE(role(model, 0, HistoryModel::UrlRole).toString(),
             QStringLiteral("https://site%1.example/").arg(HistoryModel::MaxEntries + 24));
}

// The address bar searches every row, not the model's page of them: past the display
// limit, and whatever the model's own search term.
void tst_historymodel::wholeTable()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    {
        QSqlQuery query(storage.database());
        QVERIFY(query.exec(QStringLiteral("BEGIN")));
        for (int i = 0; i < HistoryModel::DisplayLimit + 10; ++i) {
            QSqlQuery insert(storage.database());
            insert.prepare(QStringLiteral("INSERT INTO browser_history (url, title, date, "
                                          "visited_count) VALUES (?, ?, ?, ?)"));
            insert.addBindValue(QStringLiteral("https://site%1.example/").arg(i));
            insert.addBindValue(QStringLiteral("Site %1").arg(i));
            insert.addBindValue(1000 + i);
            insert.addBindValue(i % 3 + 1);
            QVERIFY(insert.exec());
        }
        QVERIFY(query.exec(QStringLiteral("COMMIT")));
    }
    HistoryModel model(storage);
    model.setSearchTerm(QStringLiteral("site1"));
    QVERIFY(model.count() < HistoryModel::DisplayLimit);

    const QList<HistoryModel::Entry> entries = model.allEntries();
    QCOMPARE(entries.count(), HistoryModel::DisplayLimit + 10);
    // Newest first, every column.
    const HistoryModel::Entry &newest = entries.first();
    QCOMPARE(newest.url,
             QStringLiteral("https://site%1.example/").arg(HistoryModel::DisplayLimit + 9));
    QCOMPARE(newest.title, QStringLiteral("Site %1").arg(HistoryModel::DisplayLimit + 9));
    QCOMPARE(newest.date.toMSecsSinceEpoch(), qint64(1000 + HistoryModel::DisplayLimit + 9));
    QCOMPARE(newest.visitCount, (HistoryModel::DisplayLimit + 9) % 3 + 1);
    QVERIFY(newest.id > 0);
    QCOMPARE(entries.last().url, QStringLiteral("https://site0.example/"));

    // A visit is there at once; a cleared table has nothing.
    model.visit(QStringLiteral("https://new.example/"), QStringLiteral("New"));
    QCOMPARE(model.allEntries().first().url, QStringLiteral("https://new.example/"));
    QCOMPARE(model.allEntries().first().visitCount, 1);
    model.clear();
    QVERIFY(model.allEntries().isEmpty());
}

// Clearing from a time on: the pages last visited then or since go, and what was
// learnt then or since; nothing, or less, is everything.
void tst_historymodel::clearSince()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 minute = qint64(60) * 1000;
    addHistory(storage, QStringLiteral("https://old.example/"), now - 600 * minute);
    addHistory(storage, QStringLiteral("https://new.example/"), now - minute);
    HistoryModel model(storage);
    QCOMPARE(model.count(), 2);
    model.recordInput(QStringLiteral("new"), QStringLiteral("https://new.example/"));
    QCOMPARE(inputsInDatabase(storage), 1);

    model.clearSince(double(now - 60 * minute));
    QCOMPARE(model.count(), 1);
    QCOMPARE(role(model, 0, HistoryModel::UrlRole).toString(),
             QStringLiteral("https://old.example/"));
    QCOMPARE(inputsInDatabase(storage), 0);

    model.recordInput(QStringLiteral("old"), QStringLiteral("https://old.example/"));
    model.clearSince(-1);
    QCOMPARE(model.count(), 0);
    QCOMPARE(rowsInDatabase(storage), 0);
    QCOMPARE(inputsInDatabase(storage), 0);
}

// Firefox's time ranges, from now; today from local midnight; everything, and anything
// else, from the beginning.
void tst_historymodel::rangeStart()
{
    const QDateTime now(QDate(2026, 9, 25), QTime(14, 30));
    const qint64 at = now.toMSecsSinceEpoch();
    const qint64 hour = qint64(60) * 60 * 1000;
    QCOMPARE(HistoryModel::rangeStart(HistoryModel::ClearLastHour, now), at - hour);
    QCOMPARE(HistoryModel::rangeStart(HistoryModel::ClearLastTwoHours, now), at - 2 * hour);
    QCOMPARE(HistoryModel::rangeStart(HistoryModel::ClearLastFourHours, now), at - 4 * hour);
    QCOMPARE(HistoryModel::rangeStart(HistoryModel::ClearToday, now),
             QDateTime(QDate(2026, 9, 25), QTime(0, 0)).toMSecsSinceEpoch());
    QCOMPARE(HistoryModel::rangeStart(HistoryModel::ClearEverything, now), qint64(0));
    QCOMPARE(HistoryModel::rangeStart(-1, now), qint64(0));
    QCOMPARE(HistoryModel::rangeStart(99, now), qint64(0));
    // From now, as QML asks for it.
    const double hourAgo = HistoryModel::rangeStart(HistoryModel::ClearLastHour);
    QVERIFY(qAbs(hourAgo - double(QDateTime::currentMSecsSinceEpoch() - hour)) < 60 * 1000);
    QCOMPARE(HistoryModel::rangeStart(HistoryModel::ClearEverything), 0.0);
}

// Firefox's input history: the text, trimmed and in lower case, and the page; each
// choice again counts one over nine tenths of what was counted; text that begins with
// what is typed leads there, the text itself twice as strongly.
void tst_historymodel::learnsWhatWasTyped()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    HistoryModel model(storage);
    const QString hub = QStringLiteral("https://github.com/");
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    QVERIFY(model.inputRanks(QStringLiteral("gh"), now).isEmpty());

    model.recordInput(QStringLiteral("  GH "), hub);
    QCOMPARE(model.inputRanks(QStringLiteral("gh"), now).value(hub), 2.0);
    QCOMPARE(model.inputRanks(QStringLiteral("G"), now).value(hub), 1.0);
    QVERIFY(model.inputRanks(QStringLiteral("ghx"), now).isEmpty());
    QVERIFY(model.inputRanks(QStringLiteral("  "), now).isEmpty());

    model.recordInput(QStringLiteral("gh"), hub);
    QCOMPARE(model.inputRanks(QStringLiteral("gh"), now).value(hub), 2 * 1.9);
    // Of two texts leading to one page, the stronger.
    model.recordInput(QStringLiteral("g"), hub);
    QCOMPARE(model.inputRanks(QStringLiteral("g"), now).value(hub), 2.0);
    QCOMPARE(inputsInDatabase(storage), 2);

    // Nothing, and nothing the history would keep, is not learnt.
    model.recordInput(QString(), hub);
    model.recordInput(QStringLiteral("about"), QStringLiteral("about:blank"));
    QCOMPARE(inputsInDatabase(storage), 2);
}

// Each day since a text was last chosen wears its count by a fortieth, and only the
// most recently chosen are kept.
void tst_historymodel::learningWearsDown()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    const QString url = QStringLiteral("https://a.example/");
    {
        HistoryModel model(storage);
        model.recordInput(QStringLiteral("a"), url);
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        const qint64 later = now + 10 * qint64(24) * 60 * 60 * 1000;
        QVERIFY(qAbs(model.inputRanks(QStringLiteral("a"), later).value(url) -
                     2 * std::pow(0.975, 10)) < 1e-6);
        // A choice stamped ahead of the clock has not worn at all.
        QCOMPARE(model.inputRanks(QStringLiteral("a"), now - 1000).value(url), 2.0);

        QSqlQuery insert(storage.database());
        insert.prepare(QStringLiteral("INSERT INTO input_history (input, url, use_count, used) "
                                      "VALUES (?, ?, 1, ?)"));
        for (int i = 0; i < HistoryModel::MaxInputs + 5; ++i) {
            insert.addBindValue(QStringLiteral("old %1").arg(i));
            insert.addBindValue(url);
            insert.addBindValue(i);
            QVERIFY(insert.exec());
        }
    }
    HistoryModel reopened(storage);
    QCOMPARE(inputsInDatabase(storage), int(HistoryModel::MaxInputs));
    // The newest stays: the one learnt just now.
    QVERIFY(reopened.inputRanks(QStringLiteral("a"), QDateTime::currentMSecsSinceEpoch())
                .contains(url));
    QVERIFY(reopened.inputRanks(QStringLiteral("old 0"), QDateTime::currentMSecsSinceEpoch())
                .isEmpty());
}

// A page removed takes with it what was learnt leads there; the rest stays.
void tst_historymodel::learningGoesWithThePage()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    HistoryModel model(storage);
    model.visit(QStringLiteral("https://a.example/"));
    model.visit(QStringLiteral("https://b.example/"));
    model.recordInput(QStringLiteral("x"), QStringLiteral("https://a.example/"));
    model.recordInput(QStringLiteral("x"), QStringLiteral("https://b.example/"));
    QCOMPARE(role(model, 0, HistoryModel::UrlRole).toString(),
             QStringLiteral("https://b.example/"));
    model.remove(0);
    const QHash<QString, double> ranks =
        model.inputRanks(QStringLiteral("x"), QDateTime::currentMSecsSinceEpoch());
    QCOMPARE(ranks.keys(), QList<QString>{QStringLiteral("https://a.example/")});
    model.clear();
    QCOMPARE(inputsInDatabase(storage), 0);
}

QTEST_GUILESS_MAIN(tst_historymodel)
#include "tst_historymodel.moc"
