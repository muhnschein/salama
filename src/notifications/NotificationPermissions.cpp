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
    switch (role) {
    case OriginRole:
        return site.origin;
    case HostRole:
        return hostOf(site.origin);
    case AllowedRole:
        return site.allowed;
    default:
        return {};
    }
}

QHash<int, QByteArray> NotificationPermissions::roleNames() const
{
    return {
        {OriginRole, "origin"},
        {HostRole, "host"},
        {AllowedRole, "allowed"},
    };
}

QString NotificationPermissions::topic() const
{
    return ListTopic;
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
    std::sort(sites.begin(), sites.end(),
              [](const Site &one, const Site &other) { return byHost(one.origin, other.origin); });
    const int before = m_sites.count();
    beginResetModel();
    m_sites = sites;
    endResetModel();
    if (m_sites.count() != before) {
        emit countChanged();
    }
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

void NotificationPermissions::put(const QString &origin, bool allowed)
{
    const int row = rowOf(origin);
    if (row >= 0) {
        if (m_sites.at(row).allowed != allowed) {
            m_sites[row].allowed = allowed;
            const QModelIndex changed = index(row);
            emit dataChanged(changed, changed, {AllowedRole});
        }
        return;
    }
    const int position = static_cast<int>(
        std::find_if(m_sites.cbegin(), m_sites.cend(),
                     [&origin](const Site &site) { return byHost(origin, site.origin); }) -
        m_sites.cbegin());
    beginInsertRows(QModelIndex(), position, position);
    m_sites.insert(position, {origin, allowed});
    endInsertRows();
    emit countChanged();
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
