// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QVector>

namespace Salama {

// The engine's permission manager, as both of the models that keep what it holds speak
// to it: NotificationPermissions for the one permission a page may ask for here
// (docs/DECISIONS/0033-web-notifications.md) and SitePermissions for the ones a reader
// sets by site (docs/DECISIONS/0039-site-permissions.md). One reading of what it
// answers and one way of asking, so that the two cannot come to read it differently.
//
// The engine is reached as the platform's Sailfish.WebView.Controls PermissionManager
// reaches it, over the observer topics embedlite-components'
// jscomps/ContentPermissionManager.js answers: "embedui:perms" with a message, `add`,
// `remove` or `get-all`, and every permission the engine holds back on "embed:perms:all"
// as `[{type, uri, capability, expireType}]`, the uri being the principal's origin. A
// capability is nsIPermissionManager's: 1 allow, 2 deny; an expiry 0 never, 1 with the
// session.
namespace EnginePermissions {

// nsIPermissionManager's capabilities and expiry types.
constexpr int AllowAction = 1;
constexpr int DenyAction = 2;
constexpr int ExpireNever = 0;

// What ContentPermissionManager.js listens on, and answers on.
QString requestTopic();
QString listTopic();

// One permission of the list the engine answers: the kind it is, by Gecko's name for
// it, the site's origin as originOf() writes it, and allow or deny.
struct Entry
{
    QString type;
    QString origin;
    int capability = 0;
};

// What the engine answered. qtmozembed hands over what it could read as JSON read, and
// anything else as the string it was; either is taken. Only what was decided for good,
// as allowed or denied, for a site that has an origin: a decision that lasts the session
// is the platform's own, not the reader's, and the other capabilities -- a cookie's
// "first party only" among them -- have no place in a choice of two. The origin may
// carry the principal's attributes after a caret; this engine has no containers, and
// the site is the part before it.
QVector<Entry> parse(const QVariant &data);

// A message for the engine, in the form ContentPermissionManager.js reads: `add` names
// the site, the kind and the capability, `remove` the site and the kind, and `get-all`
// needs nothing of them. A permission written from here is for good.
QVariantMap request(const QString &message, const QString &origin = QString(),
                    const QString &type = QString(), int capability = 0);

// A page's origin as the engine writes it -- "https://host", with the port when it is
// not the scheme's own -- for an http or https address, and empty for any other. The
// host is in its ASCII form, as Gecko's principals have it.
QString originOf(const QString &url);
// The host of an origin as a reader reads it.
QString hostOf(const QString &origin);

} // namespace EnginePermissions

} // namespace Salama
