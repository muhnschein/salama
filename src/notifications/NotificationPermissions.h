// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "ModelRoles.h"

#include <QAbstractListModel>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QVector>

namespace Salama {

// In engine permission manager, so page's `Notification.permission` sees it.
// Permanent decisions only; session ones are platform's.
class NotificationPermissions : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(int allowedCount READ allowedCount NOTIFY sitesChanged)
    Q_PROPERTY(int blockedCount READ blockedCount NOTIFY sitesChanged)
    Q_PROPERTY(QString topic READ topic CONSTANT)

public:
    enum class Role
    {
        Origin = Qt::UserRole + 1,
        Host,
        Allowed
    };

    explicit NotificationPermissions(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString topic() const;
    int allowedCount() const;
    int blockedCount() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void observe(const QString &topic, const QVariant &data);

    Q_INVOKABLE void setAllowed(const QString &origin, bool allowed);
    Q_INVOKABLE void remove(const QString &origin);

    // Allowed pages not slept out of sight: asleep page sends nothing.
    Q_INVOKABLE bool isAllowed(const QString &url) const;
    bool isBlocked(const QString &origin) const;

    // {name, value} for WebEngineSettings.setPreference(): 2 = deny, 0 = ask.
    Q_INVOKABLE static QVariantMap defaultPreference(bool blockRequests);

    // Engine already told: no engine message, no decided(). 1 allow, 2 block, else forget.
    void adopt(const QString &origin, int capability);

    // Platform PopupOpener.qml denies non-location permissions for session; site asking
    // before our UI loaded stays denied unasked. Undo unless permanently decided.
    void undoAutomaticDenial(const QString &origin);

    // "https://host[:port]", ASCII host as Gecko principal. Empty for non-http(s).
    static QString originOf(const QString &url);
    static QString hostOf(const QString &origin);

signals:
    void countChanged();
    void sitesChanged();
    // 1 allow, 2 block, 0 forget.
    void decided(const QString &origin, int capability);
    void engineRequest(const QString &topic, const QVariant &payload);

private:
    struct Site
    {
        QString origin;
        bool allowed = false;
    };

    int rowOf(const QString &origin) const;
    int positionOf(const QString &origin, bool allowed) const;
    void put(const QString &origin, bool allowed);
    void take(int row);
    void send(const QString &message, const QString &origin, int capability);

    QVector<Site> m_sites;
};

} // namespace Salama
