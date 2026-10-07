// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "SettingsSection.h"

#include <QString>

namespace Salama {

class SearchEngines;

// Owns address-bar heuristics: typed text meaning depends on engine.
class SearchSettings : public SettingsSection
{
    Q_OBJECT
    Q_PROPERTY(QString engine READ engine WRITE setEngine NOTIFY engineChanged)
    Q_PROPERTY(int engineIndex READ engineIndex WRITE setEngineIndex NOTIFY engineChanged)
    Q_PROPERTY(bool omnibarTabs READ omnibarTabs WRITE setOmnibarTabs NOTIFY omnibarTabsChanged)
    Q_PROPERTY(bool omnibarBookmarks READ omnibarBookmarks WRITE setOmnibarBookmarks NOTIFY
                   omnibarBookmarksChanged)
    Q_PROPERTY(bool omnibarHistory READ omnibarHistory WRITE setOmnibarHistory NOTIFY
                   omnibarHistoryChanged)
    Q_PROPERTY(bool omnibarDownloads READ omnibarDownloads WRITE setOmnibarDownloads NOTIFY
                   omnibarDownloadsChanged)

public:
    // `engines` must outlive this.
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
    Q_INVOKABLE QString urlForInput(const QString &input) const;
    // True iff urlForInput() wouldn't search. Empty -> false.
    Q_INVOKABLE bool isAddress(const QString &input) const;
    Q_INVOKABLE static QString displayAddress(const QString &url);

signals:
    // Also when selected engine's index moves.
    void engineChanged();
    void omnibarTabsChanged();
    void omnibarBookmarksChanged();
    void omnibarHistoryChanged();
    void omnibarDownloadsChanged();

private:
    static QString addressFor(const QString &text);

    // No emit: engineChanged() follows list change.
    void chooseAdded(const QString &key);
    // Resets choice if its engine removed.
    void enginesWereChanged();

    SearchEngines &m_engines;
};

} // namespace Salama
