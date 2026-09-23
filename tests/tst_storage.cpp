// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "storage/Storage.h"

#include <QDir>
#include <QSqlQuery>
#include <QStringList>
#include <QTemporaryDir>
#include <QtTest>

using Salama::Storage;

class tst_storage : public QObject
{
    Q_OBJECT

private slots:
    void createsSchema();
    void reopenKeepsData();
    void refusesUnusableDirectory();
    void refusesNewerSchema();
    void migratesSchemaOne();
    void dropsPrivateTabsFromSchemaFive();
    void addsDownloadsToSchemaSix();
    void defaultPaths();
};

namespace {

QStringList tableNames(const Storage &storage)
{
    QStringList names;
    QSqlQuery query(storage.database());
    query.exec(QStringLiteral("SELECT name FROM sqlite_master WHERE type = 'table' ORDER BY name"));
    while (query.next()) {
        names.append(query.value(0).toString());
    }
    return names;
}

} // namespace

void tst_storage::createsSchema()
{
    QTemporaryDir dir;
    Storage storage(dir.path() + QStringLiteral("/nested/data"));
    QVERIFY(storage.isOpen());
    QCOMPARE(storage.userVersion(), Storage::SchemaVersion);
    QVERIFY(storage.databasePath().endsWith(QStringLiteral("salama.sqlite")));

    const QStringList tables = tableNames(storage);
    QVERIFY(tables.contains(QStringLiteral("tab")));
    QVERIFY(tables.contains(QStringLiteral("browser_history")));
    QVERIFY(tables.contains(QStringLiteral("bookmark")));
    QVERIFY(tables.contains(QStringLiteral("setting")));
    QVERIFY(tables.contains(QStringLiteral("download")));
}

void tst_storage::reopenKeepsData()
{
    QTemporaryDir dir;
    {
        Storage storage(dir.path());
        QSqlQuery query(storage.database());
        QVERIFY(query.exec(
            QStringLiteral("INSERT INTO setting (name, value) VALUES ('probe', 'kept')")));
    }
    Storage storage(dir.path());
    QVERIFY(storage.isOpen());
    QCOMPARE(storage.userVersion(), Storage::SchemaVersion);
    QSqlQuery query(storage.database());
    QVERIFY(query.exec(QStringLiteral("SELECT value FROM setting WHERE name = 'probe'")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), QStringLiteral("kept"));
}

void tst_storage::refusesUnusableDirectory()
{
    const QString none;
    Storage empty(none);
    QVERIFY(!empty.isOpen());
    QVERIFY(!empty.database().isValid());

    QTemporaryDir dir;
    QFile blocker(dir.path() + QStringLiteral("/file"));
    QVERIFY(blocker.open(QIODevice::WriteOnly));
    blocker.close();
    Storage underFile(dir.path() + QStringLiteral("/file/data"));
    QVERIFY(!underFile.isOpen());
}

void tst_storage::refusesNewerSchema()
{
    QTemporaryDir dir;
    {
        Storage storage(dir.path());
        QSqlQuery query(storage.database());
        QVERIFY(query.exec(QStringLiteral("PRAGMA user_version = 99")));
    }
    Storage storage(dir.path());
    QVERIFY(!storage.isOpen());
}

