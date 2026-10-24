// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "EngineMessages.h"

#include "EngineData.h"
#include "settings/DohSettings.h"
#include "settings/PrivacySettings.h"
#include "settings/SearchSettings.h"
#include "settings/Settings.h"
#include "settings/SitePermissionSettings.h"
#include "tabs/TabModel.h"

#include <QColor>
#include <QRegularExpression>
#include <QStringList>
#include <QUrl>
#include <QVector>
#include <cstring>

namespace Salama {

namespace {

bool isWebScheme(const QString &scheme)
{
    return scheme == QLatin1String("http") || scheme == QLatin1String("https");
}

QString shownAddress(const QString &address)
{
    if (address.isEmpty()) {
        return {};
    }
    const QUrl url(address, QUrl::TolerantMode);
    if (TabModel::isExternalUrl(address)) {
        return url.path(QUrl::FullyDecoded);
    }
    if (url.host().isEmpty()) {
        return url.toDisplayString();
    }
    QString rest = url.path(QUrl::PrettyDecoded);
    if (url.hasQuery()) {
        rest += QLatin1Char('?') + url.query(QUrl::PrettyDecoded);
    }
    if (rest == QLatin1String("/")) {
        rest.clear();
    }
    return SearchSettings::displayAddress(address) + rest;
}

const int FindFound = 0;
const int FindWrapped = 2;

// Tracking protection's while on, Site permissions' while off.
const char *const CookieBehavior = "network.cookie.cookieBehavior";

struct TrackingPreference
{
    const char *name;
    QVariant off;
    QVariant standard;
    QVariant strict;
};

// Standard = Firefox defaults; Strict = ContentBlockingPrefs.sys.mjs features. Unknown names
// inert.
const QVector<TrackingPreference> &trackingPreferences()
{
    // Annotate = mark tracker (cookie behaviour 5 reads it); block = cancel. Exception features
    // go last: "major" = site breaks, "minor" = missing embed.
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
        // Total Cookie Protection. At Off, reader's cookie choice replaces 0.
        {CookieBehavior, 0, 5, 5},
        {"privacy.trackingprotection.content.annotation.enabled", false, true, true},
        {"privacy.trackingprotection.content.annotation.engines", QString(), standardAnnotation,
         strictAnnotation},
        {"privacy.trackingprotection.content.protection.enabled", false, true, true},
        {"privacy.trackingprotection.content.protection.engines", QString(), standardBlocking,
         strictBlocking},
        {"privacy.annotate_channels.strict_list.enabled", false, false, true},
        // Strict keeps only outright-breakage allow-list.
        {"privacy.trackingprotection.allow_list.baseline.enabled", true, true, true},
        {"privacy.trackingprotection.allow_list.convenience.enabled", true, true, false},
        {"privacy.fingerprintingProtection", false, false, true},
        {"privacy.query_stripping.enabled", false, false, true},
        {"network.http.referer.disallowCrossSiteRelaxingDefault.top_navigation", false, false,
         true},
        // MODE_ENABLED 1; MODE_ENABLED_DRY_RUN 3 = off per Firefox.
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

QString EngineMessages::viewportScript() const
{
    return QStringLiteral(" var meta = document.querySelector('meta[name=\"viewport\"]');"
                          " return meta && meta.content ? String(meta.content) : '';");
}

bool EngineMessages::coversCutout(const QString &viewport)
{
    const QRegularExpression cover(
        QStringLiteral("(^|[,;])\\s*viewport-fit\\s*=\\s*cover\\s*($|[,;])"),
        QRegularExpression::CaseInsensitiveOption);
    return cover.match(viewport).hasMatch();
}

QString EngineMessages::themeColor(const QString &value)
{
    const QString text = value.trimmed();
    if (text.isEmpty()) {
        return {};
    }

    QColor color;
    if (text.startsWith(QLatin1String("rgb"), Qt::CaseInsensitive)) {
        // First three numbers = channels; alpha dropped.
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
        // CSS #rrggbbaa; QColor would read as #aarrggbb.
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

QString EngineMessages::searchOfferedMessage() const
{
    return QStringLiteral("Link:AddSearch");
}

QVariantMap EngineMessages::searchOffered(const QVariant &data)
{
    const QVariantMap message = data.toMap();
    const QVariantMap engine = message.value(QStringLiteral("engine")).toMap();
    const QString href = engine.value(QStringLiteral("href")).toString();
    // Page host, not description's: descriptions may live elsewhere.
    const QUrl page(message.value(QStringLiteral("url")).toString(), QUrl::TolerantMode);
    const QString host =
        SearchSettings::displayAddress(page.host().isEmpty() ? href : page.toString());
    return {
        {QStringLiteral("title"), engine.value(QStringLiteral("title")).toString()},
        {QStringLiteral("href"), href},
        {QStringLiteral("host"), host},
    };
}

QString EngineMessages::contextMenuMessage() const
{
    return QStringLiteral("Content:ContextMenu");
}

QVariantMap EngineMessages::linkTarget(const QVariant &data)
{
    const QVariantMap message = data.toMap();
    const QStringList types = message.value(QStringLiteral("types")).toStringList();

    QString link;
    QString scheme;
    if (types.contains(QStringLiteral("link"))) {
        const QString href = message.value(QStringLiteral("linkURL")).toString().trimmed();
        scheme = QUrl(href, QUrl::TolerantMode).scheme().toLower();
        if (!scheme.isEmpty() && scheme != QLatin1String("javascript")) {
            link = href;
        } else {
            scheme.clear();
        }
    }

    QString image;
    if (types.contains(QStringLiteral("image"))) {
        const QString src = message.value(QStringLiteral("mediaURL")).toString().trimmed();
        if (isWebScheme(QUrl(src, QUrl::TolerantMode).scheme().toLower())) {
            image = src;
        }
    }

    QString kind;
    if (!link.isEmpty()) {
        kind = TabModel::isExternalUrl(link) ? QStringLiteral("app") : QStringLiteral("page");
    }
    const QString title = message.value(QStringLiteral("linkTitle")).toString().simplified();
    return {
        {QStringLiteral("link"), link},
        {QStringLiteral("kind"), kind},
        {QStringLiteral("scheme"), scheme},
        {QStringLiteral("title"), title},
        {QStringLiteral("image"), image},
        {QStringLiteral("contentType"), message.value(QStringLiteral("contentType")).toString()},
        {QStringLiteral("address"), shownAddress(link.isEmpty() ? image : link)},
    };
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

QVariantList EngineMessages::trackingProtectionPreferences(int level, int cookies)
{
    const bool known = cookies >= SitePermissionSettings::CookiesAllowAll &&
                       cookies <= SitePermissionSettings::CookiesBlockAll;
    const int behavior = known ? cookies : int(SitePermissionSettings::CookiesBlockCrossSite);
    QVariantList list;
    for (const TrackingPreference &preference : trackingPreferences()) {
        QVariant value = preference.standard;
        if (level == PrivacySettings::TrackingProtectionOff) {
            value = std::strcmp(preference.name, CookieBehavior) == 0 ? QVariant(behavior)
                                                                      : preference.off;
        } else if (level == PrivacySettings::TrackingProtectionStrict) {
            value = preference.strict;
        }
        list.append(QVariantMap{
            {QStringLiteral("name"), QLatin1String(preference.name)},
            {QStringLiteral("value"), value},
        });
    }
    return list;
}

QVariantList EngineMessages::sitePermissionPreferences(bool popupsAllowed, bool locationBlocked,
                                                       bool cameraBlocked, bool microphoneBlocked)
{
    // 0 ask, 2 deny. Location has two engine names, both set.
    const auto asking = [](const char *name, bool blocked) {
        return QVariantMap{
            {QStringLiteral("name"), QLatin1String(name)},
            {QStringLiteral("value"), blocked ? 2 : 0},
        };
    };
    return {
        QVariantMap{
            {QStringLiteral("name"), QStringLiteral("dom.disable_open_during_load")},
            {QStringLiteral("value"), !popupsAllowed},
        },
        asking("permissions.default.geo", locationBlocked),
        asking("permissions.default.geolocation", locationBlocked),
        asking("permissions.default.camera", cameraBlocked),
        asking("permissions.default.microphone", microphoneBlocked),
    };
}

QVariantList EngineMessages::websiteColorPreferences(int colors, bool darkAmbience)
{
    bool dark = darkAmbience;
    if (colors == Settings::WebsiteColorsLight) {
        dark = false;
    } else if (colors == Settings::WebsiteColorsDark) {
        dark = true;
    }
    return {QVariantMap{
        {QStringLiteral("name"), QStringLiteral("ui.systemUsesDarkTheme")},
        {QStringLiteral("value"), dark ? 1 : 0},
    }};
}

QVariantList EngineMessages::contentPreferences(bool globalPrivacyControl, bool javascript)
{
    return {
        QVariantMap{{QStringLiteral("name"), QStringLiteral("privacy.donottrackheader.enabled")},
                    {QStringLiteral("value"), false}},
        QVariantMap{{QStringLiteral("name"),
                     QStringLiteral("privacy.globalprivacycontrol.functionality.enabled")},
                    {QStringLiteral("value"), true}},
        QVariantMap{
            {QStringLiteral("name"), QStringLiteral("privacy.globalprivacycontrol.enabled")},
            {QStringLiteral("value"), globalPrivacyControl}},
        QVariantMap{{QStringLiteral("name"), QStringLiteral("javascript.enabled")},
                    {QStringLiteral("value"), javascript}}};
}

QVariantList EngineMessages::httpsOnlyPreferences(bool httpsOnly)
{
    return {QVariantMap{{QStringLiteral("name"), QStringLiteral("dom.security.https_only_mode")},
                        {QStringLiteral("value"), httpsOnly}},
            QVariantMap{{QStringLiteral("name"), QStringLiteral("dom.security.https_first")},
                        {QStringLiteral("value"), true}}};
}

QVariantList EngineMessages::dohPreferences(int protection, const QString &provider,
                                            const QStringList &exceptions)
{
    // MODE_TRROFF, MODE_TRRFIRST, MODE_TRRONLY.
    int mode = 5;
    if (protection == DohSettings::ProtectionIncreased) {
        mode = 2;
    } else if (protection == DohSettings::ProtectionMax) {
        mode = 3;
    }
    return {QVariantMap{{QStringLiteral("name"), QStringLiteral("network.trr.uri")},
                        {QStringLiteral("value"), provider}},
            QVariantMap{{QStringLiteral("name"), QStringLiteral("network.trr.excluded-domains")},
                        {QStringLiteral("value"), exceptions.join(QLatin1Char(','))}},
            QVariantMap{{QStringLiteral("name"), QStringLiteral("network.trr.mode")},
                        {QStringLiteral("value"), mode}}};
}

} // namespace Salama
