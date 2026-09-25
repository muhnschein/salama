// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "Storage.h"

#include <QCoreApplication>
#include <QDir>
#include <QPair>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QStringList>
#include <QUuid>
#include <QtDebug>

namespace Salama {

namespace {

const char *const DatabaseFileName = "salama.sqlite";

// The tab and group tables by the name to create them under: the schema and the
// rebuild that drops a column from an older table both need them.
QString tabTable(const QString &name)
{
    return QStringLiteral("CREATE TABLE IF NOT EXISTS %1 ("
                          "tab_id INTEGER PRIMARY KEY, "
                          "position INTEGER NOT NULL, "
                          "url TEXT NOT NULL, "
                          "title TEXT NOT NULL DEFAULT '', "
                          "favicon TEXT NOT NULL DEFAULT '', "
                          "thumbnail TEXT NOT NULL DEFAULT '', "
                          "last_active INTEGER NOT NULL DEFAULT 0, "
                          "group_id INTEGER NOT NULL DEFAULT 1)")
        .arg(name);
}

QString groupTable(const QString &name)
{
    return QStringLiteral("CREATE TABLE IF NOT EXISTS %1 ("
                          "group_id INTEGER PRIMARY KEY, "
                          "name TEXT NOT NULL DEFAULT '', "
                          "position INTEGER NOT NULL)")
        .arg(name);
}

const QStringList &schemaStatements()
{
    static const QStringList statements{
        tabTable(QStringLiteral("tab")),
        groupTable(QStringLiteral("tab_group")),
        QStringLiteral("CREATE TABLE IF NOT EXISTS closed_tab ("
                       "id INTEGER PRIMARY KEY, "
                       "url TEXT NOT NULL, "
                       "title TEXT NOT NULL DEFAULT '', "
                       "favicon TEXT NOT NULL DEFAULT '', "
                       "closed INTEGER NOT NULL)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS browser_history ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                       "url TEXT NOT NULL UNIQUE, "
                       "title TEXT NOT NULL DEFAULT '', "
                       "visited_count INTEGER NOT NULL DEFAULT 1, "
                       "date INTEGER NOT NULL, "
                       "favicon TEXT NOT NULL DEFAULT '')"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS browser_history_date ON browser_history(date)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS bookmark ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                       "url TEXT NOT NULL, "
                       "title TEXT NOT NULL DEFAULT '', "
                       "favicon TEXT NOT NULL DEFAULT '', "
                       "position INTEGER NOT NULL, "
                       "created INTEGER NOT NULL)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS setting ("
                       "name TEXT PRIMARY KEY, "
                       "value TEXT NOT NULL)"),
        // Schema 7: the browser's own list of downloads (src/downloads/DownloadModel.h).
        // An older database gains it here as a new one does; a whole new table needs
        // none of the column work below.
        QStringLiteral("CREATE TABLE IF NOT EXISTS download ("
                       "id INTEGER PRIMARY KEY, "
                       "name TEXT NOT NULL DEFAULT '', "
                       "url TEXT NOT NULL DEFAULT '', "
                       "path TEXT NOT NULL DEFAULT '', "
                       "mime TEXT NOT NULL DEFAULT '', "
                       "size INTEGER NOT NULL DEFAULT 0, "
                       "status INTEGER NOT NULL, "
                       "started INTEGER NOT NULL)"),
        // Schema 8: what was typed into the address bar before a page was chosen from
        // what it found, and how often (src/history/HistoryModel.h). A new table again.
        QStringLiteral("CREATE TABLE IF NOT EXISTS input_history ("
                       "input TEXT NOT NULL, "
                       "url TEXT NOT NULL, "
                       "use_count REAL NOT NULL, "
                       "used INTEGER NOT NULL, "
                       "PRIMARY KEY (input, url))"),
    };
    return statements;
}

} // namespace

Storage::Storage(const QString &dataDirectory)
    : m_connectionName(QStringLiteral("salama-") + QUuid::createUuid().toString())
{
    QDir dir(dataDirectory);
    if (dataDirectory.isEmpty() || (!dir.exists() && !dir.mkpath(QStringLiteral(".")))) {
        qWarning() << "Storage: cannot create data directory" << dataDirectory;
        return;
    }

    m_databasePath = dir.absoluteFilePath(QLatin1String(DatabaseFileName));
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    db.setDatabaseName(m_databasePath);
    if (!db.open()) {
        qWarning() << "Storage: cannot open" << m_databasePath << db.lastError().text();
        return;
    }

    if (!applySchema()) {
        qWarning() << "Storage: schema setup failed for" << m_databasePath;
        db.close();
    }
}

Storage::~Storage()
{
    {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
        if (db.isValid()) {
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(m_connectionName);
}

bool Storage::isOpen() const
{
    return QSqlDatabase::database(m_connectionName, false).isOpen();
}

QSqlDatabase Storage::database() const
{
    return QSqlDatabase::database(m_connectionName, false);
}

QString Storage::databasePath() const
{
    return m_databasePath;
}

int Storage::userVersion() const
{
    QSqlQuery query(database());
    if (query.exec(QStringLiteral("PRAGMA user_version")) && query.next()) {
        return query.value(0).toInt();
    }
    return -1;
}

QString Storage::defaultDataDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

QString Storage::defaultCacheDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
}

QString Storage::defaultDownloadDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) +
           QStringLiteral("/Salama");
}

