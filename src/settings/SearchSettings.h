// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "SettingsSection.h"

#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVector>

namespace Salama {

// Settings > Search: the search engine, and the sources the address bar's suggestions
// are drawn from. Also owns the address-bar heuristics, because what typed text means
// depends on the search engine.
//
// The engines are the three built in and the ones added from what sites offered as the
// pages were browsed: each page says, through the engine, that it has a search
// (offerEngine()), the offers are kept until one is taken up (addFoundEngine()) or
// forgotten, and what is taken up joins the built-in ones
// (docs/DECISIONS/0041-search-engines-found.md).
class SearchSettings : public SettingsSection
{
    Q_OBJECT
    Q_PROPERTY(QString engine READ engine WRITE setEngine NOTIFY engineChanged)
    Q_PROPERTY(int engineIndex READ engineIndex WRITE setEngineIndex NOTIFY engineChanged)
    // The built-in engines first, then the added ones in the order they were added; a
    // list that changes as one is added or removed. engineHosts is the site each added
    // engine came from, empty for a built-in one.
    Q_PROPERTY(QStringList engineNames READ engineNames NOTIFY enginesChanged)
    Q_PROPERTY(QStringList engineKeys READ engineKeys NOTIFY enginesChanged)
    Q_PROPERTY(QStringList engineHosts READ engineHosts NOTIFY enginesChanged)
    Q_PROPERTY(int addedCount READ addedCount NOTIFY enginesChanged)
    // What sites have offered and nothing has taken up: a list of {title, href, host}.
    Q_PROPERTY(QVariantList foundEngines READ foundEngines NOTIFY foundChanged)
    // Which sources the address bar's suggestions are drawn from, each on unless it is
    // switched off (docs/DECISIONS/0027-omnibar.md).
    Q_PROPERTY(bool omnibarTabs READ omnibarTabs WRITE setOmnibarTabs NOTIFY omnibarTabsChanged)
    Q_PROPERTY(bool omnibarBookmarks READ omnibarBookmarks WRITE setOmnibarBookmarks NOTIFY
                   omnibarBookmarksChanged)
    Q_PROPERTY(bool omnibarHistory READ omnibarHistory WRITE setOmnibarHistory NOTIFY
                   omnibarHistoryChanged)
    Q_PROPERTY(bool omnibarDownloads READ omnibarDownloads WRITE setOmnibarDownloads NOTIFY
                   omnibarDownloadsChanged)

public:
    explicit SearchSettings(QSettings &file, QObject *parent = nullptr);

    QString engine() const;
    void setEngine(const QString &key);
    int engineIndex() const;
    void setEngineIndex(int index);
    QStringList engineNames() const;
    QStringList engineKeys() const;
    QStringList engineHosts() const;
    int addedCount() const;
    QVariantList foundEngines() const;
    static QString defaultEngine();

    // A site offers a search: the title it gives it, the address of its OpenSearch
    // description and the host of the page that offered it. Kept to be taken up unless it
    // is one already: an engine of that title is on offer, or the offer is already
    // kept. Answers whether it was kept.
    Q_INVOKABLE bool offerEngine(const QString &title, const QString &href, const QString &host);
    // Takes up a kept offer with the description fetched from its address: the engine joins
    // the others, is chosen, and is no longer an offer. Answers false, and keeps the
    // offer, when the text is no description of a search the browser can use or its
    // name is that of an engine already on offer.
    Q_INVOKABLE bool addFoundEngine(const QString &href, const QString &description);
    Q_INVOKABLE void forgetFoundEngine(const QString &href);
    // An added engine removed; a built-in one is not removable. The first built-in engine
    // takes its place if it was the one chosen.
    Q_INVOKABLE void removeAddedEngine(const QString &key);
    // Every added engine removed, and every offer forgotten.
    Q_INVOKABLE void removeAddedEngines();

    bool omnibarTabs() const;
    void setOmnibarTabs(bool on);
    bool omnibarBookmarks() const;
    void setOmnibarBookmarks(bool on);
    bool omnibarHistory() const;
    void setOmnibarHistory(bool on);
    bool omnibarDownloads() const;
    void setOmnibarDownloads(bool on);

    Q_INVOKABLE QString searchUrl(const QString &query) const;
    // Whether the url is a page of results from one of the search engines on offer:
    // a search is something done, not a site visited (src/startpage/StartPage.h).
    bool isSearchUrl(const QString &url) const;
    // Typed address-bar text: a URL as-is, a host with a scheme added, or a search.
    Q_INVOKABLE QString urlForInput(const QString &input) const;
    // Whether typed text is an address rather than words: true exactly when
    // urlForInput() would not make a search of it. Empty text is neither.
    Q_INVOKABLE bool isAddress(const QString &input) const;
    // The other direction: the url as the bar shows it while it is not being edited.
    Q_INVOKABLE static QString displayAddress(const QString &url);

signals:
    void engineChanged();
    // The list of engines changed: one was added or removed. engineChanged() goes with
    // it, since where the chosen engine is in the list may have moved.
    void enginesChanged();
    void foundChanged();
    void omnibarTabsChanged();
    void omnibarBookmarksChanged();
    void omnibarHistoryChanged();
    void omnibarDownloadsChanged();

private:
    struct Engine
    {
        QString key;
        QString name;
        QString urlTemplate;
        // Where an added engine came from; empty for a built-in one.
        QString host;
    };

    struct Found
    {
        QString title;
        QString href;
        QString host;
    };

    // Where an engine's pages of results are, as isSearchUrl() asks.
    struct Results
    {
        QString host;
        // The path of a page of results, or the part of it before the words when the
        // words are in the path itself.
        QString path;
        // The query parameter that carries the words, empty when they are in the path.
        QString parameter;
        QString pathSuffix;
    };

    // Trimmed text as an address, or empty when it is words to search for.
    static QString addressFor(const QString &text);

    static bool readResults(const QString &urlTemplate, Results &results);
    void rebuildResults();
    QVector<Engine> engines() const;
    int indexOfEngine(const QString &key) const;
    bool hasEngineNamed(const QString &name) const;
    QString uniqueKey(const QString &name) const;
    void readStored();
    void storeAdded();
    void storeFound();
    // After the engines were added to or removed from: stored, results of each read
    // again, and said.
    void enginesWereChanged();
    void resetEngine();

    QVector<Engine> m_added;
    QVector<Found> m_found;
    QVector<Results> m_results;
};

} // namespace Salama
