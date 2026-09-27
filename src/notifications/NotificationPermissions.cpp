// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "NotificationPermissions.h"

#include "engine/EngineData.h"

#include <QJsonDocument>
#include <QUrl>
#include <QVariantList>
#include <algorithm>

namespace Salama {

namespace {

// What embedlite-components' ContentPermissionManager.js listens on, and answers on.
const QString RequestTopic = QStringLiteral("embedui:perms");
const QString ListTopic = QStringLiteral("embed:perms:all");
// The permission's name in Gecko, Firefox's and the engine's own
// (dom/notification/Notification.cpp, embedlite-components ContentPermissionPrompt.js).
const QString PermissionType = QStringLiteral("desktop-notification");
const QString DefaultPreference = QStringLiteral("permissions.default.desktop-notification");

// nsIPermissionManager's capabilities and expiry types.
const int AllowAction = 1;
const int DenyAction = 2;
const int ExpireNever = 0;

bool byHost(const QString &one, const QString &other)
{
    const QString oneHost = NotificationPermissions::hostOf(one);
    const QString otherHost = NotificationPermissions::hostOf(other);
    return oneHost == otherHost ? one < other : oneHost < otherHost;
}

// The list's order: the sites allowed, then the sites blocked, each by host.
bool before(const QString &one, bool oneAllowed, const QString &other, bool otherAllowed)
{
    return oneAllowed == otherAllowed ? byHost(one, other) : oneAllowed;
}

} // namespace

NotificationPermissions::NotificationPermissions(QObject *parent)
    : QAbstractListModel(parent)
{
}

int NotificationPermissions::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_sites.count();
}

QVariant NotificationPermissions::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_sites.count()) {
        return {};
    }
    const Site &site = m_sites.at(index.row());
    switch (static_cast<Role>(role)) {
    case Role::Origin:
        return site.origin;
    case Role::Host:
        return hostOf(site.origin);
    case Role::Allowed:
        return site.allowed;
    default:
        return {};
    }
}

QHash<int, QByteArray> NotificationPermissions::roleNames() const
{
    return {
        {roleId(Role::Origin), "origin"},
        {roleId(Role::Host), "host"},
        {roleId(Role::Allowed), "allowed"},
    };
}

QString NotificationPermissions::topic() const
{
    return ListTopic;
}

int NotificationPermissions::allowedCount() const
{
    return int(std::count_if(m_sites.cbegin(), m_sites.cend(),
                             [](const Site &site) { return site.allowed; }));
}

int NotificationPermissions::blockedCount() const
{
    return m_sites.count() - allowedCount();
}

void NotificationPermissions::refresh()
{
    emit engineRequest(RequestTopic,
                       QVariantMap{{QStringLiteral("msg"), QStringLiteral("get-all")}});
}

void NotificationPermissions::observe(const QString &topic, const QVariant &data)
{
    if (topic != ListTopic) {
        return;
    }
    // qtmozembed hands over what it could read as JSON read, and anything else as the
    // string it was.
    const QVariantList list =
        data.userType() == QMetaType::QString
            ? QJsonDocument::fromJson(data.toString().toUtf8()).toVariant().toList()
            : data.toList();
    QVector<Site> sites;
    for (const QVariant &entry : list) {
        const QVariantMap permission = entry.toMap();
        if (permission.value(QStringLiteral("type")).toString() != PermissionType) {
            continue;
        }
        const QVariant expireType = permission.value(QStringLiteral("expireType"));
        if (!EngineData::isNumber(expireType) || expireType.toInt() != ExpireNever) {
            continue;
        }
        const QVariant capability = permission.value(QStringLiteral("capability"));
        const int action = EngineData::isNumber(capability) ? capability.toInt() : 0;
        // The origin may carry the principal's attributes after a caret; this engine
        // has no containers, and the site is the part before it.
        const QString origin = originOf(
            permission.value(QStringLiteral("uri")).toString().section(QLatin1Char('^'), 0, 0));
        if (origin.isEmpty() || (action != AllowAction && action != DenyAction)) {
            continue;
        }
        const auto same = [&origin](const Site &site) { return site.origin == origin; };
        if (std::none_of(sites.cbegin(), sites.cend(), same)) {
            sites.append({origin, action == AllowAction});
        }
    }
    std::sort(sites.begin(), sites.end(), [](const Site &one, const Site &other) {
        return before(one.origin, one.allowed, other.origin, other.allowed);
    });
    const int rows = m_sites.count();
    beginResetModel();
    m_sites = sites;
    endResetModel();
    if (m_sites.count() != rows) {
        emit countChanged();
    }
    emit sitesChanged();
}

