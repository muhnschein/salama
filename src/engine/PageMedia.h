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

// Per-tab play/mute; front tab owns playback. Leaving tab paused while still on screen: page
// told it's hidden may pause for good (YouTube). Decoder events name no page, so every loaded page
// is asked by script: requested() -> script() -> answer().
class PageMedia : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int queryDelay READ queryDelay CONSTANT)

public:
    enum class Command
    {
        // Also applies muted flag.
        Query,
        Pause,
        Play
    };

    static constexpr int DefaultQueryDelay = 200;

    explicit PageMedia(TabModel *tabs, QObject *parent = nullptr);
    PageMedia(TabModel *tabs, int queryDelay, QObject *parent = nullptr);

    int queryDelay() const;

    QString script(int tabId, Command command) const;
    Q_INVOKABLE QString script(int tabId, int command) const;

    // Pause answer not re-asked: stubborn page would loop.
    void answer(int tabId, Command command, const QVariant &answer);
    Q_INVOKABLE void answer(int tabId, int command, const QVariant &answer);
    Q_INVOKABLE void forget(int tabId);

    // Heard -> mute + pause. Else unmute + play, fronting tab if behind.
    Q_INVOKABLE void toggleMuted(int tabId);
    Q_INVOKABLE bool isHeard(int tabId) const;

    void setBackground(bool background);

    void refresh();

signals:
    // Tab 0 = every loaded view.
    void requested(int tab, int command);

private:
    void request(int tabId, Command command);

    TabModel *m_tabs;
    QTimer m_queryTimer;
    // Model signals front change on mere grid move.
    int m_frontTabId = 0;
    QSet<int> m_held;
    bool m_background = false;
};

} // namespace Salama
