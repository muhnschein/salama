// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "SearchSettings.h"

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
const char *const OmnibarTabsKey = "omnibarTabs";
const char *const OmnibarBookmarksKey = "omnibarBookmarks";
const char *const OmnibarHistoryKey = "omnibarHistory";
const char *const OmnibarDownloadsKey = "omnibarDownloads";
// What stands in for the words searched for when a search engine's address template
// is read as an address.
const char *const SearchTermsMarker = "searchTerms";

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

int indexOfEngine(const QString &key)
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

SearchSettings::SearchSettings(QSettings &file, QObject *parent)
    : SettingsSection(file, parent)
{
}

QString SearchSettings::defaultEngine()
{
    return QLatin1String(searchEngines().first().key);
}

QString SearchSettings::engine() const
{
    const QString key = value(SearchEngineKey).toString();
    return indexOfEngine(key) >= 0 ? key : defaultEngine();
}

void SearchSettings::setEngine(const QString &key)
{
    if (indexOfEngine(key) < 0 || key == engine()) {
        return;
    }
    setValue(SearchEngineKey, key);
    emit engineChanged();
}

int SearchSettings::engineIndex() const
{
    return indexOfEngine(engine());
}

void SearchSettings::setEngineIndex(int index)
{
    if (index < 0 || index >= searchEngines().count()) {
        return;
    }
    setEngine(QLatin1String(searchEngines().at(index).key));
}

QStringList SearchSettings::engineNames() const
{
    QStringList names;
    for (const SearchEngine &offered : searchEngines()) {
        names.append(QLatin1String(offered.name));
    }
    return names;
}

QStringList SearchSettings::engineKeys() const
{
    QStringList keys;
    for (const SearchEngine &offered : searchEngines()) {
        keys.append(QLatin1String(offered.key));
    }
    return keys;
}

bool SearchSettings::omnibarTabs() const
{
    return flag(OmnibarTabsKey);
}

void SearchSettings::setOmnibarTabs(bool on)
{
    if (setFlag(OmnibarTabsKey, on)) {
        emit omnibarTabsChanged();
    }
}

bool SearchSettings::omnibarBookmarks() const
{
    return flag(OmnibarBookmarksKey);
}

void SearchSettings::setOmnibarBookmarks(bool on)
{
    if (setFlag(OmnibarBookmarksKey, on)) {
        emit omnibarBookmarksChanged();
    }
}

bool SearchSettings::omnibarHistory() const
{
    return flag(OmnibarHistoryKey);
}

void SearchSettings::setOmnibarHistory(bool on)
{
    if (setFlag(OmnibarHistoryKey, on)) {
        emit omnibarHistoryChanged();
    }
}

bool SearchSettings::omnibarDownloads() const
{
    return flag(OmnibarDownloadsKey);
}

void SearchSettings::setOmnibarDownloads(bool on)
{
    if (setFlag(OmnibarDownloadsKey, on)) {
        emit omnibarDownloadsChanged();
    }
}

QString SearchSettings::searchUrl(const QString &query) const
{
    const QString encoded = QString::fromLatin1(QUrl::toPercentEncoding(query.trimmed()));
    return QString::fromLatin1(searchEngines().at(engineIndex()).urlTemplate).arg(encoded);
}

// The engine's host and path, and its words in the parameter the template puts them in.
// Not the template's text up to the words: an engine is free to add parameters of its
// own ahead of them as it redirects, and to drop "www.". The start page asks this of
// every page in the history, so the templates are read once.
bool SearchSettings::isSearchUrl(const QString &url)
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
QString SearchSettings::displayAddress(const QString &url)
{
    const QUrl parsed(url, QUrl::TolerantMode);
    const QString host = parsed.host();
    return host.isEmpty() ? url : withoutWww(host);
}

QString SearchSettings::addressFor(const QString &text)
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

QString SearchSettings::urlForInput(const QString &input) const
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
bool SearchSettings::isAddress(const QString &input) const
{
    const QString text = input.trimmed();
    return !text.isEmpty() && !addressFor(text).isEmpty();
}

} // namespace Salama
