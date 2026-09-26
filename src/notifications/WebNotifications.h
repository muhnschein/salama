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

// The Notifications API, for the pages (docs/DECISIONS/0033-web-notifications.md).
//
// The engine has it and cannot show one: Gecko hands a notification to its alerts
// service, which on this platform has no system backend and falls back to opening a
// XUL window, which embedlite has no way to open; and the platform's WebView refuses
// every request to allow one (Sailfish.WebView.Popups, PopupOpener.qml). So this
// browser answers the API itself, as Firefox's front end answers Gecko's:
//
//  * pageScript(), run in every page, puts a Notification of the application's in
//    the page's place, which asks the browser where Gecko's would ask its alerts
//    service, and reads the permission from Gecko's own;
//  * relayScriptUrl(), a frame script as sailfish-browser loads its own and the
//    platform's WebView its text zoom, hands what the page asks to the application,
//    with the page's origin and the permission Gecko holds for it, which the page
//    cannot write;
//  * this class decides, keeps what is shown, and answers the page with
//    replyScript()s run in it.
//
// What is shown is the platform's (Nemo.Notifications), made by the browsing page as
// publishRequested() and closeRequested() ask. Each is known by a key of this class's.
class WebNotifications : public QObject
{
    Q_OBJECT
    // The name of the frame script's messages, for WebView.addMessageListener().
    Q_PROPERTY(QString messageName READ messageName CONSTANT)
    // The frame script, as a url for WebView.loadFrameScript().
    Q_PROPERTY(QString relayScriptUrl READ relayScriptUrl CONSTANT)
    // For WebView.runJavaScript(), as a page is shown; it answers true once in place.
    Q_PROPERTY(QString pageScript READ pageScript CONSTANT)

public:
    // The reader's answer to a site asking to send notifications, as Firefox offers
    // it: allowed for good, blocked for good, or not now -- refused, and the page not
    // asked about again until it is loaded again. Unscoped for the reason
    // CoverSettings::Style is.
    enum Decision // NOSONAR(cpp:S3642) QML on Qt 5.6 reads no scoped enum
    {
        Allow = 0,
        Block = 1,
        NotNow = 2
    };
    Q_ENUM(Decision)

    // What the page may hand over. A title and a body longer than this are cut, as
    // the platform's own prompts cut a page's text; a message longer than the limit,
    // which is an icon's size, is not read.
    static constexpr int TextLimit = 1000;
    static constexpr int MessageLimit = 1024 * 1024;
    // An icon's longer side: the page draws its icon no larger than this.
    static constexpr int IconSize = 256;

    WebNotifications(NotificationPermissions *permissions, QString iconDirectory,
                     QObject *parent = nullptr);

    QString messageName() const;
    QString relayScriptUrl() const;
    QString pageScript() const;
    QString relayScript() const;

    // What the frame script sent from this tab's page: `{origin, permission, detail}`,
    // the detail being what the page said, as JSON. Anything else is ignored.
    Q_INVOKABLE void receive(int tabId, const QVariant &data);

    // The reader's answer, for every request the tab's page has waiting.
    Q_INVOKABLE void answer(int tabId, int decision);

    // The platform's notification was tapped: the tab comes to the front, and the page
    // hears of it. Then it is closed, as a tapped notification is.
    Q_INVOKABLE void activate(int key);
    // The platform's notification went -- swiped away, or closed by the platform.
    Q_INVOKABLE void closed(int key);

    // The tab's page is gone -- the tab closed, or its view given up -- and with it
    // what it showed and what it asked.
    Q_INVOKABLE void forgetTab(int tabId);
    // Everything shown goes: the browser is closing.
    Q_INVOKABLE void closeAll();

    // The platform's WebView is about to answer something the page asked the engine
    // for itself; the page is at this address. A notification permission it refuses
    // is taken back (NotificationPermissions::undoAutomaticDenial()).
    Q_INVOKABLE void popupOpening(const QString &pageUrl, const QString &topic,
                                  const QVariant &data);

    // What answers the page, for WebView.runJavaScript(): the message, as JSON, handed
    // to the page's Notification.
    static QString replyScript(const QVariantMap &message);

    // The shown notifications, by key: for the tests.
    QList<int> keys() const;
    int tabOf(int key) const;

signals:
    // The tab's page asks whether it may send notifications, and nothing is waiting
    // for an answer to it yet. Answered with answer().
    void permissionRequested(int tabId, const QString &host);
    // The page that asked is gone; its question is no longer to be put.
    void permissionWithdrawn(int tabId);
    // Show a notification, or show this one in place of what the key showed:
    // {summary, body, subText, icon}, the icon a file or empty.
    void publishRequested(int key, const QVariantMap &fields);
    void closeRequested(int key);
    // Run this script in the tab's view.
    void pageRequested(int tabId, const QString &script);
    // Bring the tab to the front, and the browser with it.
    void tabRequested(int tabId);

private:
    struct Shown
    {
        int key = 0;
        int tabId = 0;
        // The page's own: its document, and its number for the notification there.
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
    // Forgets what the key shows; the platform's notification is the caller's.
    Shown take(int key);
    QString saveIcon(int key, const QString &dataUrl);
    void removeIcon(const QString &path) const;

    NotificationPermissions *m_permissions;
    QString m_iconDirectory;
    QHash<int, Shown> m_shown;
    QVector<Request> m_requests;
    int m_nextKey = 1;
    // Each icon is a file of its own, so that the platform, which may keep what it
    // read of a file, draws a notification shown in place of another afresh.
    int m_nextIcon = 1;
};

} // namespace Salama
