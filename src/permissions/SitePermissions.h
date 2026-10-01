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

// What a reader decided for a site that differs from what sites may do unless told
// otherwise: its exceptions (docs/DECISIONS/0039-site-permissions.md). Allowed or
// blocked, one for each kind of permission and each site; a site with none of a kind
// follows the default for it, which SitePermissionSettings keeps and the engine applies,
// and which is not in this list.
//
// They are kept where Firefox keeps them, and sailfish-browser keeps its own: in the
// engine's permission manager, as the permission of the kind's name for each site's
// origin (sailfish-browser apps/browser/qml/pages/PermissionPage.qml lists the same
// ones). So the page's own scripts read them as they are. The engine is reached as
// NotificationPermissions reaches it, and by the same code (engine/EnginePermissions.h);
// this model is the list as last read, with what was set from here since.
//
// The list is flat, every kind's exceptions in one, which is what a page of one kind's
// reads through SiteExceptions; what the details of one site show is asked by site.
class SitePermissions : public QAbstractListModel
{
    Q_OBJECT
    // How many sites have an exception of any kind: what the main Settings page says of
    // them before they are listed.
    Q_PROPERTY(int exceptionSiteCount READ exceptionSiteCount NOTIFY changed)
    // Counts how many times the list changed, so that what a page asks by kind or by site
    // -- which is not a property, and a binding cannot watch -- is asked again as it does:
    // `(SitePermissions.revision, SitePermissions.decision(kind, origin))`.
    Q_PROPERTY(int revision READ revision NOTIFY changed)
    // The topic the engine answers on, for WebEngine.addObserver().
    Q_PROPERTY(QString topic READ topic CONSTANT)

public:
    // What a page may ask for and a reader may decide on, in the order Settings lists
    // them. Unscoped, as PrivacySettings::TrackingProtection is, since QML on Qt 5.6
    // reads no scoped enum: `SitePermissions.Popups`. Which of the engine's own names
    // each stands for is in the .cpp, and nowhere else.
    enum Kind // NOSONAR(cpp:S3642) QML on Qt 5.6 reads no scoped enum
    {
        Notifications = 0,
        Popups,
        Cookies,
        Location,
        Camera,
        Microphone,
        // Allowed for a site means tracking protection is off for it, as it does for
        // Firefox's own content blocking allow list; there is no blocking of it.
        TrackingProtection
    };
    Q_ENUM(Kind)

    // What a site's exception says; Default is no exception.
    enum Decision // NOSONAR(cpp:S3642) QML on Qt 5.6 reads no scoped enum
    {
        Default = 0,
        Allow = 1,
        Block = 2,
        // Asked each time, whatever the default: only for a kind that asks
        // (canAsk()). The numbers are the engine's capabilities.
        Ask = 3
    };
    Q_ENUM(Decision)

    enum class Role
    {
        Kind = Qt::UserRole + 1,
        Origin,
        Host,
        Allowed,
        // A Decision: Allow, Block or Ask.
        Decision
    };

    explicit SitePermissions(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int exceptionSiteCount() const;
    int revision() const;
    QString topic() const;

    // Asks the engine for what it holds; the answer arrives through observe().
    Q_INVOKABLE void refresh();
    // What WebEngine.recvObserve() delivered. Anything but the list is ignored.
    Q_INVOKABLE void observe(const QString &topic, const QVariant &data);

    // What was decided for the site, as a Decision: Default for a site with no
    // exception of the kind, and for an address that is not a site's.
    Q_INVOKABLE int decision(int kind, const QString &origin) const;
    // How many sites have an exception of the kind, and how many exceptions the site has
    // of any.
    Q_INVOKABLE int count(int kind) const;
    Q_INVOKABLE int originCount(const QString &origin) const;

    // The site is allowed or blocked for the kind, or follows the default again (Default).
    Q_INVOKABLE void set(int kind, const QString &origin, int decision);
    // The site follows the default for the kind again.
    Q_INVOKABLE void remove(int kind, const QString &origin);
    // Every site follows the default for the kind again.
    Q_INVOKABLE void removeAll(int kind);
    // The site follows the default for every kind again.
    Q_INVOKABLE void removeAllForOrigin(const QString &origin);

    // A site's origin as the engine writes it, from any address of it, and empty for an
    // address that is not an http or https one; and a site's host as a reader reads it.
    // Whether a site can be asked about the kind each time: notifications, location,
    // the camera and the microphone, which a page asks for; not pop-ups or cookies,
    // which it simply does.
    Q_INVOKABLE static bool canAsk(int kind);
    Q_INVOKABLE static QString originOf(const QString &url);
    Q_INVOKABLE static QString hostOf(const QString &origin);

    // A decision made elsewhere in this application, about a kind that has a model of its
    // own, that the engine has already been told of: taken in without telling the engine
    // again, and without saying it was decided from here (NotificationPermissions).
    void adopt(int kind, const QString &origin, int decision);

    // The engine's names for a kind, which a reader never sees: the one for most, and for
    // a location both of the ones in use (the .cpp says which and why). Empty for a
    // number that is no kind.
    static QStringList typesOf(int kind);
    // The kind a name of the engine's is for, or -1 for one that is none of them.
    static int kindOf(const QString &type);

signals:
    // Something in the list changed.
    void changed();
    // A site's exception of a kind was set or taken away from here and the engine told:
    // Default says it was taken away. NotificationPermissions follows those it keeps.
    void decided(int kind, const QString &origin, int decision);
    // Something for the engine, for WebEngine.notifyObservers().
    void engineRequest(const QString &topic, const QVariant &payload);

private:
    struct Exception
    {
        int kind = 0;
        QString origin;
        // A Decision other than Default.
        int decision = Block;
    };

    int rowOf(int kind, const QString &origin) const;
    // Puts the exception in the list, and answers whether that changed it.
    bool put(int kind, const QString &origin, int decision);
    bool take(int kind, const QString &origin);
    void send(const QString &message, int kind, const QString &origin, int capability);
    void touch();

    QVector<Exception> m_exceptions;
    int m_revision = 0;
};

} // namespace Salama
