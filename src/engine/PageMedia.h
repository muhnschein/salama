// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include <QObject>
#include <QSet>
#include <QString>
#include <QTimer>
#include <QVariant>

namespace Salama {

class TabModel;

// What each page is playing, and the one control a tab has over it: silenced, which
// mutes and pauses it, and heard again, which unmutes it and plays what that paused.
// What plays is the tab in front's (docs/DECISIONS/0026-media-controls.md):
//
//  * A tab left for another is held: paused as it is left, while its page is still
//    the one on the screen, and played again when it comes back to the front. The
//    engine would silence it anyway -- Sailfish's Gecko suspends the media of a hidden
//    document, and a view behind the one in front is hidden -- but a page told it is
//    hidden may pause itself for good, as YouTube's does, and one paused from here
//    comes back as it was.
//  * While the tab in front plays, every other tab that plays is paused.
//  * While the application is out of sight, the pictures of what plays are hidden
//    from the page, so the engine stops decoding them and the page stops drawing
//    them: what goes on playing is the sound.
//
// The engine says that something started or stopped playing, but not where: its
// "media-decoder-info" notification, which PageActivity reads, names a decoder by its
// address and no page. So on each of those, and when another tab comes to the front,
// every loaded page is asked what it plays, with a script run in the page -- the same
// way the favicon and the theme colour are asked for (EngineMessages). The script
// reaches the page's <audio> and <video> elements and those of its frames from the
// same site; the answer, and the tab's muted flag, are kept in the tab model, where
// the grid's previews and the navigation bar read them.
//
// The browsing page runs what this asks for: requested() names a tab, or none for
// every loaded page, and a command; the page runs script() in that tab's view and
// hands the answer back through answer().
class PageMedia : public QObject
{
    Q_OBJECT
    // How long after the engine's word the pages are asked. Several decoders report
    // at once when a page starts playing, and they are asked once for all of them.
    Q_PROPERTY(int queryDelay READ queryDelay CONSTANT)

public:
    enum Command
    {
        // What does the page play? Also what applies the tab's muted flag to it.
        Query,
        Pause,
        Play
    };
    Q_ENUM(Command)

    static constexpr int DefaultQueryDelay = 200;

    explicit PageMedia(TabModel *tabs, QObject *parent = nullptr);
    PageMedia(TabModel *tabs, int queryDelay, QObject *parent = nullptr);

    int queryDelay() const;

    // Script for WebView.runJavaScript(): the body of a function, as EngineMessages'
    // are. It carries the command and whether the tab is muted, and answers what the
    // page plays after carrying them out.
    Q_INVOKABLE QString script(int tabId, int command) const;

    // What the script answered for this tab, to this command. What a page answers to
    // being paused is taken as it comes and asks nothing more: a page that will not
    // pause would otherwise be asked again, and again.
    Q_INVOKABLE void answer(int tabId, int command, const QVariant &answer);
    // The tab's page is going, and what it played with it: a new one is loading.
    Q_INVOKABLE void forget(int tabId);

    // The control, on the grid's previews, on the navigation bar and on the cover. A
    // tab that is heard is muted, and paused as well: a page silenced and left to
    // play on would still be going when it was wanted back. One that is not -- muted,
    // paused, held behind the front -- is unmuted and played, and one behind the
    // front is brought to it to be played (TabModel::shownMediaState()).
    Q_INVOKABLE void toggleMuted(int tabId);
    // Whether the tab plays and is heard, which is what the control shows.
    Q_INVOKABLE bool isHeard(int tabId) const;

    // The application is out of sight, or back: every loaded page is asked again at
    // once, which hides or shows what plays.
    void setBackground(bool background);

    // Something started or stopped playing somewhere: every loaded page is asked, a
    // moment from now, once however often this is called meanwhile.
    void refresh();

signals:
    // Run script(tab, command) in this tab's view, or in every loaded view for tab 0.
    void requested(int tab, int command);

private:
    TabModel *m_tabs;
    QTimer m_queryTimer;
    // The tab in front when last looked: the tab model says the front changed when
    // only its place in the grid did, as a carried cell trades places.
    int m_frontTabId = 0;
    // Tabs paused as they were left, to be played when they are back in front.
    QSet<int> m_held;
    bool m_background = false;
};

} // namespace Salama
