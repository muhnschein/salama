// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariant>

namespace Salama {

class TabModel;

// What each page is playing, and the controls a tab has over it: play and pause, mute
// and unmute. Only one tab plays at a time: while the tab in front plays, every other
// tab that plays is paused (docs/DECISIONS/0024-media-controls.md).
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

    // The two controls, on the grid's previews and on the navigation bar. Playing a
    // tab that is not in front brings it to the front: a page behind the one in front
    // is hidden, and the engine does not play a hidden page's media
    // (TabModel::shownMediaState()).
    Q_INVOKABLE void togglePlayback(int tabId);
    Q_INVOKABLE void toggleMuted(int tabId);

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
};

} // namespace Salama
