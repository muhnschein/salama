// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

namespace Salama {

// All engine-facing strings, so QML carries no engine quirk.
class EngineMessages : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString clearPrivateDataTopic READ clearPrivateDataTopic CONSTANT)
    Q_PROPERTY(QString cookiesAndSiteDataPayload READ cookiesAndSiteDataPayload CONSTANT)
    Q_PROPERTY(QString cachePayload READ cachePayload CONSTANT)
    Q_PROPERTY(QString memoryPressureTopic READ memoryPressureTopic CONSTANT)
    Q_PROPERTY(QString heapMinimizePayload READ heapMinimizePayload CONSTANT)
    Q_PROPERTY(QString faviconScript READ faviconScript CONSTANT)
    Q_PROPERTY(QString themeColorScript READ themeColorScript CONSTANT)
    Q_PROPERTY(QString viewportScript READ viewportScript CONSTANT)
    Q_PROPERTY(QString findMessage READ findMessage CONSTANT)
    Q_PROPERTY(QString findResultMessage READ findResultMessage CONSTANT)
    Q_PROPERTY(QString searchOfferedMessage READ searchOfferedMessage CONSTANT)
    Q_PROPERTY(QString contextMenuMessage READ contextMenuMessage CONSTANT)

public:
    explicit EngineMessages(QObject *parent = nullptr);

    QString clearPrivateDataTopic() const;
    QString cookiesAndSiteDataPayload() const;
    QString cachePayload() const;

    QString memoryPressureTopic() const;
    QString heapMinimizePayload() const;

    // Asked of page: WebView has no favicon property. All scripts here are function bodies and
    // must `return` (embedhelper.js wraps them in `new content.Function`).
    QString faviconScript() const;

    Q_INVOKABLE QString resolveFavicon(const QString &pageUrl, const QString &href) const;
    Q_INVOKABLE QString defaultFavicon(const QString &pageUrl) const;

    // Asked of page: Harbour WebView gets no theme-color message.
    QString themeColorScript() const;

    // Reads rgb() and #rrggbbaa, which QColor can't. Alpha dropped: band must be opaque.
    Q_INVOKABLE static QString themeColor(const QString &value);

    // Asked of page: Harbour WebView gets no viewport-fit.
    QString viewportScript() const;

    Q_INVOKABLE static bool coversCutout(const QString &viewport);

    // findResultMessage heard only after addMessageListener.
    QString findMessage() const;
    QString findResultMessage() const;

    // Needs addMessageListener.
    QString searchOfferedMessage() const;

    Q_INVOKABLE static QVariantMap searchOffered(const QVariant &data);

    // Platform WebView already listens, no registration needed.
    QString contextMenuMessage() const;

    // All keys present. link "" for javascript:; kind "page", "app" (isExternalUrl) or "".
    Q_INVOKABLE static QVariantMap linkTarget(const QVariant &data);

    // Empty text ends search, else last match stays highlighted.
    Q_INVOKABLE QVariantMap findRequest(const QString &text, bool again, bool backwards) const;

    // nsITypeAheadFind FOUND 0 / WRAPPED 2 count as found.
    Q_INVOKABLE static bool findFound(const QVariant &data);

    // Every level sets same prefs in same order: switching leaves no residue. Out of range =
    // Standard. At Off, cookies = reader's SitePermissionSettings::Cookies.
    Q_INVOKABLE static QVariantList trackingProtectionPreferences(int level, int cookies);

    Q_INVOKABLE static QVariantList sitePermissionPreferences(bool popupsAllowed,
                                                              bool locationBlocked,
                                                              bool cameraBlocked,
                                                              bool microphoneBlocked);

    // ui.systemUsesDarkTheme: only prefers-color-scheme control Harbour leaves.
    Q_INVOKABLE static QVariantList websiteColorPreferences(int colors, bool darkAmbience);

    // GPC needs both prefs on (one off in Gecko defaults). DNT forced off.
    Q_INVOKABLE static QVariantList contentPreferences(bool globalPrivacyControl, bool javascript);

    Q_INVOKABLE static QVariantList httpsOnlyPreferences(bool httpsOnly);

    // trr.mode: Off 5, Increased 2 (fallback), Max 3 (no fallback). Out of range = Off.
    Q_INVOKABLE static QVariantList dohPreferences(int protection, const QString &provider,
                                                   const QStringList &exceptions);
};

} // namespace Salama
