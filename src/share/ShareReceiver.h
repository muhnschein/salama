// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantMap>

namespace Salama {

// Sailfish.Share ShareProvider can't parse link shares (url in status): answered here,
// as harbour-nextmarks src/sharereceiver.cpp does.
class ShareReceiver : public QObject
{
    Q_OBJECT

public:
    explicit ShareReceiver(QObject *parent = nullptr);

    // False if no session bus (host tests) or name taken.
    bool registerService();

    Q_INVOKABLE void setReady();

    void receive(const QVariantMap &arguments);

    // Empty unless http(s) with host.
    static QString sharedUrl(const QVariantMap &arguments);

    // Qt D-Bus leaves nested a{sv} containers as QDBusArgument; unwraps one level.
    static QVariant unwrap(const QVariant &value);

signals:
    void linkShared(const QString &url);

private:
    bool m_ready = false;
    QString m_pending;
};

} // namespace Salama