QString Storage::defaultConfigFilePath()
{
    // Sandboxed apps must not use the default QSettings path; this is the layout
    // recommended by sailjail-permissions/README.md.
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QLatin1Char('/') +
           QCoreApplication::applicationName() + QStringLiteral(".conf");
}

QVariant Storage::text(const QString &value)
{
    return value.isNull() ? QVariant(QStringLiteral("")) : QVariant(value);
}

bool Storage::hasColumn(const QString &table, const QString &column) const
{
    QSqlQuery query(database());
    if (!query.exec(QStringLiteral("PRAGMA table_info(%1)").arg(table))) {
        return false;
    }
    while (query.next()) {
        if (query.value(1).toString() == column) {
            return true;
        }
    }
    return false;
}

bool Storage::applySchema() const
{
    QSqlDatabase db = database();
    const int version = userVersion();
    if (version == SchemaVersion) {
        return true;
    }
    if (version > SchemaVersion) {
        qWarning() << "Storage: database is newer than this build:" << version;
        return false;
    }

    if (!db.transaction()) {
        return false;
    }
    for (const QString &statement : schemaStatements()) {
        QSqlQuery query(db);
        if (!query.exec(statement)) {
            qWarning() << "Storage:" << query.lastError().text();
            db.rollback();
            return false;
        }
    }

    // Schema 1 predates tab previews, schema 2 the cover's order of tabs, schema 3
    // tab groups and schema 9 the history's icons. CREATE TABLE IF NOT EXISTS above
    // leaves an existing table alone, so the columns are added here; asking the table
    // rather than the version number makes this correct whichever way the database was
    // created. Every tab from before schema 4 lands in group 1, which TabModel creates
    // when no group row claims the id.
    struct Column
    {
        const char *table;
        const char *name;
        const char *definition;
    };
    const QList<Column> columns{
        {"tab", "thumbnail", "TEXT NOT NULL DEFAULT ''"},
        {"tab", "last_active", "INTEGER NOT NULL DEFAULT 0"},
        {"tab", "group_id", "INTEGER NOT NULL DEFAULT 1"},
        // Schema 9: the icon a page of the history loaded with, for the address bar.
        {"browser_history", "favicon", "TEXT NOT NULL DEFAULT ''"},
    };
    for (const Column &column : columns) {
        if (hasColumn(QLatin1String(column.table), QLatin1String(column.name))) {
            continue;
        }
        QSqlQuery query(db);
        if (!query.exec(QStringLiteral("ALTER TABLE %1 ADD COLUMN %2 %3")
                            .arg(QLatin1String(column.table), QLatin1String(column.name),
                                 QLatin1String(column.definition)))) {
            qWarning() << "Storage:" << query.lastError().text();
            db.rollback();
            return false;
        }
    }
    // Schema 5 kept private tabs, as a flag on the tab and on the group; schema 6
    // does not. A table that still carries the column loses its private rows and is
    // rebuilt without it -- copied, because SQLite before 3.35 cannot drop a column.
    // The ids are kept, so the tabs still name their groups.
    struct Rebuild
    {
        const char *table;
        const char *kept;
        QString (*create)(const QString &);
    };
    const QList<Rebuild> rebuilds{
        {"tab", "tab_id, position, url, title, favicon, thumbnail, last_active, group_id",
         &tabTable},
        {"tab_group", "group_id, name, position", &groupTable},
    };
    for (const Rebuild &rebuild : rebuilds) {
        const QString table = QLatin1String(rebuild.table);
        if (!hasColumn(table, QStringLiteral("private"))) {
            continue;
        }
        const QString fresh = table + QStringLiteral("_rebuilt");
        const QString kept = QLatin1String(rebuild.kept);
        const QStringList steps{
            QStringLiteral("DELETE FROM %1 WHERE private = 1").arg(table),
            rebuild.create(fresh),
            QStringLiteral("INSERT INTO %1 (%2) SELECT %2 FROM %3").arg(fresh, kept, table),
            QStringLiteral("DROP TABLE %1").arg(table),
            QStringLiteral("ALTER TABLE %1 RENAME TO %2").arg(fresh, table),
        };
        for (const QString &step : steps) {
            QSqlQuery query(db);
            if (!query.exec(step)) {
                qWarning() << "Storage:" << query.lastError().text();
                db.rollback();
                return false;
            }
        }
    }
    QSqlQuery pragma(db);
    if (!pragma.exec(QStringLiteral("PRAGMA user_version = %1").arg(SchemaVersion))) {
        db.rollback();
        return false;
    }
    return db.commit();
}

} // namespace Salama
