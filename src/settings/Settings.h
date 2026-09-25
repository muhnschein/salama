// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include <QObject>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QVariantList>

namespace Salama {

// User preferences, stored in the Sailjail-approved config location. Also owns the
// address-bar heuristics because "what does typed text mean" depends on the search engine.
class Settings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString homePage READ homePage WRITE setHomePage NOTIFY homePageChanged)
    Q_PROPERTY(
        QString searchEngine READ searchEngine WRITE setSearchEngine NOTIFY searchEngineChanged)
    Q_PROPERTY(int searchEngineIndex READ searchEngineIndex WRITE setSearchEngineIndex NOTIFY
                   searchEngineChanged)
    Q_PROPERTY(QStringList searchEngineNames READ searchEngineNames CONSTANT)
    Q_PROPERTY(bool desktopMode READ desktopMode WRITE setDesktopMode NOTIFY desktopModeChanged)
    Q_PROPERTY(bool cutoutGuard READ cutoutGuard WRITE setCutoutGuard NOTIFY cutoutGuardChanged)
    Q_PROPERTY(int coverStyle READ coverStyle WRITE setCoverStyle NOTIFY coverStyleChanged)
    // How many tabs keep their page loaded; 0 for all of them
    // (docs/DECISIONS/0016-five-live-pages.md). The index is over liveTabLimitChoices,
    // for a combo box.
    Q_PROPERTY(int liveTabLimit READ liveTabLimit NOTIFY liveTabLimitChanged)
    Q_PROPERTY(int liveTabLimitIndex READ liveTabLimitIndex WRITE setLiveTabLimitIndex NOTIFY
                   liveTabLimitChanged)
    Q_PROPERTY(QVariantList liveTabLimitChoices READ liveTabLimitChoices CONSTANT)
    // How much of the engine's own anti-tracking is switched on; a TrackingProtection
    // value (docs/DECISIONS/0023-tracking-protection.md).
    Q_PROPERTY(int trackingProtection READ trackingProtection WRITE setTrackingProtection NOTIFY
                   trackingProtectionChanged)
    // How the reader view sets an article (docs/DECISIONS/0024-reader-view.md).
    Q_PROPERTY(int readerColors READ readerColors WRITE setReaderColors NOTIFY readerColorsChanged)
    Q_PROPERTY(
        int readerTypeface READ readerTypeface WRITE setReaderTypeface NOTIFY readerTypefaceChanged)
    Q_PROPERTY(
        int readerTextSize READ readerTextSize WRITE setReaderTextSize NOTIFY readerTextSizeChanged)
    // Which sources the address bar's suggestions are drawn from, each on unless it is
    // switched off (docs/DECISIONS/0027-omnibar.md).
    Q_PROPERTY(bool omnibarTabs READ omnibarTabs WRITE setOmnibarTabs NOTIFY omnibarTabsChanged)
    Q_PROPERTY(bool omnibarBookmarks READ omnibarBookmarks WRITE setOmnibarBookmarks NOTIFY
                   omnibarBookmarksChanged)
    Q_PROPERTY(bool omnibarHistory READ omnibarHistory WRITE setOmnibarHistory NOTIFY
                   omnibarHistoryChanged)
    Q_PROPERTY(bool omnibarDownloads READ omnibarDownloads WRITE setOmnibarDownloads NOTIFY
                   omnibarDownloadsChanged)
    // Whether the pages visited are kept in the history, on unless switched off, and
    // whether the history is cleared as the browser closes, off unless switched on:
    // Firefox's Remember browsing and download history and Clear history when Firefox
    // closes (docs/DECISIONS/0030-history-settings.md).
    Q_PROPERTY(bool rememberHistory READ rememberHistory WRITE setRememberHistory NOTIFY
                   rememberHistoryChanged)
    Q_PROPERTY(bool clearHistoryOnClose READ clearHistoryOnClose WRITE setClearHistoryOnClose NOTIFY
                   clearHistoryOnCloseChanged)
    // The cover's one quick action, a QuickAction value, and for QuickActionBookmark the
    // bookmark it opens and the picture it wears (docs/DECISIONS/0029-quick-action.md).
    // The bookmark is kept as its id, and its address and title beside it: the id for
    // as long as the bookmark lives, the address to find it again when it is removed and
    // added back under a new id, the title to name it by once it is gone for good.
    Q_PROPERTY(int quickAction READ quickAction WRITE setQuickAction NOTIFY quickActionChanged)
    Q_PROPERTY(int quickActionBookmark READ quickActionBookmark NOTIFY quickActionBookmarkChanged)
    Q_PROPERTY(QString quickActionBookmarkUrl READ quickActionBookmarkUrl NOTIFY
                   quickActionBookmarkChanged)
    Q_PROPERTY(QString quickActionBookmarkTitle READ quickActionBookmarkTitle NOTIFY
                   quickActionBookmarkChanged)
    Q_PROPERTY(QString quickActionIcon READ quickActionIcon WRITE setQuickActionIcon NOTIFY
                   quickActionIconChanged)
    Q_PROPERTY(QStringList quickActionIcons READ quickActionIcons CONSTANT)

