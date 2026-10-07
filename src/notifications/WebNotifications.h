// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QVector>

namespace Salama {

class NotificationPermissions;

// Web Notifications done by app: Gecko alerts service has no backend here (XUL fallback
// embedlite can't open). pageScript() replaces page Notification; frame script relays with
// unforgeable origin + permission; QML shows via Nemo.Notifications.
class WebNotifications : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString messageName READ messageName CONSTANT)
    Q_PROPERTY(QString relayScriptUrl READ relayScriptUrl CONSTANT)
    // Returns true once installed.
    Q_PROPERTY(QString pageScript READ pageScript CONSTANT)

public:
    enum Decision // NOSONAR(cpp:S3642) QML on Qt 5.6 reads no scoped enum
    {
        Allow = 0,
        Block = 1,
        NotNow = 2
    };
    Q_ENUM(Decision)

    // Title/body cut at TextLimit chars. Message over MessageLimit bytes (icon-sized) dropped.
    static constexpr int TextLimit = 1000;
    static constexpr int MessageLimit = 1024 * 1024;
    static constexpr int IconSize = 256;

    WebNotifications(NotificationPermissions *permissions, QString iconDirectory,
                     QObject *parent = nullptr);

    QString messageName() const;
    QString relayScriptUrl() const;
    QString pageScript() const;
    QString relayScript() const;

    // `{origin, permission, detail}`, detail = page JSON. Else ignored.
    Q_INVOKABLE void receive(int tabId, const QVariant &data);

    Q_INVOKABLE void answer(int tabId, int decision);

    // Tapped: raise tab, tell page, then close.
    Q_INVOKABLE void activate(int key);
    Q_INVOKABLE void closed(int key);

    Q_INVOKABLE void forgetTab(int tabId);
    Q_INVOKABLE void closeAll();

    // See NotificationPermissions::undoAutomaticDenial().
    Q_INVOKABLE void popupOpening(const QString &pageUrl, const QString &topic,
                                  const QVariant &data);

    static QString replyScript(const QVariantMap &message);

    // For tests.
    QList<int> keys() const;
    int tabOf(int key) const;

signals:
    void permissionRequested(int tabId, const QString &host);
    void permissionWithdrawn(int tabId);
    // {summary, body, subText, icon}; replaces key's previous.
    void publishRequested(int key, const QVariantMap &fields);
    void closeRequested(int key);
    void pageRequested(int tabId, const QString &script);
    void tabRequested(int tabId);

private:
    struct Shown
    {
        int key = 0;
        int tabId = 0;
        QString page;
        int id = 0;
        QString origin;
        QString tag;
        QString icon;
    };

    struct Request
    {
        int tabId = 0;
        QString page;
        int id = 0;
        QString origin;
    };

    void request(int tabId, const QString &page, int id, const QString &origin);
    void show(int tabId, const QString &page, const QVariantMap &message, const QString &origin,
              bool allowed);
    void close(int tabId, const QString &page, int id);
    void unload(int tabId);
    void reply(int tabId, const QString &page, const QVariantMap &message);
    // Caller closes platform notification.
    Shown take(int key);
    QString saveIcon(int key, const QString &dataUrl);
    void removeIcon(const QString &path) const;

    NotificationPermissions *m_permissions;
    QString m_iconDirectory;
    QHash<int, Shown> m_shown;
    QVector<Request> m_requests;
    int m_nextKey = 1;
    // New file per icon: platform may cache by path, replacement must redraw.
    int m_nextIcon = 1;
};

} // namespace Salama
