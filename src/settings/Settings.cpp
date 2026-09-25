// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "Settings.h"

#include <QHostAddress>
#include <QPair>
#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>
#include <QVector>
#include <algorithm>

namespace Salama {

namespace {

const char *const SearchEngineKey = "searchEngine";
const char *const CutoutGuardKey = "cutoutGuard";
const char *const CoverStyleKey = "coverStyle";
const char *const TrackingProtectionKey = "trackingProtection";
const char *const ReaderColorsKey = "readerColors";
const char *const ReaderTypefaceKey = "readerTypeface";
const char *const ReaderTextSizeKey = "readerTextSize";
const char *const OmnibarTabsKey = "omnibarTabs";
const char *const OmnibarBookmarksKey = "omnibarBookmarks";
const char *const OmnibarHistoryKey = "omnibarHistory";
const char *const OmnibarDownloadsKey = "omnibarDownloads";
const char *const RememberHistoryKey = "rememberHistory";
const char *const ClearHistoryOnCloseKey = "clearHistoryOnClose";
const char *const QuickActionKey = "quickAction";
const char *const QuickActionBookmarkKey = "quickActionBookmark";
const char *const QuickActionBookmarkUrlKey = "quickActionBookmarkUrl";
const char *const QuickActionBookmarkTitleKey = "quickActionBookmarkTitle";
const char *const QuickActionIconKey = "quickActionIcon";
const char *const StartPageBlankKey = "startPageBlank";
const char *const StartPageTopSitesKey = "startPageTopSites";
const char *const StartPageBookmarksKey = "startPageBookmarks";
const char *const StartPageRecentKey = "startPageRecent";
// Where an earlier release kept the address of its home page. The start page took the
// home page's place (docs/DECISIONS/0032-start-page.md), and nothing reads it now.
const char *const RetiredHomePageKey = "homePage";
// What stands in for the words searched for when a search engine's address template
// is read as an address.
const char *const SearchTermsMarker = "searchTerms";

// The pictures a bookmark's quick action can wear, drawn in icons/cover/. The star
// last: the bookmarks overview's own glyph is a star, as the menu sheet's Bookmarks is,
// and a bookmark that wore it by default would read as the overview.
const QStringList &quickActionIconNames()
{
    static const QStringList names{
        QStringLiteral("globe"), QStringLiteral("heart"), QStringLiteral("home"),
        QStringLiteral("work"),  QStringLiteral("news"),  QStringLiteral("music"),
        QStringLiteral("shop"),  QStringLiteral("star"),
    };
    return names;
}

struct SearchEngine
{
    const char *key;
    const char *name;
    const char *urlTemplate;
};

const QVector<SearchEngine> &searchEngines()
{
    static const QVector<SearchEngine> engines{
        {"qwant", "Qwant", "https://www.qwant.com/?q=%1"},
        {"ecosia", "Ecosia", "https://www.ecosia.org/search?q=%1"},
        {"startpage", "Startpage", "https://www.startpage.com/do/search?q=%1"},
    };
    return engines;
}

QString withoutWww(const QString &host)
{
    return host.startsWith(QLatin1String("www.")) ? host.mid(4) : host;
}

// Where each engine's results are: its host without "www.", its path, and the parameter
// that carries the words searched for.
struct SearchResults
{
    QString host;
    QString path;
    QString parameter;
};

const QVector<SearchResults> &searchResults()
{
    static const QVector<SearchResults> pages = [] {
        QVector<SearchResults> read;
        for (const SearchEngine &engine : searchEngines()) {
            const QUrl results(QString::fromLatin1(engine.urlTemplate)
                                   .arg(QString::fromLatin1(SearchTermsMarker)));
            for (const QPair<QString, QString> &item : QUrlQuery(results).queryItems()) {
                if (item.second == QLatin1String(SearchTermsMarker)) {
                    read.append(
                        SearchResults{withoutWww(results.host()), results.path(), item.first});
                }
            }
        }
        return read;
    }();
    return pages;
}

int engineIndex(const QString &key)
{
    const QVector<SearchEngine> &engines = searchEngines();
    for (int i = 0; i < engines.count(); ++i) {
        if (QLatin1String(engines.at(i).key) == key) {
            return i;
        }
    }
    return -1;
}

bool isNavigableScheme(const QString &scheme)
{
    return scheme == QLatin1String("http") || scheme == QLatin1String("https") ||
           scheme == QLatin1String("about") || scheme == QLatin1String("file") ||
           scheme == QLatin1String("data");
}

bool isLocalHost(const QString &host)
{
    return host == QLatin1String("localhost") || !QHostAddress(host).isNull();
}

} // namespace

Settings::Settings(const QString &filePath, QObject *parent)
    : QObject(parent)
    , m_settings(filePath, QSettings::IniFormat)
{
    if (m_settings.contains(QLatin1String(RetiredHomePageKey))) {
        m_settings.remove(QLatin1String(RetiredHomePageKey));
    }
}

QString Settings::defaultSearchEngine()
{
    return QLatin1String(searchEngines().first().key);
}

QString Settings::searchEngine() const
{
    const QString key = m_settings.value(QLatin1String(SearchEngineKey)).toString();
    return engineIndex(key) >= 0 ? key : defaultSearchEngine();
}

void Settings::setSearchEngine(const QString &key)
{
    if (engineIndex(key) < 0 || key == searchEngine()) {
        return;
    }
    m_settings.setValue(QLatin1String(SearchEngineKey), key);
    emit searchEngineChanged();
}

int Settings::searchEngineIndex() const
{
    return engineIndex(searchEngine());
}

void Settings::setSearchEngineIndex(int index)
{
    if (index < 0 || index >= searchEngines().count()) {
        return;
    }
    setSearchEngine(QLatin1String(searchEngines().at(index).key));
}

QStringList Settings::searchEngineNames() const
{
    QStringList names;
    for (const SearchEngine &engine : searchEngines()) {
        names.append(QLatin1String(engine.name));
    }
    return names;
}

QStringList Settings::searchEngineKeys() const
{
    QStringList keys;
    for (const SearchEngine &engine : searchEngines()) {
        keys.append(QLatin1String(engine.key));
    }
    return keys;
}

bool Settings::cutoutGuard() const
{
    return m_settings.value(QLatin1String(CutoutGuardKey), true).toBool();
}

void Settings::setCutoutGuard(bool cutoutGuard)
{
    if (cutoutGuard == this->cutoutGuard()) {
        return;
    }
    m_settings.setValue(QLatin1String(CutoutGuardKey), cutoutGuard);
    emit cutoutGuardChanged();
}

int Settings::coverStyle() const
{
    const int stored = m_settings.value(QLatin1String(CoverStyleKey), CoverLightning).toInt();
    if (stored < CoverLightning || stored > CoverLatestTab) {
        return CoverLightning;
    }
    return stored;
}

void Settings::setCoverStyle(int style)
{
    if (style < CoverLightning || style > CoverLatestTab || style == coverStyle()) {
        return;
    }
    m_settings.setValue(QLatin1String(CoverStyleKey), style);
    emit coverStyleChanged();
}

int Settings::trackingProtection() const
{
    const int stored =
        m_settings.value(QLatin1String(TrackingProtectionKey), TrackingProtectionStandard).toInt();
    if (stored < TrackingProtectionOff || stored > TrackingProtectionStrict) {
        return TrackingProtectionStandard;
    }
    return stored;
}

void Settings::setTrackingProtection(int level)
{
    if (level < TrackingProtectionOff || level > TrackingProtectionStrict ||
        level == trackingProtection()) {
        return;
    }
    m_settings.setValue(QLatin1String(TrackingProtectionKey), level);
    emit trackingProtectionChanged();
}

int Settings::readerColors() const
{
    const int stored = m_settings.value(QLatin1String(ReaderColorsKey), ReaderAmbience).toInt();
    return stored < ReaderAmbience || stored > ReaderDark ? int(ReaderAmbience) : stored;
}

void Settings::setReaderColors(int colors)
{
    if (colors < ReaderAmbience || colors > ReaderDark || colors == readerColors()) {
        return;
    }
    m_settings.setValue(QLatin1String(ReaderColorsKey), colors);
    emit readerColorsChanged();
}

int Settings::readerTypeface() const
{
    const int stored = m_settings.value(QLatin1String(ReaderTypefaceKey), ReaderSansSerif).toInt();
    return stored < ReaderSansSerif || stored > ReaderSerif ? int(ReaderSansSerif) : stored;
}

void Settings::setReaderTypeface(int typeface)
{
    if (typeface < ReaderSansSerif || typeface > ReaderSerif || typeface == readerTypeface()) {
        return;
    }
    m_settings.setValue(QLatin1String(ReaderTypefaceKey), typeface);
    emit readerTypefaceChanged();
}

int Settings::readerTextSize() const
{
    const int stored =
        m_settings.value(QLatin1String(ReaderTextSizeKey), ReaderTextSizeDefault).toInt();
    return stored < ReaderTextSizeMin || stored > ReaderTextSizeMax ? ReaderTextSizeDefault
                                                                    : stored;
}

void Settings::setReaderTextSize(int size)
{
    if (size < ReaderTextSizeMin || size > ReaderTextSizeMax || size == readerTextSize()) {
        return;
    }
    m_settings.setValue(QLatin1String(ReaderTextSizeKey), size);
    emit readerTextSizeChanged();
}

bool Settings::flag(const char *key, bool initially) const
{
    return m_settings.value(QLatin1String(key), initially).toBool();
}

// Whether the value changed, so the caller knows to say so.
bool Settings::setFlag(const char *key, bool on, bool initially)
{
    if (on == flag(key, initially)) {
        return false;
    }
    m_settings.setValue(QLatin1String(key), on);
    return true;
}

bool Settings::rememberHistory() const
{
    return flag(RememberHistoryKey);
}

void Settings::setRememberHistory(bool on)
{
    if (setFlag(RememberHistoryKey, on)) {
        emit rememberHistoryChanged();
    }
}

bool Settings::clearHistoryOnClose() const
{
    return flag(ClearHistoryOnCloseKey, false);
}

void Settings::setClearHistoryOnClose(bool on)
{
    if (setFlag(ClearHistoryOnCloseKey, on, false)) {
        emit clearHistoryOnCloseChanged();
    }
}

bool Settings::omnibarTabs() const
{
    return flag(OmnibarTabsKey);
}

void Settings::setOmnibarTabs(bool on)
{
    if (setFlag(OmnibarTabsKey, on)) {
        emit omnibarTabsChanged();
    }
}

bool Settings::omnibarBookmarks() const
{
    return flag(OmnibarBookmarksKey);
}

void Settings::setOmnibarBookmarks(bool on)
{
    if (setFlag(OmnibarBookmarksKey, on)) {
        emit omnibarBookmarksChanged();
    }
}

bool Settings::omnibarHistory() const
{
    return flag(OmnibarHistoryKey);
}

void Settings::setOmnibarHistory(bool on)
{
    if (setFlag(OmnibarHistoryKey, on)) {
        emit omnibarHistoryChanged();
    }
}

bool Settings::omnibarDownloads() const
{
    return flag(OmnibarDownloadsKey);
}

void Settings::setOmnibarDownloads(bool on)
{
    if (setFlag(OmnibarDownloadsKey, on)) {
        emit omnibarDownloadsChanged();
    }
}

int Settings::quickAction() const
{
    const int stored = m_settings.value(QLatin1String(QuickActionKey), QuickActionSearch).toInt();
    return stored < QuickActionNone || stored > QuickActionHistory ? int(QuickActionSearch)
                                                                   : stored;
}

void Settings::setQuickAction(int action)
{
    if (action < QuickActionNone || action > QuickActionHistory || action == quickAction()) {
        return;
    }
    m_settings.setValue(QLatin1String(QuickActionKey), action);
    emit quickActionChanged();
}

int Settings::quickActionBookmark() const
{
    const int stored = m_settings.value(QLatin1String(QuickActionBookmarkKey), 0).toInt();
    return std::max(stored, 0);
}

QString Settings::quickActionBookmarkUrl() const
{
    return m_settings.value(QLatin1String(QuickActionBookmarkUrlKey)).toString();
}

QString Settings::quickActionBookmarkTitle() const
{
    return m_settings.value(QLatin1String(QuickActionBookmarkTitleKey)).toString();
}

void Settings::setQuickActionBookmark(int id, const QString &url, const QString &title)
{
    if (id < 0) {
        return;
    }
    // No bookmark has no address and no title either: what is forgotten is forgotten
    // whole, and nothing is left to find it again by.
    const QString keptUrl = id == 0 ? QString() : url;
    const QString keptTitle = id == 0 ? QString() : title;
    if (id == quickActionBookmark() && keptUrl == quickActionBookmarkUrl() &&
        keptTitle == quickActionBookmarkTitle()) {
        return;
    }
    m_settings.setValue(QLatin1String(QuickActionBookmarkKey), id);
    m_settings.setValue(QLatin1String(QuickActionBookmarkUrlKey), keptUrl);
    m_settings.setValue(QLatin1String(QuickActionBookmarkTitleKey), keptTitle);
    emit quickActionBookmarkChanged();
}

QString Settings::quickActionIcon() const
{
    const QString stored = m_settings.value(QLatin1String(QuickActionIconKey)).toString();
    return quickActionIconNames().contains(stored) ? stored : quickActionIconNames().first();
}

void Settings::setQuickActionIcon(const QString &name)
{
    if (!quickActionIconNames().contains(name) || name == quickActionIcon()) {
        return;
    }
    m_settings.setValue(QLatin1String(QuickActionIconKey), name);
    emit quickActionIconChanged();
}

QStringList Settings::quickActionIcons() const
{
    return quickActionIconNames();
}

// The home screen draws a cover action's picture from its file as it is, unscaled, so
// the picture has to be drawn at the size it is shown at: icons/render.sh draws each
// glyph at every size from 32 to 64 pixels in steps of 8, which is where Silica's small
// icon falls on the phones this is for, and the size asked for is snapped to the
// nearest of those -- halfway rounds up, as Math.round() does in QML -- and kept
// within them. Bounded before rounding, so no size, however wild, has no int to round
// to.
QString Settings::coverIconPath(const QString &name, qreal iconSize, bool onDark)
{
    const int size = qRound(qBound(qreal(32), iconSize, qreal(64)) / 8) * 8;
    return QStringLiteral("art/cover/%1-%2-%3.png")
        .arg(name)
        .arg(size)
        .arg(onDark ? QStringLiteral("white") : QStringLiteral("black"));
}

qreal Settings::pageZoom(qreal pixelRatio)
{
    return qRound(pixelRatio * 1.75 / 0.5) * 0.5;
}

bool Settings::startPageBlank() const
{
    return flag(StartPageBlankKey, false);
}

void Settings::setStartPageBlank(bool blank)
{
    if (setFlag(StartPageBlankKey, blank, false)) {
        emit startPageChanged();
    }
}

bool Settings::startPageTopSites() const
{
    return flag(StartPageTopSitesKey);
}

void Settings::setStartPageTopSites(bool shown)
{
    if (setFlag(StartPageTopSitesKey, shown)) {
        emit startPageChanged();
    }
}

bool Settings::startPageBookmarks() const
{
    return flag(StartPageBookmarksKey);
}

void Settings::setStartPageBookmarks(bool shown)
{
    if (setFlag(StartPageBookmarksKey, shown)) {
        emit startPageChanged();
    }
}

bool Settings::startPageRecent() const
{
    return flag(StartPageRecentKey);
}

void Settings::setStartPageRecent(bool shown)
{
    if (setFlag(StartPageRecentKey, shown)) {
        emit startPageChanged();
    }
}

QString Settings::searchUrl(const QString &query) const
{
    const QString encoded = QString::fromLatin1(QUrl::toPercentEncoding(query.trimmed()));
    return QString::fromLatin1(searchEngines().at(searchEngineIndex()).urlTemplate).arg(encoded);
}

// The engine's host and path, and its words in the parameter the template puts them in.
// Not the template's text up to the words: an engine is free to add parameters of its
// own ahead of them as it redirects, and to drop "www.". The start page asks this of
// every page in the history, so the templates are read once.
bool Settings::isSearchUrl(const QString &url)
{
    const QUrl page(url, QUrl::TolerantMode);
    const QString host = withoutWww(page.host());
    const QVector<SearchResults> &pages = searchResults();
    return std::any_of(pages.cbegin(), pages.cend(), [&](const SearchResults &results) {
        return host == results.host && page.path() == results.path &&
               QUrlQuery(page).hasQueryItem(results.parameter);
    });
}

// What the bar shows when the address is not being edited: the host, without the
// scheme, without "www." and without the path -- the part that says whose page this
// is. The field shows the whole url again the moment it is tapped, so nothing is
// hidden from the person who asks for it.
//
// The host is taken as the engine reports it rather than reduced to a registrable
// domain: "docs.example.com" and "example.com" are different sites, and deciding
// where the site ends needs the public suffix list, which is not worth carrying and
// would be wrong the day it goes stale. The port is left off entirely: 80 and 443 say
// nothing, and the rest are noise in a bar this narrow. Anything without a host --
// about:, data:, file: -- is shown as it is.
QString Settings::displayAddress(const QString &url)
{
    const QUrl parsed(url, QUrl::TolerantMode);
    const QString host = parsed.host();
    return host.isEmpty() ? url : withoutWww(host);
}

QString Settings::addressFor(const QString &text)
{
    const QUrl typed(text, QUrl::TolerantMode);
    if (typed.isValid() && isNavigableScheme(typed.scheme())) {
        return typed.toString();
    }

    // "host", "host/path", "host:port" without a scheme; a space means a search.
    static const QRegularExpression hostLike(
        QStringLiteral("^[^\\s/?#:]+(:[0-9]{1,5})?(/[^\\s]*)?$"));
    if (hostLike.match(text).hasMatch()) {
        const QString host = text.section(QLatin1Char('/'), 0, 0).section(QLatin1Char(':'), 0, 0);
        const bool looksLikeHost = host.contains(QLatin1Char('.')) || isLocalHost(host);
        if (looksLikeHost) {
            const QString scheme =
                isLocalHost(host) ? QStringLiteral("http://") : QStringLiteral("https://");
            return QUrl(scheme + text, QUrl::TolerantMode).toString();
        }
    }
    return {};
}

QString Settings::urlForInput(const QString &input) const
{
    const QString text = input.trimmed();
    if (text.isEmpty()) {
        return {};
    }
    const QString address = addressFor(text);
    return address.isEmpty() ? searchUrl(text) : address;
}

// Asked of the same rule urlForInput() follows, so the address bar never offers to go
// to an address that Enter would have searched for, nor the other way round.
bool Settings::isAddress(const QString &input) const
{
    const QString text = input.trimmed();
    return !text.isEmpty() && !addressFor(text).isEmpty();
}

} // namespace Salama
