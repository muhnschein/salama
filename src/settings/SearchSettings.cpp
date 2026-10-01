// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "SearchSettings.h"

#include "search/OpenSearch.h"

#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPair>
#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>
#include <QVector>
#include <algorithm>

namespace Salama {

namespace {

const char *const SearchEngineKey = "searchEngine";
// The added engines and the offers as JSON arrays of objects: records do not go in a
// file of keys, and a name or an address may hold any character an INI line would
// take apart.
const char *const AddedEnginesKey = "searchEnginesAdded";
const char *const FoundEnginesKey = "searchEnginesFound";
const char *const OmnibarTabsKey = "omnibarTabs";
const char *const OmnibarBookmarksKey = "omnibarBookmarks";
const char *const OmnibarHistoryKey = "omnibarHistory";
const char *const OmnibarDownloadsKey = "omnibarDownloads";
// What stands for the words searched for when a search engine's address template
// is read as an address.
const char *const SearchTermsWord = "searchTerms";
// The start of an added engine's key: the built-in keys are plain words, so none of
// these can be one of them.
const char *const AddedKeyPrefix = "added-";

struct BuiltInEngine
{
    const char *key;
    const char *name;
    const char *urlTemplate;
};

const QVector<BuiltInEngine> &builtInEngines()
{
    static const QVector<BuiltInEngine> engines{
        {"qwant", "Qwant", "https://www.qwant.com/?q={searchTerms}"},
        {"ecosia", "Ecosia", "https://www.ecosia.org/search?q={searchTerms}"},
        {"startpage", "Startpage", "https://www.startpage.com/do/search?q={searchTerms}"},
    };
    return engines;
}

QString withoutWww(const QString &host)
{
    return host.startsWith(QLatin1String("www.")) ? host.mid(4) : host;
}

bool isWebUrl(const QUrl &url)
{
    return (url.scheme() == QLatin1String("http") || url.scheme() == QLatin1String("https")) &&
           !url.host().isEmpty();
}

// "Wikipedia (en)" as the middle of a key: letters and digits, the rest as single dashes.
QString slug(const QString &name)
{
    QString text;
    for (const QChar letter : name.toLower()) {
        if (letter.isLetterOrNumber()) {
            text += letter;
        } else if (!text.isEmpty() && !text.endsWith(QLatin1Char('-'))) {
            text += QLatin1Char('-');
        }
    }
    while (text.endsWith(QLatin1Char('-'))) {
        text.chop(1);
    }
    return text.isEmpty() ? QStringLiteral("engine") : text;
}

QString textOf(const QJsonObject &record, const char *field)
{
    return record.value(QLatin1String(field)).toString();
}

QJsonArray recordsIn(const QVariant &stored)
{
    return QJsonDocument::fromJson(stored.toString().toUtf8()).array();
}

QString jsonOf(const QJsonArray &records)
{
    return QString::fromUtf8(QJsonDocument(records).toJson(QJsonDocument::Compact));
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
    readStored();
}

// What the file says, kept as it was read: the file is one a user can edit, so a record
// that is no engine is passed over rather than trusted, and nothing else writes these
// keys, so what is held here is what is there.
void SearchSettings::readStored()
{
    for (const auto &entry : recordsIn(value(AddedEnginesKey))) {
        const QJsonObject record = entry.toObject();
        const Engine engine{textOf(record, "key"), textOf(record, "name"),
                            textOf(record, "template"), textOf(record, "host")};
        if (!engine.key.isEmpty() && !engine.name.isEmpty() &&
            OpenSearch::isTemplate(engine.urlTemplate) && indexOfEngine(engine.key) < 0) {
            m_added.append(engine);
        }
    }
    for (const auto &entry : recordsIn(value(FoundEnginesKey))) {
        const QJsonObject record = entry.toObject();
        const Found offer{textOf(record, "title"), textOf(record, "href"), textOf(record, "host")};
        const bool kept = std::any_of(m_found.cbegin(), m_found.cend(),
                                      [&](const Found &known) { return known.href == offer.href; });
        if (!offer.title.isEmpty() && isWebUrl(QUrl(offer.href)) && !kept) {
            m_found.append(offer);
        }
    }
    rebuildResults();
}

void SearchSettings::storeAdded()
{
    QJsonArray records;
    for (const Engine &engine : m_added) {
        records.append(QJsonObject{{QStringLiteral("key"), engine.key},
                                   {QStringLiteral("name"), engine.name},
                                   {QStringLiteral("template"), engine.urlTemplate},
                                   {QStringLiteral("host"), engine.host}});
    }
    if (records.isEmpty()) {
        remove(AddedEnginesKey);
    } else {
        setValue(AddedEnginesKey, jsonOf(records));
    }
}

void SearchSettings::storeFound()
{
    QJsonArray records;
    for (const Found &offer : m_found) {
        records.append(QJsonObject{{QStringLiteral("title"), offer.title},
                                   {QStringLiteral("href"), offer.href},
                                   {QStringLiteral("host"), offer.host}});
    }
    if (records.isEmpty()) {
        remove(FoundEnginesKey);
    } else {
        setValue(FoundEnginesKey, jsonOf(records));
    }
}

QString SearchSettings::defaultEngine()
{
    return QLatin1String(builtInEngines().first().key);
}

QVector<SearchSettings::Engine> SearchSettings::engines() const
{
    QVector<Engine> all;
    for (const BuiltInEngine &builtIn : builtInEngines()) {
        all.append(Engine{QLatin1String(builtIn.key), QLatin1String(builtIn.name),
                          QLatin1String(builtIn.urlTemplate), QString()});
    }
    return all + m_added;
}

int SearchSettings::indexOfEngine(const QString &key) const
{
    const QVector<Engine> all = engines();
    for (int i = 0; i < all.count(); ++i) {
        if (all.at(i).key == key) {
            return i;
        }
    }
    return -1;
}

// An engine is told apart by its name, which is what the lists show: two of one name
// would be two rows no one could choose between.
bool SearchSettings::hasEngineNamed(const QString &name) const
{
    const QVector<Engine> all = engines();
    return std::any_of(all.cbegin(), all.cend(), [&](const Engine &engine) {
        return engine.name.compare(name, Qt::CaseInsensitive) == 0;
    });
}

QString SearchSettings::uniqueKey(const QString &name) const
{
    const QString base = QLatin1String(AddedKeyPrefix) + slug(name);
    QString key = base;
    for (int copy = 2; indexOfEngine(key) >= 0; ++copy) {
        key = base + QLatin1Char('-') + QString::number(copy);
    }
    return key;
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
    const QVector<Engine> all = engines();
    if (index < 0 || index >= all.count()) {
        return;
    }
    setEngine(all.at(index).key);
}

QStringList SearchSettings::engineNames() const
{
    QStringList names;
    for (const Engine &offered : engines()) {
        names.append(offered.name);
    }
    return names;
}

QStringList SearchSettings::engineKeys() const
{
    QStringList keys;
    for (const Engine &offered : engines()) {
        keys.append(offered.key);
    }
    return keys;
}

QStringList SearchSettings::engineHosts() const
{
    QStringList hosts;
    for (const Engine &offered : engines()) {
        hosts.append(offered.host);
    }
    return hosts;
}

int SearchSettings::addedCount() const
{
    return m_added.count();
}

QVariantList SearchSettings::foundEngines() const
{
    QVariantList offers;
    for (const Found &offer : m_found) {
        offers.append(QVariantMap{{QStringLiteral("title"), offer.title},
                                  {QStringLiteral("href"), offer.href},
                                  {QStringLiteral("host"), offer.host}});
    }
    return offers;
}

// The page says what it has twice over if it has two links, and again on every visit:
// what is on offer already, by title or by address, is not kept again, and neither is
// what the browser has under that name. There is no private mode to keep it from
// (docs/DECISIONS/0019-no-private-tabs.md), so every page is listened to.
bool SearchSettings::offerEngine(const QString &title, const QString &href, const QString &host)
{
    const QString name = title.trimmed();
    const QString address = href.trimmed();
    const QUrl url(address, QUrl::TolerantMode);
    if (name.isEmpty() || !isWebUrl(url) || hasEngineNamed(name)) {
        return false;
    }
    const bool known = std::any_of(m_found.cbegin(), m_found.cend(), [&](const Found &offer) {
        return offer.href == address || offer.title.compare(name, Qt::CaseInsensitive) == 0;
    });
    if (known) {
        return false;
    }
    const QString site = host.trimmed();
    m_found.append(Found{name, address, site.isEmpty() ? withoutWww(url.host()) : site});
    storeFound();
    emit foundChanged();
    return true;
}

bool SearchSettings::addFoundEngine(const QString &href, const QString &description)
{
    const auto *const offer =
        std::find_if(m_found.cbegin(), m_found.cend(),
                     [&](const Found &found) { return found.href == href.trimmed(); });
    if (offer == m_found.cend()) {
        return false;
    }
    const OpenSearchEngine read = OpenSearch::parse(description);
    // A description may leave its name out; the title the page gave it stands in.
    const QString name = read.name.isEmpty() ? offer->title : read.name;
    if (!read.isValid() || hasEngineNamed(name)) {
        return false;
    }
    const Engine added{uniqueKey(name), name, read.urlTemplate, offer->host};
    m_added.append(added);
    m_found.remove(static_cast<int>(offer - m_found.cbegin()));
    setValue(SearchEngineKey, added.key);
    storeAdded();
    storeFound();
    enginesWereChanged();
    emit foundChanged();
    return true;
}

void SearchSettings::forgetFoundEngine(const QString &href)
{
    for (int i = 0; i < m_found.count(); ++i) {
        if (m_found.at(i).href == href.trimmed()) {
            m_found.remove(i);
            storeFound();
            emit foundChanged();
            return;
        }
    }
}

void SearchSettings::removeAddedEngine(const QString &key)
{
    for (int i = 0; i < m_added.count(); ++i) {
        if (m_added.at(i).key != key) {
            continue;
        }
        const bool chosen = engine() == key;
        m_added.remove(i);
        storeAdded();
        if (chosen) {
            resetEngine();
        }
        enginesWereChanged();
        return;
    }
}

void SearchSettings::removeAddedEngines()
{
    const bool hadAdded = !m_added.isEmpty();
    const bool hadFound = !m_found.isEmpty();
    const bool chosen = indexOfEngine(engine()) >= builtInEngines().count();
    m_added.clear();
    m_found.clear();
    storeAdded();
    storeFound();
    if (chosen) {
        resetEngine();
    }
    if (hadAdded) {
        enginesWereChanged();
    }
    if (hadFound) {
        emit foundChanged();
    }
}

// Written rather than left to engine()'s fallback, so that the file never names an
// engine that is not in it.
void SearchSettings::resetEngine()
{
    setValue(SearchEngineKey, defaultEngine());
}

void SearchSettings::enginesWereChanged()
{
    rebuildResults();
    emit enginesChanged();
    emit engineChanged();
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
    return OpenSearch::fill(engines().at(engineIndex()).urlTemplate, query);
}

// Where an engine's results are: its host without "www.", and either its path and the
// parameter that carries the words, or -- when the template has the words in the path --
// what stands before them and after them. False for an address that says neither, and
// for one that would take in every page of its site ("https://example.org/{searchTerms}").
bool SearchSettings::readResults(const QString &urlTemplate, Results &results)
{
    QString probed = urlTemplate;
    probed.replace(OpenSearch::marker(), QLatin1String(SearchTermsWord));
    const QUrl page(probed, QUrl::TolerantMode);
    results.host = withoutWww(page.host());
    results.path = page.path();
    if (results.host.isEmpty()) {
        return false;
    }
    for (const QPair<QString, QString> &item : QUrlQuery(page).queryItems()) {
        if (item.second == QLatin1String(SearchTermsWord)) {
            results.parameter = item.first;
            return true;
        }
    }
    const int at = results.path.indexOf(QLatin1String(SearchTermsWord));
    if (at <= 1) {
        return false;
    }
    results.pathSuffix = results.path.mid(at + QLatin1String(SearchTermsWord).size());
    results.path = results.path.left(at);
    return true;
}

// The asking is of every page in the history, so what each engine's results look like
// is read once, here, and again when the engines change.
void SearchSettings::rebuildResults()
{
    m_results.clear();
    for (const Engine &offered : engines()) {
        Results results;
        if (readResults(offered.urlTemplate, results)) {
            m_results.append(results);
        }
    }
}

// The engine's host and path, and its words in the parameter the template puts them in.
// Not the template's text up to the words: an engine is free to add parameters of its
// own ahead of them as it redirects, and to drop "www.".
bool SearchSettings::isSearchUrl(const QString &url) const
{
    const QUrl page(url, QUrl::TolerantMode);
    const QString host = withoutWww(page.host());
    const QString path = page.path();
    return std::any_of(m_results.cbegin(), m_results.cend(), [&](const Results &results) {
        if (host != results.host) {
            return false;
        }
        if (!results.parameter.isEmpty()) {
            return path == results.path && QUrlQuery(page).hasQueryItem(results.parameter);
        }
        return path.size() > results.path.size() + results.pathSuffix.size() &&
               path.startsWith(results.path) && path.endsWith(results.pathSuffix);
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
