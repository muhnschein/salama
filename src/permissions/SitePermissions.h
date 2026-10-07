// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "ModelRoles.h"

#include <QAbstractListModel>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVector>

namespace Salama {

// Engine names for Kind; Location has two (see .cpp). Empty for invalid kind.
QStringList permissionTypesOf(int kind);
int permissionKindOf(const QString &type);

// Per-site exceptions to SitePermissionSettings defaults, in engine permission manager.
class SitePermissions : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int exceptionSiteCount READ exceptionSiteCount NOTIFY changed)
    // Bumps on change so bindings re-call invokables:
    // `(SitePermissions.revision, SitePermissions.decision(kind, origin))`.
    Q_PROPERTY(int revision READ revision NOTIFY changed)
    Q_PROPERTY(QString topic READ topic CONSTANT)

public:
    enum Kind // NOSONAR(cpp:S3642) QML on Qt 5.6 reads no scoped enum
    {
        Notifications = 0,
        Popups,
        Cookies,
        Location,
        Camera,
        Microphone,
        // Allow = protection off (Firefox allow list). No Block.
        TrackingProtection
    };
    Q_ENUM(Kind)

    enum Decision // NOSONAR(cpp:S3642) QML on Qt 5.6 reads no scoped enum
    {
        Default = 0,
        Allow = 1,
        Block = 2,
        // Only if canAsk(). Values = engine capabilities.
        Ask = 3
    };
    Q_ENUM(Decision)

    enum class Role
    {
        Kind = Qt::UserRole + 1,
        Origin,
        Host,
        Allowed,
        Decision
    };

    explicit SitePermissions(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int exceptionSiteCount() const;
    int revision() const;
    QString topic() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void observe(const QString &topic, const QVariant &data);

    Q_INVOKABLE int decision(int kind, const QString &origin) const;
    Q_INVOKABLE int count(int kind) const;
    Q_INVOKABLE int originCount(const QString &origin) const;

    Q_INVOKABLE void set(int kind, const QString &origin, int decision);
    Q_INVOKABLE void remove(int kind, const QString &origin);
    Q_INVOKABLE void removeAll(int kind);
    Q_INVOKABLE void removeAllForOrigin(const QString &origin);

    // False for popups, cookies: pages don't request those.
    Q_INVOKABLE static bool canAsk(int kind);
    Q_INVOKABLE static QString originOf(const QString &url);

    // Engine already told: no engine message, no decided().
    void adopt(int kind, const QString &origin, int decision);

signals:
    void changed();
    void decided(int kind, const QString &origin, int decision);
    void engineRequest(const QString &topic, const QVariant &payload);

private:
    struct Exception
    {
        int kind = 0;
        QString origin;
        int decision = Block;
    };

    int rowOf(int kind, const QString &origin) const;
    bool put(int kind, const QString &origin, int decision);
    bool take(int kind, const QString &origin);
    void send(const QString &message, int kind, const QString &origin, int capability);
    void touch();

    QVector<Exception> m_exceptions;
    int m_revision = 0;
};

} // namespace Salama
