// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QVariant>

namespace Salama {

// One per process; models borrow connection. Bump SchemaVersion on schema change.
class Storage
{
public:
    static const int SchemaVersion = 9;

    explicit Storage(const QString &dataDirectory);
    ~Storage();

    Storage(const Storage &) = delete;
    Storage &operator=(const Storage &) = delete;

    bool isOpen() const;
    QSqlDatabase database() const;
    QString databasePath() const;
    int userVersion() const;

    // Sailjail allows writes only below these; see docs/HARBOUR.md.
    static QString defaultDataDirectory();
    static QString defaultConfigFilePath();
    static QString defaultCacheDirectory();
    static QString defaultDownloadDirectory();

    // For TEXT NOT NULL: null QString binds SQL NULL.
    static QVariant text(const QString &value);

private:
    bool applySchema() const;
    bool hasColumn(const QString &table, const QString &column) const;

    QString m_connectionName;
    QString m_databasePath;
};

} // namespace Salama
