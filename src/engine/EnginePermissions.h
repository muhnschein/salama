// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QVector>

namespace Salama {

// Engine permission manager via ContentPermissionManager.js topics, shared by
// NotificationPermissions and SitePermissions. Capability 1 allow, 2 deny; expiry 0 never.
namespace EnginePermissions {

constexpr int AllowAction = 1;
constexpr int DenyAction = 2;
constexpr int PromptAction = 3;
constexpr int ExpireNever = 0;

QString requestTopic();
QString listTopic();

struct Entry
{
    QString type;
    QString origin;
    int capability = 0;
};

// Takes parsed JSON or raw string. Keeps only permanent allow/deny/prompt with origin; drops
// origin attributes after '^' (no containers).
QVector<Entry> parse(const QVariant &data);

// Always permanent.
QVariantMap request(const QString &message, const QString &origin = QString(),
                    const QString &type = QString(), int capability = 0);

// "https://host[:port]", ASCII host like Gecko principals; "" unless http(s).
QString originOf(const QString &url);
QString hostOf(const QString &origin);

} // namespace EnginePermissions

} // namespace Salama
