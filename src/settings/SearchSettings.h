// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "SettingsSection.h"

#include <QString>

namespace Salama {

class SearchEngines;

// Settings > Search: the search engine in use, and the sources the address bar's
// suggestions are drawn from. Also owns the address-bar heuristics, because what typed
// text means depends on the search engine.
//
// Which engines there are to choose from -- the built-in ones and those added from what
// sites offered -- is SearchEngines', which this borrows
// (docs/DECISIONS/0041-search-engines-found.md): an engine it adds becomes the one in
// use, and the first built-in one takes the place of an engine it removes from under the
// choice.
class SearchSettings : public SettingsSection
{
    Q_OBJECT
    Q_PROPERTY(QString engine READ engine WRITE setEngine NOTIFY engineChanged)
    Q_PROPERTY(int engineIndex READ engineIndex WRITE setEngineIndex NOTIFY engineChanged)
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
    // The engines are borrowed: whoever owns the sections makes them first, and they
    // outlive this.
    SearchSettings(QSettings &file, SearchEngines &engines, QObject *parent = nullptr);

    QString engine() const;
    void setEngine(const QString &key);
    int engineIndex() const;
    void setEngineIndex(int index);
    static QString defaultEngine();

    bool omnibarTabs() const;
    void setOmnibarTabs(bool on);
    bool omnibarBookmarks() const;
    void setOmnibarBookmarks(bool on);
    bool omnibarHistory() const;
    void setOmnibarHistory(bool on);
    bool omnibarDownloads() const;
    void setOmnibarDownloads(bool on);

    Q_INVOKABLE QString searchUrl(const QString &query) const;
    // Typed address-bar text: a URL as-is, a host with a scheme added, or a search.
    Q_INVOKABLE QString urlForInput(const QString &input) const;
    // Whether typed text is an address rather than words: true exactly when
    // urlForInput() would not make a search of it. Empty text is neither.
    Q_INVOKABLE bool isAddress(const QString &input) const;
    // The other direction: the url as the bar shows it while it is not being edited.
    Q_INVOKABLE static QString displayAddress(const QString &url);

signals:
    // The engine in use changed, or where it is in the list of engines did.
    void engineChanged();
    void omnibarTabsChanged();
    void omnibarBookmarksChanged();
    void omnibarHistoryChanged();
    void omnibarDownloadsChanged();

private:
    // Trimmed text as an address, or empty when it is words to search for.
    static QString addressFor(const QString &text);

    // An offer was taken up as the engine with this key: the engine in use from now on.
    // Not said here; engineChanged() goes with the change of the list that follows.
    void chooseAdded(const QString &key);
    // After the engines were added to or removed from: written over the choice if it was
    // the engine that went, so that the file never names an engine that is not in it,
    // and said, since where the engine in use is in the list may have moved.
    void enginesWereChanged();

    SearchEngines &m_engines;
};

} // namespace Salama
