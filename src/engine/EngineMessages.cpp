// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 tuuli contributors
#include "EngineMessages.h"

#include <QColor>
#include <QRegularExpression>
#include <QUrl>

namespace Tuuli {

namespace {

bool isWebScheme(const QString &scheme)
{
    return scheme == QLatin1String("http") || scheme == QLatin1String("https");
}

} // namespace

EngineMessages::EngineMessages(QObject *parent)
    : QObject(parent)
{
}

QString EngineMessages::clearPrivateDataTopic() const
{
    return QStringLiteral("clear-private-data");
}

QString EngineMessages::cookiesAndSiteDataPayload() const
{
    return QStringLiteral("cookies-and-site-data");
}

QString EngineMessages::cachePayload() const
{
    return QStringLiteral("cache");
}

QString EngineMessages::faviconScript() const
{
    return QStringLiteral("(function () {"
                          " var link = document.querySelector('link[rel~=\"icon\"]');"
                          " return link && link.href ? String(link.href) : '';"
                          " })()");
}

QString EngineMessages::themeColorScript() const
{
    return QStringLiteral("(function () {"
                          " var meta = document.querySelector('meta[name=\"theme-color\"]');"
                          " return meta && meta.content ? String(meta.content) : '';"
                          " })()");
}

QString EngineMessages::themeColor(const QString &value)
{
    const QString text = value.trimmed();
    if (text.isEmpty()) {
        return {};
    }

    QColor color;
    if (text.startsWith(QLatin1String("rgb"), Qt::CaseInsensitive)) {
        // rgb(r, g, b) and rgba(r, g, b, a), in either the comma-separated form or the
        // space-separated one. The channels are the first three numbers in it; what
        // follows is alpha, which is dropped anyway.
        const QRegularExpression number(QStringLiteral("\\d+"));
        QRegularExpressionMatchIterator matches = number.globalMatch(text);
        QList<int> channels;
        while (matches.hasNext() && channels.count() < 3) {
            channels.append(qBound(0, matches.next().captured().toInt(), 255));
        }
        if (channels.count() == 3) {
            color = QColor(channels.at(0), channels.at(1), channels.at(2));
        }
    } else if (text.startsWith(QLatin1Char('#')) && text.length() == 9) {
        // #rrggbbaa is CSS; QColor would read the same string as #aarrggbb.
        color = QColor(text.left(7));
    } else if (QColor::isValidColor(text)) {
        color = QColor(text);
    }

    if (!color.isValid()) {
        return {};
    }
    color.setAlpha(255);
    return color.name();
}

QString EngineMessages::defaultFavicon(const QString &pageUrl) const
{
    const QUrl page(pageUrl, QUrl::TolerantMode);
    if (!page.isValid() || !isWebScheme(page.scheme()) || page.host().isEmpty()) {
        return {};
    }
    QUrl icon;
    icon.setScheme(page.scheme());
    icon.setHost(page.host());
    icon.setPort(page.port());
    icon.setPath(QStringLiteral("/favicon.ico"));
    return icon.toString();
}

QString EngineMessages::resolveFavicon(const QString &pageUrl, const QString &href) const
{
    const QUrl page(pageUrl, QUrl::TolerantMode);
    const QUrl candidate = page.resolved(QUrl(href.trimmed(), QUrl::TolerantMode));
    if (!href.trimmed().isEmpty() && candidate.isValid() &&
        (isWebScheme(candidate.scheme()) || candidate.scheme() == QLatin1String("data"))) {
        return candidate.toString();
    }
    return defaultFavicon(pageUrl);
}

} // namespace Tuuli