void NotificationPermissions::setAllowed(const QString &origin, bool allowed)
{
    const QString site = originOf(origin);
    if (site.isEmpty()) {
        return;
    }
    send(QStringLiteral("add"), site, allowed ? AllowAction : DenyAction);
    put(site, allowed);
}

void NotificationPermissions::remove(const QString &origin)
{
    const QString site = originOf(origin);
    const int row = rowOf(site);
    if (row < 0) {
        return;
    }
    send(QStringLiteral("remove"), site, 0);
    beginRemoveRows(QModelIndex(), row, row);
    m_sites.remove(row);
    endRemoveRows();
    emit countChanged();
    emit sitesChanged();
}

bool NotificationPermissions::isAllowed(const QString &url) const
{
    const int row = rowOf(originOf(url));
    return row >= 0 && m_sites.at(row).allowed;
}

bool NotificationPermissions::isBlocked(const QString &origin) const
{
    const int row = rowOf(originOf(origin));
    return row >= 0 && !m_sites.at(row).allowed;
}

QVariantMap NotificationPermissions::defaultPreference(bool blockRequests)
{
    return {
        {QStringLiteral("name"), DefaultPreference},
        {QStringLiteral("value"), blockRequests ? DenyAction : 0},
    };
}

void NotificationPermissions::undoAutomaticDenial(const QString &origin)
{
    const QString site = originOf(origin);
    if (site.isEmpty() || rowOf(site) >= 0) {
        return;
    }
    send(QStringLiteral("remove"), site, 0);
}

QString NotificationPermissions::originOf(const QString &url)
{
    const QUrl address(url, QUrl::StrictMode);
    const QString scheme = address.scheme().toLower();
    if (!address.isValid() ||
        (scheme != QLatin1String("https") && scheme != QLatin1String("http"))) {
        return {};
    }
    QString host = address.host(QUrl::FullyEncoded).toLower();
    if (host.isEmpty()) {
        return {};
    }
    if (host.contains(QLatin1Char(':'))) {
        host = QLatin1Char('[') + host + QLatin1Char(']');
    }
    const int port = address.port();
    const int schemePort = scheme == QLatin1String("https") ? 443 : 80;
    QString origin = scheme + QStringLiteral("://") + host;
    if (port >= 0 && port != schemePort) {
        origin += QLatin1Char(':') + QString::number(port);
    }
    return origin;
}

QString NotificationPermissions::hostOf(const QString &origin)
{
    return QUrl(origin).host(QUrl::PrettyDecoded);
}

int NotificationPermissions::rowOf(const QString &origin) const
{
    if (origin.isEmpty()) {
        return -1;
    }
    for (int row = 0; row < m_sites.count(); ++row) {
        if (m_sites.at(row).origin == origin) {
            return row;
        }
    }
    return -1;
}

int NotificationPermissions::positionOf(const QString &origin, bool allowed) const
{
    return static_cast<int>(std::find_if(m_sites.cbegin(), m_sites.cend(),
                                         [&origin, allowed](const Site &site) {
                                             return site.origin != origin &&
                                                    before(origin, allowed, site.origin,
                                                           site.allowed);
                                         }) -
                            m_sites.cbegin());
}

// A site allowed or blocked from here goes under the other heading: its row moves
// rather than being taken out and put back, so the list keeps the row a finger is on.
void NotificationPermissions::put(const QString &origin, bool allowed)
{
    const int row = rowOf(origin);
    if (row >= 0) {
        if (m_sites.at(row).allowed == allowed) {
            return;
        }
        // Where it goes counted among the others, and where that is before the move
        // as beginMoveRows() counts it.
        int to = positionOf(origin, allowed);
        if (to > row) {
            --to;
        }
        if (to != row) {
            beginMoveRows(QModelIndex(), row, row, QModelIndex(), to > row ? to + 1 : to);
            m_sites.move(row, to);
            endMoveRows();
        }
        m_sites[to].allowed = allowed;
        const QModelIndex changed = index(to);
        emit dataChanged(changed, changed, {roleId(Role::Allowed)});
        emit sitesChanged();
        return;
    }
    const int position = positionOf(origin, allowed);
    beginInsertRows(QModelIndex(), position, position);
    m_sites.insert(position, {origin, allowed});
    endInsertRows();
    emit countChanged();
    emit sitesChanged();
}

void NotificationPermissions::send(const QString &message, const QString &origin, int capability)
{
    emit engineRequest(RequestTopic, QVariantMap{
                                         {QStringLiteral("msg"), message},
                                         {QStringLiteral("uri"), origin},
                                         {QStringLiteral("type"), PermissionType},
                                         {QStringLiteral("permission"), capability},
                                         {QStringLiteral("expireType"), ExpireNever},
                                     });
}

} // namespace Salama
