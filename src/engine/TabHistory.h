// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QVariant>
#include <QVariantList>

namespace Salama {

class Storage;

// Back hold's list. WebView exposes canGoBack only: frame script reads tab's session history,
// answers askMessage with resultMessage (needs addMessageListener), goes on goMessage
// `{index, address}`.
class TabHistory : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString scriptUrl READ scriptUrl CONSTANT)
    Q_PROPERTY(QString askMessage READ askMessage CONSTANT)
    Q_PROPERTY(QString resultMessage READ resultMessage CONSTANT)
    Q_PROPERTY(QString goMessage READ goMessage CONSTANT)

public:
    // Older ones via History page.
    static constexpr int Limit = 10;

    explicit TabHistory(const Storage &storage, QObject *parent = nullptr);

    QString scriptUrl() const;
    QString script() const;
    QString askMessage() const;
    QString resultMessage() const;
    QString goMessage() const;

    // Back entries newest first, at most Limit: `{index, address, url, title, favicon}`.
    // address = engine's, url = shown (reader view -> article's), favicon from history.
    Q_INVOKABLE QVariantList entries(const QVariant &data) const;

private:
    QString faviconOf(const QString &url) const;

    QSqlDatabase m_db;
};

} // namespace Salama
