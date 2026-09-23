// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include <QHash>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariant>

namespace Salama {

// Whether the pages may keep running while the application is out of sight
// (docs/DECISIONS/0020-pages-sleep-out-of-sight.md).
//
// A page's scripts, timers and workers run on in the background -- the engine only
// slows them -- and a busy page keeps the processor busy for as long as it is open. So
// once the application has been out of sight for a moment the pages are put to sleep,
// unless one of them is making a sound: a player's next track, or a call, needs its
// page's scripts. What the engine says is playing arrives on two observer topics,
// which are the ones sailfish-browser reads for the same decision
// (apps/shared/ResourceController.qml):
//
//  * "media-decoder-info", from Gecko's media decoder as patched for Sailfish OS
//    (sailfishos/gecko-dev,
//    rpm/0056-Ensure-audio-continues-when-screen-is-locked.-Contri.patch): a
//    decoder's metadata, `{"owner", "state": "meta", "a", "v"}` -- does the stream
//    have sound, does it have pictures -- and every change of its play state,
//    `{"owner", "state": "play" | "pause"}`. The owner is the decoder's address.
//  * "webrtc-media-info", from embedlite-components jscomps/EmbedLiteWebrtcUI.js:
//    `{"audio", "video"}`, whether any page is capturing from the microphone or camera.
//
// What the pages do about it is the browsing page's: this only says when.
class PageActivity : public QObject
{
    Q_OBJECT
    // The topics to subscribe to, for WebEngine.addObserver().
    Q_PROPERTY(QStringList topics READ topics CONSTANT)
    // The application is out of sight. Set by the browsing page, which hears of it.
    Q_PROPERTY(bool background READ background WRITE setBackground NOTIFY backgroundChanged)
    // A page is playing something with sound, or is in a call.
    Q_PROPERTY(bool audible READ audible NOTIFY audibleChanged)
    // The pages should be asleep now. Turns true after settleDelay out of sight, or
    // soundGraceDelay after the last sound stopped out of sight; false the moment the
    // application is back.
    Q_PROPERTY(bool asleep READ asleep NOTIFY asleepChanged)
    Q_PROPERTY(int settleDelay READ settleDelay CONSTANT)
    Q_PROPERTY(int soundGraceDelay READ soundGraceDelay CONSTANT)

public:
    // A second: the application goes out of sight for a moment when a system dialog
    // or a peek at the home screen comes over it, and a page woken on every one of
    // those would sleep for nothing. Five seconds after sound stops, because a player
    // starts its next track from the page's own scripts, and sailfish-browser found a
    // second too little to buffer one over a mobile connection.
    static constexpr int DefaultSettleDelay = 1000;
    static constexpr int DefaultSoundGraceDelay = 5000;

    explicit PageActivity(QObject *parent = nullptr);
    PageActivity(int settleDelay, int soundGraceDelay, QObject *parent = nullptr);

    QStringList topics() const;

    bool background() const;
    void setBackground(bool background);

    bool audible() const;
    bool asleep() const;
    int settleDelay() const;
    int soundGraceDelay() const;

    // What WebEngine.recvObserve() delivered. Anything else is ignored.
    Q_INVOKABLE void observe(const QString &topic, const QVariant &data);

signals:
    void backgroundChanged();
    void audibleChanged();
    void asleepChanged();

private:
    void observeDecoder(const QVariantMap &info);
    void forgetIdleDecoders();
    void updateAudible();
    void setAsleep(bool asleep);
    void fallAsleepAfter(int delay);

    int m_settleDelay;
    int m_soundGraceDelay;
    QTimer m_timer;
    bool m_background = false;
    bool m_audible = false;
    bool m_asleep = false;
    bool m_inCall = false;
    // Every decoder whose metadata has been seen, and whether its stream has sound.
    QHash<QString, bool> m_hasSound;
    // The decoders playing now.
    QSet<QString> m_playing;
};

} // namespace Salama
