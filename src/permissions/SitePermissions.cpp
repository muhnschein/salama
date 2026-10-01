// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "SitePermissions.h"

#include "engine/EnginePermissions.h"

#include <QSet>
#include <algorithm>

namespace Salama {

namespace {

using EnginePermissions::AllowAction;
using EnginePermissions::DenyAction;

const int KindCount = SitePermissions::TrackingProtection + 1;

int capabilityOf(bool allowed)
{
    return allowed ? AllowAction : DenyAction;
}

} // namespace

// The engine's name for each kind. Notifications, pop-ups, cookies, the camera and the
// microphone have one each, Firefox's and sailfish-browser's alike. A location has two:
// the platform's prompt writes what the page asked for, "geolocation"
// (embedlite-components jscomps/ContentPermissionPrompt.js), which sailfish-browser lists
// under that name, and Gecko's own front end and its default preference call it "geo".
// Which of them the engine reads cannot be seen from here, so both are written, and
// either is read. "trackingprotection" is Gecko's content blocking allow list: a site
// with it allowed has tracking protection off.
QStringList SitePermissions::typesOf(int kind)
{
    switch (kind) {
    case Notifications:
        return {QStringLiteral("desktop-notification")};
    case Popups:
        return {QStringLiteral("popup")};
    case Cookies:
        return {QStringLiteral("cookie")};
    case Location:
        return {QStringLiteral("geolocation"), QStringLiteral("geo")};
    case Camera:
        return {QStringLiteral("camera")};
    case Microphone:
        return {QStringLiteral("microphone")};
    case TrackingProtection:
        return {QStringLiteral("trackingprotection")};
    default:
        return {};
    }
}

int SitePermissions::kindOf(const QString &type)
{
    for (int kind = 0; kind < KindCount; ++kind) {
        if (typesOf(kind).contains(type)) {
            return kind;
        }
    }
    return -1;
}

SitePermissions::SitePermissions(QObject *parent)
    : QAbstractListModel(parent)
{
}

int SitePermissions::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_exceptions.count();
}

QVariant SitePermissions::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_exceptions.count()) {
        return {};
    }
    const Exception &exception = m_exceptions.at(index.row());
    switch (static_cast<Role>(role)) {
    case Role::Kind:
        return exception.kind;
    case Role::Origin:
        return exception.origin;
    case Role::Host:
        return hostOf(exception.origin);
    case Role::Allowed:
        return exception.allowed;
    default:
        return {};
    }
}

QHash<int, QByteArray> SitePermissions::roleNames() const
{
    return {
        {roleId(Role::Kind), "kind"},
        {roleId(Role::Origin), "origin"},
        {roleId(Role::Host), "host"},
        {roleId(Role::Allowed), "allowed"},
    };
}

int SitePermissions::exceptionSiteCount() const
{
    QSet<QString> sites;
    for (const Exception &exception : m_exceptions) {
        sites.insert(exception.origin);
    }
    return sites.count();
}

int SitePermissions::revision() const
{
    return m_revision;
}

QString SitePermissions::topic() const
{
    return EnginePermissions::listTopic();
}

void SitePermissions::refresh()
{
    emit engineRequest(EnginePermissions::requestTopic(),
                       EnginePermissions::request(QStringLiteral("get-all")));
}

void SitePermissions::observe(const QString &topic, const QVariant &data)
{
    if (topic != EnginePermissions::listTopic()) {
        return;
    }
    QVector<Exception> exceptions;
    for (const EnginePermissions::Entry &permission : EnginePermissions::parse(data)) {
        const int kind = kindOf(permission.type);
        const bool allowed = permission.capability == AllowAction;
        // Gecko's allow list has no deny: a record of one is nothing this application
        // writes or reads as a decision.
        if (kind < 0 || (kind == TrackingProtection && !allowed)) {
            continue;
        }
        const auto same = [kind, &permission](const Exception &exception) {
            return exception.kind == kind && exception.origin == permission.origin;
        };
        if (std::none_of(exceptions.cbegin(), exceptions.cend(), same)) {
            exceptions.append({kind, permission.origin, allowed});
        }
    }
    beginResetModel();
    m_exceptions = exceptions;
    endResetModel();
    touch();
}

int SitePermissions::decision(int kind, const QString &origin) const
{
    const int row = rowOf(kind, originOf(origin));
    if (row < 0) {
        return Default;
    }
    return m_exceptions.at(row).allowed ? Allow : Block;
}

