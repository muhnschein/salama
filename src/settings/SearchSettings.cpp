// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "SearchSettings.h"

#include "search/OpenSearch.h"
#include "search/SearchEngines.h"

#include <QHostAddress>
#include <QRegularExpression>
#include <QUrl>

namespace Salama {

namespace {

const char *const SearchEngineKey = "searchEngine";
const char *const OmnibarTabsKey = "omnibarTabs";
const char *const OmnibarBookmarksKey = "omnibarBookmarks";
const char *const OmnibarHistoryKey = "omnibarHistory";
const char *const OmnibarDownloadsKey = "omnibarDownloads";

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

SearchSettings::SearchSettings(QSettings &file, SearchEngines &engines, QObject *parent)
    : SettingsSection(file, parent)
    , m_engines(engines)
{
    connect(&m_engines, &SearchEngines::engineAdded, this, &SearchSettings::chooseAdded);
    connect(&m_engines, &SearchEngines::enginesChanged, this, &SearchSettings::enginesWereChanged);
}

QString SearchSettings::defaultEngine()
{
    return defaultSearchEngine();
}

QString SearchSettings::engine() const
{
    const QString key = value(SearchEngineKey).toString();
    return m_engines.indexOf(key) >= 0 ? key : defaultEngine();
}

void SearchSettings::setEngine(const QString &key)
{
    if (m_engines.indexOf(key) < 0 || key == engine()) {
        return;
    }
    setValue(SearchEngineKey, key);
    emit engineChanged();
}

int SearchSettings::engineIndex() const
{
    return m_engines.indexOf(engine());
}

void SearchSettings::setEngineIndex(int index)
{
    if (index < 0 || index >= m_engines.count()) {
        return;
    }
    setEngine(m_engines.keyAt(index));
}

void SearchSettings::chooseAdded(const QString &key)
{
    if (m_engines.indexOf(key) >= 0) {
        setValue(SearchEngineKey, key);
    }
}

// Write fallback explicitly: file never names missing engine.
void SearchSettings::enginesWereChanged()
{
    const QString chosen = value(SearchEngineKey).toString();
    if (!chosen.isEmpty() && m_engines.indexOf(chosen) < 0) {
        setValue(SearchEngineKey, defaultEngine());
    }
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
    return OpenSearch::fill(m_engines.templateAt(engineIndex()), query);
}

// No registrable-domain reduction: needs public suffix list, goes stale.
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

    // "host", "host/path", "host:port" sans scheme; space = search.
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

// Same rule as urlForInput(): bar never disagrees with Enter.
bool SearchSettings::isAddress(const QString &input) const
{
    const QString text = input.trimmed();
    return !text.isEmpty() && !addressFor(text).isEmpty();
}

} // namespace Salama