void tst_storage::migratesSchemaOne()
{
    QTemporaryDir dir;
    const QString path = QDir(dir.path()).absoluteFilePath(QStringLiteral("salama.sqlite"));
    {
        // A schema 1 database: the tab table has none of the columns later schemas
        // added -- thumbnail (2), last_active (3), group_id (4) -- and neither the
        // group table nor the closed-tab table.
        QSqlDatabase db =
            QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("legacy"));
        db.setDatabaseName(path);
        QVERIFY(db.open());
        QSqlQuery query(db);
        QVERIFY(query.exec(QStringLiteral("CREATE TABLE tab (tab_id INTEGER PRIMARY KEY, "
                                          "position INTEGER NOT NULL, url TEXT NOT NULL, "
                                          "title TEXT NOT NULL DEFAULT '', "
                                          "favicon TEXT NOT NULL DEFAULT '')")));
        QVERIFY(query.exec(QStringLiteral("INSERT INTO tab (tab_id, position, url, title) "
                                          "VALUES (1, 1, 'https://a.example/', 'A')")));
        QVERIFY(query.exec(QStringLiteral("PRAGMA user_version = 1")));
        db.close();
    }
    QSqlDatabase::removeDatabase(QStringLiteral("legacy"));

    Storage storage(dir.path());
    QVERIFY(storage.isOpen());
    QCOMPARE(storage.userVersion(), Storage::SchemaVersion);

    QSqlQuery query(storage.database());
    QVERIFY(query.exec(
        QStringLiteral("SELECT tab_id, title, thumbnail, last_active, group_id FROM tab")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 1);
    QCOMPARE(query.value(1).toString(), QStringLiteral("A"));
    QVERIFY(query.value(2).toString().isEmpty());
    // Never in front as far as the database knows; the model stamps the restored tab.
    QCOMPARE(query.value(3).toLongLong(), 0LL);
    // In group 1, which the model creates when it finds no row for it.
    QCOMPARE(query.value(4).toInt(), 1);
    QVERIFY(query.exec(QStringLiteral("SELECT COUNT(*) FROM tab_group")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 0);
    QVERIFY(query.exec(QStringLiteral("SELECT COUNT(*) FROM closed_tab")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 0);

    // Reopening an already migrated database changes nothing.
    Storage again(dir.path());
    QVERIFY(again.isOpen());
    QCOMPARE(again.userVersion(), Storage::SchemaVersion);
}

// Schema 5 flagged private tabs and a private group. Schema 6 has neither: the
// flagged rows go, and the tables are rebuilt without the column, ids and the rest
// of the rows intact.
void tst_storage::dropsPrivateTabsFromSchemaFive()
{
    QTemporaryDir dir;
    const QString path = QDir(dir.path()).absoluteFilePath(QStringLiteral("salama.sqlite"));
    {
        QSqlDatabase db =
            QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("five"));
        db.setDatabaseName(path);
        QVERIFY(db.open());
        QSqlQuery query(db);
        QVERIFY(query.exec(QStringLiteral(
            "CREATE TABLE tab (tab_id INTEGER PRIMARY KEY, position INTEGER NOT NULL, "
            "url TEXT NOT NULL, title TEXT NOT NULL DEFAULT '', "
            "favicon TEXT NOT NULL DEFAULT '', thumbnail TEXT NOT NULL DEFAULT '', "
            "last_active INTEGER NOT NULL DEFAULT 0, group_id INTEGER NOT NULL DEFAULT 1, "
            "private INTEGER NOT NULL DEFAULT 0)")));
        QVERIFY(
            query.exec(QStringLiteral("CREATE TABLE tab_group (group_id INTEGER PRIMARY KEY, "
                                      "name TEXT NOT NULL DEFAULT '', position INTEGER NOT NULL, "
                                      "private INTEGER NOT NULL DEFAULT 0)")));
        QVERIFY(query.exec(QStringLiteral("INSERT INTO tab_group VALUES (2, '', 1, 1)")));
        QVERIFY(query.exec(QStringLiteral("INSERT INTO tab_group VALUES (1, '', 2, 0)")));
        QVERIFY(query.exec(QStringLiteral("INSERT INTO tab_group VALUES (3, 'Work', 3, 0)")));
        QVERIFY(query.exec(
            QStringLiteral("INSERT INTO tab (tab_id, position, url, title, group_id, private) "
                           "VALUES (1, 1, 'https://a.example/', 'A', 1, 0), "
                           "(2, 2, 'https://secret.example/', 'S', 2, 1), "
                           "(3, 3, 'https://w.example/', 'W', 3, 0)")));
        QVERIFY(query.exec(QStringLiteral("PRAGMA user_version = 5")));
        db.close();
    }
    QSqlDatabase::removeDatabase(QStringLiteral("five"));

    Storage storage(dir.path());
    QVERIFY(storage.isOpen());
    QCOMPARE(storage.userVersion(), Storage::SchemaVersion);
    QSqlQuery query(storage.database());
    auto columns = [&query](const QString &table) {
        QStringList names;
        if (query.exec(QStringLiteral("PRAGMA table_info(%1)").arg(table))) {
            while (query.next()) {
                names.append(query.value(1).toString());
            }
        }
        return names;
    };
    QVERIFY(!columns(QStringLiteral("tab")).contains(QStringLiteral("private")));
    QVERIFY(columns(QStringLiteral("tab")).contains(QStringLiteral("group_id")));
    QVERIFY(!columns(QStringLiteral("tab_group")).contains(QStringLiteral("private")));

    QVERIFY(
        query.exec(QStringLiteral("SELECT tab_id, title, group_id FROM tab ORDER BY position")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 1);
    QCOMPARE(query.value(1).toString(), QStringLiteral("A"));
    QCOMPARE(query.value(2).toInt(), 1);
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 3);
    QCOMPARE(query.value(2).toInt(), 3);
    QVERIFY(!query.next());
    QVERIFY(query.exec(QStringLiteral("SELECT group_id, name FROM tab_group ORDER BY position")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 1);
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 3);
    QCOMPARE(query.value(1).toString(), QStringLiteral("Work"));
    QVERIFY(!query.next());

    // The rebuilt tables are the schema's own: a new row still gets its defaults.
    QVERIFY(query.exec(QStringLiteral(
        "INSERT INTO tab (tab_id, position, url) VALUES (9, 9, 'https://n.example/')")));
    QVERIFY(query.exec(QStringLiteral("SELECT group_id, title FROM tab WHERE tab_id = 9")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 1);
    QVERIFY(query.value(1).toString().isEmpty());
}

// Schema 7 adds the download table and nothing else. A schema 6 database gains it on
// opening, with every row it already had left where it was.
void tst_storage::addsDownloadsToSchemaSix()
{
    QTemporaryDir dir;
    const QString path = QDir(dir.path()).absoluteFilePath(QStringLiteral("salama.sqlite"));
    {
        QSqlDatabase db =
            QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("six"));
        db.setDatabaseName(path);
        QVERIFY(db.open());
        QSqlQuery query(db);
        const QStringList schemaSix{
            QStringLiteral("CREATE TABLE tab (tab_id INTEGER PRIMARY KEY, "
                           "position INTEGER NOT NULL, url TEXT NOT NULL, "
                           "title TEXT NOT NULL DEFAULT '', favicon TEXT NOT NULL DEFAULT '', "
                           "thumbnail TEXT NOT NULL DEFAULT '', "
                           "last_active INTEGER NOT NULL DEFAULT 0, "
                           "group_id INTEGER NOT NULL DEFAULT 1)"),
            QStringLiteral("CREATE TABLE tab_group (group_id INTEGER PRIMARY KEY, "
                           "name TEXT NOT NULL DEFAULT '', position INTEGER NOT NULL)"),
            QStringLiteral("CREATE TABLE closed_tab (id INTEGER PRIMARY KEY, "
                           "url TEXT NOT NULL, title TEXT NOT NULL DEFAULT '', "
                           "favicon TEXT NOT NULL DEFAULT '', closed INTEGER NOT NULL)"),
            QStringLiteral("CREATE TABLE browser_history (id INTEGER PRIMARY KEY AUTOINCREMENT, "
                           "url TEXT NOT NULL UNIQUE, title TEXT NOT NULL DEFAULT '', "
                           "visited_count INTEGER NOT NULL DEFAULT 1, date INTEGER NOT NULL)"),
            QStringLiteral("CREATE INDEX browser_history_date ON browser_history(date)"),
            QStringLiteral("CREATE TABLE bookmark (id INTEGER PRIMARY KEY AUTOINCREMENT, "
                           "url TEXT NOT NULL, title TEXT NOT NULL DEFAULT '', "
                           "favicon TEXT NOT NULL DEFAULT '', position INTEGER NOT NULL, "
                           "created INTEGER NOT NULL)"),
            QStringLiteral("CREATE TABLE setting (name TEXT PRIMARY KEY, value TEXT NOT NULL)"),
            QStringLiteral("INSERT INTO tab (tab_id, position, url, title) "
                           "VALUES (1, 1, 'https://a.example/', 'A')"),
            QStringLiteral("INSERT INTO browser_history (url, title, date) "
                           "VALUES ('https://a.example/', 'A', 5)"),
            QStringLiteral("PRAGMA user_version = 6"),
        };
        for (const QString &statement : schemaSix) {
            QVERIFY2(query.exec(statement), qPrintable(statement));
        }
        db.close();
    }
    QSqlDatabase::removeDatabase(QStringLiteral("six"));

    Storage storage(dir.path());
    QVERIFY(storage.isOpen());
    QCOMPARE(storage.userVersion(), 7);
    QCOMPARE(Storage::SchemaVersion, 7);
    QVERIFY(tableNames(storage).contains(QStringLiteral("download")));

    QSqlQuery query(storage.database());
    QVERIFY(query.exec(QStringLiteral("SELECT url, title FROM tab")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), QStringLiteral("https://a.example/"));
    QVERIFY(query.exec(QStringLiteral("SELECT title FROM browser_history")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), QStringLiteral("A"));

    // The new table is the schema's own: a row gets its defaults, and a status and a
    // start time are what it cannot do without.
    QVERIFY(query.exec(QStringLiteral("SELECT COUNT(*) FROM download")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 0);
    QVERIFY(
        query.exec(QStringLiteral("INSERT INTO download (id, status, started) VALUES (1, 0, 9)")));
    QVERIFY(query.exec(
        QStringLiteral("SELECT name, url, path, mime, size FROM download WHERE id = 1")));
    QVERIFY(query.next());
    for (int column = 0; column < 4; ++column) {
        QCOMPARE(query.value(column).toString(), QString());
        QVERIFY(!query.value(column).isNull());
    }
    QCOMPARE(query.value(4).toLongLong(), 0LL);
    QVERIFY(!query.exec(QStringLiteral("INSERT INTO download (id, started) VALUES (2, 9)")));
    QVERIFY(!query.exec(QStringLiteral("INSERT INTO download (id, status) VALUES (3, 0)")));

    // And opening it again finds nothing left to do.
    Storage again(dir.path());
    QVERIFY(again.isOpen());
    QCOMPARE(again.userVersion(), 7);
}

void tst_storage::defaultPaths()
{
    QVERIFY(!Storage::defaultDataDirectory().isEmpty());
    QVERIFY(Storage::defaultConfigFilePath().endsWith(QStringLiteral(".conf")));
    QVERIFY(!Storage::defaultCacheDirectory().isEmpty());
}

QTEST_GUILESS_MAIN(tst_storage)
#include "tst_storage.moc"
