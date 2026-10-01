// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "EnginePermissions.h"

#include "EngineData.h"

#include <QJsonDocument>
#include <QUrl>
#include <QVariantList>

namespace Salama {

namespace EnginePermissions {

QString requestTopic()
{
    return QStringLiteral("embedui:perms");
}

QString listTopic()
{
    return QStringLiteral("embed:perms:all");
}

QVector<Entry> parse(const QVariant &data)
{
    const QVariantList list =
        data.userType() == QMetaType::QString
            ? QJsonDocument::fromJson(data.toString().toUtf8()).toVariant().toList()
            : data.toList();
    QVector<Entry> entries;
    for (const QVariant &entry : list) {
        const QVariantMap permission = entry.toMap();
        const QVariant expireType = permission.value(QStringLiteral("expireType"));
        if (!EngineData::isNumber(expireType) || expireType.toInt() != ExpireNever) {
            continue;
        }
        const QVariant capability = permission.value(QStringLiteral("capability"));
        const int action = EngineData::isNumber(capability) ? capability.toInt() : 0;
        const QString origin = originOf(
            permission.value(QStringLiteral("uri")).toString().section(QLatin1Char('^'), 0, 0));
        if (origin.isEmpty() || (action != AllowAction && action != DenyAction && action != PromptAction)) {
            continue;
        }
        entries.append({permission.value(QStringLiteral("type")).toString(), origin, action});
    }
    return entries;
}

QVariantMap request(const QString &message, const QString &origin, const QString &type,
                    int capability)
{
    QVariantMap request{{QStringLiteral("msg"), message}};
    if (message != QLatin1String("get-all")) {
        request.insert(QStringLiteral("uri"), origin);
        request.insert(QStringLiteral("type"), type);
        request.insert(QStringLiteral("permission"), capability);
        request.insert(QStringLiteral("expireType"), ExpireNever);
    }
    return request;
}

QString originOf(const QString &url)
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

QString hostOf(const QString &origin)
{
    return QUrl(origin).host(QUrl::PrettyDecoded);
}

} // namespace EnginePermissions

} // namespace Salama
