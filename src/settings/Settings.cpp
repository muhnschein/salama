// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "Settings.h"

#include <QHostAddress>
#include <QRegularExpression>
#include <QUrl>
#include <QVector>
#include <algorithm>

namespace Salama {

namespace {

const char *const HomePageKey = "homePage";
const char *const SearchEngineKey = "searchEngine";
const char *const DesktopModeKey = "desktopMode";
const char *const CutoutGuardKey = "cutoutGuard";
const char *const CoverStyleKey = "coverStyle";
const char *const LiveTabLimitKey = "liveTabLimit";
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

// Jolla's browser keeps five pages live and reloads the rest on return; the same
// five here, with a way to ask for fewer, more, or all of them.
const QVector<int> &liveTabLimits()
{
    static const QVector<int> limits{3, 5, 10, 0};
    return limits;
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
}

QString Settings::defaultHomePage()
{
    return QStringLiteral("https://www.qwant.com/");
}

QString Settings::defaultSearchEngine()
{
    return QLatin1String(searchEngines().first().key);
}

QString Settings::homePage() const
{
    return m_settings.value(QLatin1String(HomePageKey), defaultHomePage()).toString();
}

void Settings::setHomePage(const QString &url)
{
    const QString value = url.trimmed().isEmpty() ? defaultHomePage() : url.trimmed();
    if (value == homePage()) {
        return;
    }
    m_settings.setValue(QLatin1String(HomePageKey), value);
    emit homePageChanged();
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

bool Settings::desktopMode() const
{
    return m_settings.value(QLatin1String(DesktopModeKey), false).toBool();
}

void Settings::setDesktopMode(bool desktopMode)
{
    if (desktopMode == this->desktopMode()) {
        return;
    }
    m_settings.setValue(QLatin1String(DesktopModeKey), desktopMode);
    emit desktopModeChanged();
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
    const int stored = m_settings.value(QLatin1String(CoverStyleKey), CoverEveryTab).toInt();
    if (stored < CoverIconOnly || stored > CoverEveryTab) {
        return CoverEveryTab;
    }
    return stored;
}

void Settings::setCoverStyle(int style)
{
    if (style < CoverIconOnly || style > CoverEveryTab || style == coverStyle()) {
        return;
    }
    m_settings.setValue(QLatin1String(CoverStyleKey), style);
    emit coverStyleChanged();
}

int Settings::defaultLiveTabLimit()
{
    return 5;
}

int Settings::liveTabLimit() const
{
    const int stored =
        m_settings.value(QLatin1String(LiveTabLimitKey), defaultLiveTabLimit()).toInt();
    return liveTabLimits().contains(stored) ? stored : defaultLiveTabLimit();
}

int Settings::liveTabLimitIndex() const
{
    return liveTabLimits().indexOf(liveTabLimit());
}

void Settings::setLiveTabLimitIndex(int index)
{
    if (index < 0 || index >= liveTabLimits().count() || index == liveTabLimitIndex()) {
        return;
    }
    m_settings.setValue(QLatin1String(LiveTabLimitKey), liveTabLimits().at(index));
    emit liveTabLimitChanged();
}

QVariantList Settings::liveTabLimitChoices() const
{
    QVariantList choices;
    for (int limit : liveTabLimits()) {
        choices.append(limit);
    }
    return choices;
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

QString Settings::searchUrl(const QString &query) const
{
    const QString encoded = QString::fromLatin1(QUrl::toPercentEncoding(query.trimmed()));
    return QString::fromLatin1(searchEngines().at(searchEngineIndex()).urlTemplate).arg(encoded);
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
    QString host = parsed.host();
    if (host.isEmpty()) {
        return url;
    }
    if (host.startsWith(QLatin1String("www."))) {
        host = host.mid(4);
    }
    return host;
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
