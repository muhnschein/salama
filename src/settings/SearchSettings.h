// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "SettingsSection.h"

#include <QString>
#include <QStringList>

namespace Salama {

// Settings > Search: the search engine, and the sources the address bar's suggestions
// are drawn from. Also owns the address-bar heuristics, because what typed text means
// depends on the search engine.
class SearchSettings : public SettingsSection
{
    Q_OBJECT
    Q_PROPERTY(QString engine READ engine WRITE setEngine NOTIFY engineChanged)
    Q_PROPERTY(int engineIndex READ engineIndex WRITE setEngineIndex NOTIFY engineChanged)
    Q_PROPERTY(QStringList engineNames READ engineNames CONSTANT)
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
    // Whether the url is a page of results from one of the search engines on offer:
    // a search is something done, not a site visited (src/startpage/StartPage.h).
    static bool isSearchUrl(const QString &url);
    // Typed address-bar text: a URL as-is, a host with a scheme added, or a search.
    Q_INVOKABLE QString urlForInput(const QString &input) const;
    // Whether typed text is an address rather than words: true exactly when
    // urlForInput() would not make a search of it. Empty text is neither.
    Q_INVOKABLE bool isAddress(const QString &input) const;
    // The other direction: the url as the bar shows it while it is not being edited.
    Q_INVOKABLE static QString displayAddress(const QString &url);

signals:
    void engineChanged();
    void omnibarTabsChanged();
    void omnibarBookmarksChanged();
    void omnibarHistoryChanged();
    void omnibarDownloadsChanged();

private:
    // Trimmed text as an address, or empty when it is words to search for.
    static QString addressFor(const QString &text);
};

} // namespace Salama
