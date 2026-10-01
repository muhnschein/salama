// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "settings/SettingsSection.h"

#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVector>

namespace Salama {

// The search engines the address bar can search with: the three built in and the ones
// added from what sites offered as the pages were browsed. Each page says, through the
// engine, that it has a search (offerEngine()), the offers are kept until one is taken up
// (addFoundEngine()) or forgotten, and what is taken up joins the built-in ones
// (docs/DECISIONS/0041-search-engines-found.md). Which of them is in use is
// SearchSettings' to say; this is what there is to choose from, and which pages are
// pages of their results.
class SearchEngines : public SettingsSection
{
    Q_OBJECT
    // The built-in engines first, then the added ones in the order they were added; a
    // list that changes as one is added or removed. engineHosts is the site each added
    // engine came from, empty for a built-in one.
    Q_PROPERTY(QStringList engineNames READ engineNames NOTIFY enginesChanged)
    Q_PROPERTY(QStringList engineKeys READ engineKeys NOTIFY enginesChanged)
    Q_PROPERTY(QStringList engineHosts READ engineHosts NOTIFY enginesChanged)
    Q_PROPERTY(int addedCount READ addedCount NOTIFY enginesChanged)
    // What sites have offered and nothing has taken up: a list of {title, href, host}.
    Q_PROPERTY(QVariantList foundEngines READ foundEngines NOTIFY foundChanged)

public:
    explicit SearchEngines(QSettings &file, QObject *parent = nullptr);

    QStringList engineNames() const;
    QStringList engineKeys() const;
    QStringList engineHosts() const;
    int addedCount() const;
    QVariantList foundEngines() const;

    // The key of the engine in use until another is chosen: the first built-in one.
    static QString defaultKey();
    // A host without its "www.", which is how sites are told apart here.
    static QString withoutWww(const QString &host);

    // Where an engine is in the list, or -1 for a key that is none; and what the engine
    // at an index in it is called by, and searches with. The index is one of count().
    int indexOf(const QString &key) const;
    int count() const;
    QString keyAt(int index) const;
    QString templateAt(int index) const;

    // A site offers a search: the title it gives it, the address of its OpenSearch
    // description and the host of the page that offered it. Kept to be taken up unless it
    // is one already: an engine of that title is on offer, or the offer is already
    // kept. Answers whether it was kept.
    Q_INVOKABLE bool offerEngine(const QString &title, const QString &href, const QString &host);
    // Takes up a kept offer with the description fetched from its address: the engine joins
    // the others, and is no longer an offer; engineAdded() says which it is, for the
    // search settings to choose it. Answers false, and keeps the offer, when the text is no
    // description of a search the browser can use or its name is that of an engine
    // already on offer.
    Q_INVOKABLE bool addFoundEngine(const QString &href, const QString &description);
    Q_INVOKABLE void forgetFoundEngine(const QString &href);
    // An added engine removed; a built-in one is not removable.
    Q_INVOKABLE void removeAddedEngine(const QString &key);
    // Every added engine removed, and every offer forgotten.
    Q_INVOKABLE void removeAddedEngines();

    // Whether the url is a page of results from one of the search engines on offer:
    // a search is something done, not a site visited (src/startpage/StartPage.h).
    bool isSearchUrl(const QString &url) const;

signals:
    // The list of engines changed: one was added or removed.
    void enginesChanged();
    void foundChanged();
    // An offer was taken up, as the engine with this key. Said before enginesChanged(),
    // so that whoever chooses it has done so by the time the list is said to have moved.
    void engineAdded(const QString &key);

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

    static bool readResults(const QString &urlTemplate, Results &results);
    void rebuildResults();
    QVector<Engine> engines() const;
    bool hasEngineNamed(const QString &name) const;
    QString uniqueKey(const QString &name) const;
    void readStored();
    void storeAdded();
    void storeFound();
    // After the engines were added to or removed from: results of each read again, and
    // said.
    void enginesWereChanged();

    QVector<Engine> m_added;
    QVector<Found> m_found;
    QVector<Results> m_results;
};

} // namespace Salama
