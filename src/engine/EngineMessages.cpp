// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "EngineMessages.h"

#include "EngineData.h"
#include "settings/Settings.h"

#include <QColor>
#include <QRegularExpression>
#include <QUrl>
#include <QVector>

namespace Salama {

namespace {

bool isWebScheme(const QString &scheme)
{
    return scheme == QLatin1String("http") || scheme == QLatin1String("https");
}

// nsITypeAheadFind's answers that mean the text is on the page.
const int FindFound = 0;
const int FindWrapped = 2;

// One engine preference, and its value at each level of Settings::TrackingProtection.
struct TrackingPreference
{
    const char *name;
    QVariant off;
    QVariant standard;
    QVariant strict;
};

// Names from gecko-dev modules/libpref/init/StaticPrefList.yaml; Standard's values are
// that file's defaults as browser/app/profile/firefox.js leaves them, and Strict's are
// what ContentBlockingPrefs.sys.mjs sets for the features named in firefox.js's
// browser.contentblocking.features.strict, given in the comments. An engine that
// does not know a name keeps it as a preference nothing reads.
const QVector<TrackingPreference> &trackingPreferences()
{
    // Content classifier features, by their names in kFeatures (gecko-dev
    // toolkit/components/content-classifier/ContentClassifierService.cpp). Annotating
    // marks a request as a tracker's, which is what cookie behaviour 5 reads to refuse
    // a tracker its cookies; blocking cancels the request. "trackers-content" is the
    // level-2 list, which Firefox annotates in Strict and never blocks. Exception-only
    // features go last in a blocking list, where they let an earlier match load after
    // all: "major" for a site that breaks outright, "minor" for one missing an embed.
    static const QString standardAnnotation =
        QStringLiteral("trackers,social-trackers,fingerprinters,cryptominers,email-trackers");
    static const QString strictAnnotation =
        QStringLiteral("trackers,trackers-content,social-trackers,fingerprinters,cryptominers,"
                       "email-trackers");
    static const QString standardBlocking =
        QStringLiteral("fingerprinters,cryptominers,major-exceptions,minor-exceptions");
    static const QString strictBlocking =
        QStringLiteral("trackers,social-trackers,fingerprinters,cryptominers,email-trackers,"
                       "major-exceptions");

    static const QVector<TrackingPreference> preferences{
        // "cookieBehavior5": BEHAVIOR_PARTITION_FOREIGN, Total Cookie Protection; 0 is
        // BEHAVIOR_ACCEPT (netwerk/cookie/nsICookieService.idl).
        {"network.cookie.cookieBehavior", 0, 5, 5},
        {"privacy.trackingprotection.content.annotation.enabled", false, true, true},
        {"privacy.trackingprotection.content.annotation.engines", QString(), standardAnnotation,
         strictAnnotation},
        // "fp", "cryptoTP"; and "tp", "stp", "emailTP" in Strict.
        {"privacy.trackingprotection.content.protection.enabled", false, true, true},
        {"privacy.trackingprotection.content.protection.engines", QString(), standardBlocking,
         strictBlocking},
        // "lvl2": a request on the level-2 list counts as a tracker's.
        {"privacy.annotate_channels.strict_list.enabled", false, false, true},
        // The allow-lists for sites tracking protection breaks; Strict keeps the one
        // for sites that break outright.
        {"privacy.trackingprotection.allow_list.baseline.enabled", true, true, true},
        {"privacy.trackingprotection.allow_list.convenience.enabled", true, true, false},
        // "fpp"
        {"privacy.fingerprintingProtection", false, false, true},
        // "qps"
        {"privacy.query_stripping.enabled", false, false, true},
        // "rpTop"
        {"network.http.referer.disallowCrossSiteRelaxingDefault.top_navigation", false, false,
         true},
        // "btp": MODE_ENABLED 1; MODE_ENABLED_DRY_RUN 3, which Firefox counts as off
        // (toolkit/components/antitracking/bouncetrackingprotection/
        // nsIBounceTrackingProtection.idl).
        {"privacy.bounceTrackingProtection.mode", 3, 3, 1},
    };
    return preferences;
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

QString EngineMessages::memoryPressureTopic() const
{
    return QStringLiteral("memory-pressure");
}

QString EngineMessages::heapMinimizePayload() const
{
    return QStringLiteral("heap-minimize");
}

QString EngineMessages::cachePayload() const
{
    return QStringLiteral("cache");
}

QString EngineMessages::faviconScript() const
{
    return QStringLiteral(" var link = document.querySelector('link[rel~=\"icon\"]');"
                          " return link && link.href ? String(link.href) : '';");
}

QString EngineMessages::themeColorScript() const
{
    return QStringLiteral(" var meta = document.querySelector('meta[name=\"theme-color\"]');"
                          " return meta && meta.content ? String(meta.content) : '';");
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

QString EngineMessages::findMessage() const
{
    return QStringLiteral("embedui:find");
}

QString EngineMessages::findResultMessage() const
{
    return QStringLiteral("embed:find");
}

QVariantMap EngineMessages::findRequest(const QString &text, bool again, bool backwards) const
{
    return {
        {QStringLiteral("text"), text},
        {QStringLiteral("again"), again},
        {QStringLiteral("backwards"), backwards},
    };
}

bool EngineMessages::findFound(const QVariant &data)
{
    const QVariant result = data.toMap().value(QStringLiteral("r"));
    if (!EngineData::isNumber(result)) {
        return false;
    }
    const double value = result.toDouble();
    return value == FindFound || value == FindWrapped;
}

QVariantList EngineMessages::trackingProtectionPreferences(int level)
{
    QVariantList list;
    for (const TrackingPreference &preference : trackingPreferences()) {
        QVariant value = preference.standard;
        if (level == Settings::TrackingProtectionOff) {
            value = preference.off;
        } else if (level == Settings::TrackingProtectionStrict) {
            value = preference.strict;
        }
        list.append(QVariantMap{
            {QStringLiteral("name"), QLatin1String(preference.name)},
            {QStringLiteral("value"), value},
        });
    }
    return list;
}

} // namespace Salama
