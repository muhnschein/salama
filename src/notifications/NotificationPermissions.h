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

// The sites allowed to send notifications, and the sites blocked from asking
// (docs/DECISIONS/0033-web-notifications.md).
//
// They are kept where Firefox keeps them, and sailfish-browser keeps its site
// permissions: in the engine's own permission manager, as the "desktop-notification"
// permission of each site's origin. So the page's own `Notification.permission` and
// `navigator.permissions` read them as they are, with nothing of this application in
// the way. The engine is reached as the platform's Sailfish.WebView.Controls
// PermissionManager reaches it, over the observer topics embedlite-components'
// jscomps/ContentPermissionManager.js answers: "embedui:perms" with a message, `add`,
// `remove` or `get-all`, and every permission the engine holds back on
// "embed:perms:all" as `[{type, uri, capability, expireType}]`, the uri being the
// principal's origin. A capability is nsIPermissionManager's: 1 allow, 2 deny; an
// expiry 0 never, 1 with the session.
//
// This model is the list the engine holds, as last read, with what was set from here
// since. It lists what was decided for good: a decision that lasts the session is the
// platform's, not the reader's (below).
class NotificationPermissions : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    // The topic the engine answers on, for WebEngine.addObserver().
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

    // Asks the engine for what it holds; the answer arrives through observe().
    Q_INVOKABLE void refresh();
    // What WebEngine.recvObserve() delivered. Anything but the list is ignored.
    Q_INVOKABLE void observe(const QString &topic, const QVariant &data);

    // The site may send notifications, or may not ask to, until this is changed.
    Q_INVOKABLE void setAllowed(const QString &origin, bool allowed);
    // The site is forgotten: it asks again the next time it wants to.
    Q_INVOKABLE void remove(const QString &origin);

    // Whether the site a page is on -- any address of it -- may send notifications, as
    // this model knows. The browsing page does not put such a page to sleep out of
    // sight (docs/DECISIONS/0020-pages-sleep-out-of-sight.md), since a page asleep
    // sends nothing.
    Q_INVOKABLE bool isAllowed(const QString &url) const;
    bool isBlocked(const QString &origin) const;

    // Firefox's "Block new requests asking to allow notifications": the engine's
    // default for the permission, which sites with none of their own answer to. The
    // preference, as {name, value} for WebEngineSettings.setPreference(): 2 when new
    // requests are blocked, which nsIPermissionManager reads as deny, and 0, unknown,
    // when they are asked about (netwerk PermissionManager, "permissions.default.").
    Q_INVOKABLE static QVariantMap defaultPreference(bool blockRequests);

    // The platform answered a request the page made of the engine itself, rather than
    // of this browser, and denied it for the session: Sailfish.WebView.Popups'
    // PopupOpener.qml refuses every permission but a location's, and
    // ContentPermissionPrompt.js writes that down (sailfish-components-webview,
    // embedlite-components). A site that asked before this browser's part of the page
    // was in place would be refused for the rest of the session, never having been
    // asked. So the refusal is taken back, unless the site is one decided on for good.
    // The platform names the site by its host alone; the origin is the page's.
    void undoAutomaticDenial(const QString &origin);

    // A page's origin as the engine writes it -- "https://host", with the port when it
    // is not the scheme's own -- for an http or https address, and empty for any other.
    // The host is in its ASCII form, as Gecko's principals have it.
    static QString originOf(const QString &url);
    // The host of an origin as a reader reads it.
    static QString hostOf(const QString &origin);

signals:
    void countChanged();
    // Something for the engine, for WebEngine.notifyObservers().
    void engineRequest(const QString &topic, const QVariant &payload);

private:
    struct Site
    {
        QString origin;
        bool allowed = false;
    };

    int rowOf(const QString &origin) const;
    void put(const QString &origin, bool allowed);
    void send(const QString &message, const QString &origin, int capability);

    // By host, as the reader looks for one.
    QVector<Site> m_sites;
};

} // namespace Salama