public:
    // How much of itself the cover shows; see docs/DECISIONS/0014-cover-is-the-tab-count.md.
    // The values are stored, so their numbers are part of the file format.
    //
    // Unscoped on purpose, and not the oversight SonarQube reads it as (cpp:S3642):
    // the cover reaches these as `Settings.CoverIconOnly`, and QML could not do that
    // with a scoped enum until Qt 5.8. This application is built against 5.6 (SCOPE.md
    // §4), so `enum class` here would compile on the host and leave the cover blank on
    // the phone. TabModel::Role is unscoped for the same reason.
    enum CoverStyle
    {
        CoverIconOnly = 0,
        CoverLatestTab = 1,
        CoverEveryTab = 2
    };
    Q_ENUM(CoverStyle)

    // Firefox's Enhanced Tracking Protection categories, less protection first, with
    // Off in place of Custom. Stored, so the numbers are part of the file format, and
    // unscoped for the reason CoverStyle is. What each asks of the engine is
    // EngineMessages::trackingProtectionPreferences().
    enum TrackingProtection
    {
        TrackingProtectionOff = 0,
        TrackingProtectionStandard = 1,
        TrackingProtectionStrict = 2
    };
    Q_ENUM(TrackingProtection)

    // The reader view's colours: the ambience's own, light or dark as it is, or one of
    // Firefox's reader themes whatever the ambience. Stored, like CoverStyle, and
    // unscoped for the same reason.
    enum ReaderColors
    {
        ReaderAmbience = 0,
        ReaderLight = 1,
        ReaderSepia = 2,
        ReaderDark = 3
    };
    Q_ENUM(ReaderColors)

    // The reader view's typeface, as Firefox offers it. Stored, and unscoped as
    // CoverStyle is.
    enum ReaderTypeface
    {
        ReaderSansSerif = 0,
        ReaderSerif = 1
    };
    Q_ENUM(ReaderTypeface)

    // The reader view's text size, in Firefox's reader.font_size steps: 1 to 9, 5 the
    // default. An enum so the slider in Settings can read its ends from here, and
    // unscoped as CoverStyle is, for the same reason.
    enum ReaderTextSize
    {
        ReaderTextSizeMin = 1,
        ReaderTextSizeDefault = 5,
        ReaderTextSizeMax = 9
    };
    Q_ENUM(ReaderTextSize)

    // What the cover's quick action does: nothing, open the address bar for a new tab,
    // show the bookmarks, open one bookmark, show the downloads, or show the history.
    // Stored, and unscoped as CoverStyle is, for the same reason.
    enum QuickAction
    {
        QuickActionNone = 0,
        QuickActionSearch = 1,
        QuickActionBookmarks = 2,
        QuickActionBookmark = 3,
        QuickActionDownloads = 4,
        QuickActionHistory = 5
    };
    Q_ENUM(QuickAction)

    explicit Settings(const QString &filePath, QObject *parent = nullptr);

    QString homePage() const;
    void setHomePage(const QString &url);

    QString searchEngine() const;
    void setSearchEngine(const QString &key);
    int searchEngineIndex() const;
    void setSearchEngineIndex(int index);
    QStringList searchEngineNames() const;
    QStringList searchEngineKeys() const;

    bool desktopMode() const;
    void setDesktopMode(bool desktopMode);

    // Whether this application keeps out of the display's own cutout. On by default:
    // a camera notch over the first line of a page is not a design decision.
    bool cutoutGuard() const;
    void setCutoutGuard(bool cutoutGuard);

    // Out-of-range values read back as the default rather than as a cover that draws
    // nothing: this comes from a file a user can edit.
    int coverStyle() const;
    void setCoverStyle(int style);

    int liveTabLimit() const;
    int liveTabLimitIndex() const;
    void setLiveTabLimitIndex(int index);
    QVariantList liveTabLimitChoices() const;
    static int defaultLiveTabLimit();

    // Standard unless changed, as in Firefox. Out of range reads back as the default,
    // like coverStyle.
    int trackingProtection() const;
    void setTrackingProtection(int level);

    // Out-of-range values read back as the defaults, as coverStyle's do.
    int readerColors() const;
    void setReaderColors(int colors);
    int readerTypeface() const;
    void setReaderTypeface(int typeface);
    int readerTextSize() const;
    void setReaderTextSize(int size);

    bool omnibarTabs() const;
    void setOmnibarTabs(bool on);
    bool omnibarBookmarks() const;
    void setOmnibarBookmarks(bool on);
    bool omnibarHistory() const;
    void setOmnibarHistory(bool on);
    bool omnibarDownloads() const;
    void setOmnibarDownloads(bool on);

    bool rememberHistory() const;
    void setRememberHistory(bool on);
    bool clearHistoryOnClose() const;
    void setClearHistoryOnClose(bool on);

    // Search unless changed: what the cover offered before there was a choice. Out of
    // range reads back as the default, like coverStyle.
    int quickAction() const;
    void setQuickAction(int action);
    // 0, with no address and no title, when no bookmark has been picked.
    int quickActionBookmark() const;
    QString quickActionBookmarkUrl() const;
    QString quickActionBookmarkTitle() const;
    // The three together, as a bookmark is picked or found again under a new id: one
    // write, one signal. An id of 0 forgets the bookmark; a negative one is refused.
    // The action itself is left as it is.
    Q_INVOKABLE void setQuickActionBookmark(int id, const QString &url, const QString &title);
    // One of quickActionIcons(), the first unless changed; a name not on the list is
    // refused, and reads back as the first.
    QString quickActionIcon() const;
    void setQuickActionIcon(const QString &name);
    QStringList quickActionIcons() const;
    // The file a cover action's picture is drawn from, relative to the application's
    // root: "art/cover/<name>-<size>-<white|black>.png", at the rendered size nearest the
    // icon size asked for, white for a dark ambience and black for a light one
    // (icons/render.sh draws them).
    Q_INVOKABLE static QString coverIconPath(const QString &name, qreal iconSize, bool onDark);
    // How many screen pixels the engine lays a css pixel out on, for a screen of this
    // Theme.pixelRatio: 1.75 of it in steps of a half, which is about 360 css pixels
    // across a 1080 wide screen -- the width a phone layout is written for -- where the
    // platform's own 1.5 gives 410. The browsing page hands it the engine; the reader
    // settings' preview sets its text by it, as large as the reader view will.
    Q_INVOKABLE static qreal pageZoom(qreal pixelRatio);

    Q_INVOKABLE QString searchUrl(const QString &query) const;
    // Typed address-bar text: a URL as-is, a host with a scheme added, or a search.
    Q_INVOKABLE QString urlForInput(const QString &input) const;
    // Whether typed text is an address rather than words: true exactly when
    // urlForInput() would not make a search of it. Empty text is neither.
    Q_INVOKABLE bool isAddress(const QString &input) const;
    // The other direction: the url as the bar shows it while it is not being edited.
    Q_INVOKABLE static QString displayAddress(const QString &url);

    static QString defaultHomePage();
    static QString defaultSearchEngine();

signals:
    void homePageChanged();
    void searchEngineChanged();
    void desktopModeChanged();
    void cutoutGuardChanged();
    void coverStyleChanged();
    void liveTabLimitChanged();
    void trackingProtectionChanged();
    void readerColorsChanged();
    void readerTypefaceChanged();
    void readerTextSizeChanged();
    void omnibarTabsChanged();
    void omnibarBookmarksChanged();
    void omnibarHistoryChanged();
    void omnibarDownloadsChanged();
    void rememberHistoryChanged();
    void clearHistoryOnCloseChanged();
    void quickActionChanged();
    void quickActionBookmarkChanged();
    void quickActionIconChanged();

private:
    // Trimmed text as an address, or empty when it is words to search for.
    static QString addressFor(const QString &text);
    // A switch as stored, what it is before it is first switched given.
    bool flag(const char *key, bool initially = true) const;
    bool setFlag(const char *key, bool on, bool initially = true);

    QSettings m_settings;
};

} // namespace Salama