int SitePermissions::count(int kind) const
{
    return int(
        std::count_if(m_exceptions.cbegin(), m_exceptions.cend(),
                      [kind](const Exception &exception) { return exception.kind == kind; }));
}

int SitePermissions::originCount(const QString &origin) const
{
    const QString site = originOf(origin);
    if (site.isEmpty()) {
        return 0;
    }
    return int(
        std::count_if(m_exceptions.cbegin(), m_exceptions.cend(),
                      [&site](const Exception &exception) { return exception.origin == site; }));
}

void SitePermissions::set(int kind, const QString &origin, int decision)
{
    if (decision != Allow && decision != Block) {
        remove(kind, origin);
        return;
    }
    const QString site = originOf(origin);
    if (site.isEmpty() || typesOf(kind).isEmpty()) {
        return;
    }
    send(QStringLiteral("add"), kind, site, capabilityOf(decision == Allow));
    if (put(kind, site, decision == Allow)) {
        touch();
    }
    emit decided(kind, site, decision);
}

void SitePermissions::remove(int kind, const QString &origin)
{
    const QString site = originOf(origin);
    if (site.isEmpty() || typesOf(kind).isEmpty()) {
        return;
    }
    // Sent whether or not the list holds it: the engine may hold what was written since
    // it was last read, and a site that follows the default is what was asked for.
    send(QStringLiteral("remove"), kind, site, 0);
    if (take(kind, site)) {
        touch();
    }
    emit decided(kind, site, Default);
}

void SitePermissions::removeAll(int kind)
{
    QStringList sites;
    for (const Exception &exception : m_exceptions) {
        if (exception.kind == kind) {
            sites.append(exception.origin);
        }
    }
    for (const QString &site : sites) {
        remove(kind, site);
    }
}

void SitePermissions::removeAllForOrigin(const QString &origin)
{
    const QString site = originOf(origin);
    QVector<int> kinds;
    for (const Exception &exception : m_exceptions) {
        if (exception.origin == site) {
            kinds.append(exception.kind);
        }
    }
    for (int kind : kinds) {
        remove(kind, site);
    }
}

QString SitePermissions::originOf(const QString &url)
{
    return EnginePermissions::originOf(url);
}

QString SitePermissions::hostOf(const QString &origin)
{
    return EnginePermissions::hostOf(origin);
}

void SitePermissions::adopt(int kind, const QString &origin, int decision)
{
    const QString site = originOf(origin);
    if (site.isEmpty() || typesOf(kind).isEmpty()) {
        return;
    }
    const bool changed = decision == Allow || decision == Block ? put(kind, site, decision == Allow)
                                                                : take(kind, site);
    if (changed) {
        touch();
    }
}

int SitePermissions::rowOf(int kind, const QString &origin) const
{
    if (origin.isEmpty()) {
        return -1;
    }
    for (int row = 0; row < m_exceptions.count(); ++row) {
        if (m_exceptions.at(row).kind == kind && m_exceptions.at(row).origin == origin) {
            return row;
        }
    }
    return -1;
}

bool SitePermissions::put(int kind, const QString &origin, bool allowed)
{
    const int row = rowOf(kind, origin);
    if (row >= 0) {
        if (m_exceptions.at(row).allowed == allowed) {
            return false;
        }
        m_exceptions[row].allowed = allowed;
        const QModelIndex changed = index(row);
        emit dataChanged(changed, changed, {roleId(Role::Allowed)});
        return true;
    }
    const int position = m_exceptions.count();
    beginInsertRows(QModelIndex(), position, position);
    m_exceptions.append({kind, origin, allowed});
    endInsertRows();
    return true;
}

bool SitePermissions::take(int kind, const QString &origin)
{
    const int row = rowOf(kind, origin);
    if (row < 0) {
        return false;
    }
    beginRemoveRows(QModelIndex(), row, row);
    m_exceptions.remove(row);
    endRemoveRows();
    return true;
}

void SitePermissions::send(const QString &message, int kind, const QString &origin, int capability)
{
    for (const QString &type : typesOf(kind)) {
        emit engineRequest(EnginePermissions::requestTopic(),
                           EnginePermissions::request(message, origin, type, capability));
    }
}

void SitePermissions::touch()
{
    ++m_revision;
    emit changed();
}

} // namespace Salama
