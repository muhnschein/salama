// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "engine/PageActivity.h"

#include <QSignalSpy>
#include <QtTest>

using Salama::PageActivity;

namespace {

// Short enough to wait out, and far enough apart that a slow machine cannot mistake
// one for the other: every wait that expects nothing to have happened yet is a few
// settle delays long, and well inside the grace.
const int Settle = 20;
const int Grace = 1000;

QVariantMap decoder(const QString &owner, const QString &state)
{
    return {{QStringLiteral("owner"), owner}, {QStringLiteral("state"), state}};
}

// What the engine sends once a decoder knows its stream. It writes the flags as
// numbers, which is what they are after the JSON is read.
QVariantMap meta(const QString &owner, bool sound, bool pictures)
{
    QVariantMap info = decoder(owner, QStringLiteral("meta"));
    info.insert(QStringLiteral("a"), sound ? 1.0 : 0.0);
    info.insert(QStringLiteral("v"), pictures ? 1.0 : 0.0);
    return info;
}

QVariantMap call(bool audio, bool video)
{
    return {{QStringLiteral("audio"), audio}, {QStringLiteral("video"), video}};
}

const QString DecoderTopic = QStringLiteral("media-decoder-info");
const QString CallTopic = QStringLiteral("webrtc-media-info");

} // namespace

class tst_pageactivity : public QObject
{
    Q_OBJECT

private slots:
    void defaults();
    void asleepAfterSettling();
    void backInSightWakesAtOnce();
    void soundKeepsPagesAwake();
    void silentVideoDoesNot();
    void soundStartingCancelsSleep();
    void callsKeepPagesAwake();
    void unknownDecodersCountAsSound();
    void forgottenDecodersAreBounded();
    void otherMessagesIgnored();
    void playStateChangesAreTold();
};

void tst_pageactivity::defaults()
{
    PageActivity activity;
    // The topics are the two sailfish-browser subscribes to for the same decision.
    QCOMPARE(activity.topics(), QStringList({DecoderTopic, CallTopic}));
    QCOMPARE(activity.settleDelay(), 1000);
    QCOMPARE(activity.soundGraceDelay(), 5000);
    QVERIFY(!activity.background());
    QVERIFY(!activity.audible());
    QVERIFY(!activity.asleep());
}

void tst_pageactivity::asleepAfterSettling()
{
    PageActivity activity(Settle, Grace);
    QSignalSpy background(&activity, &PageActivity::backgroundChanged);
    QSignalSpy asleep(&activity, &PageActivity::asleepChanged);

    // In sight, the pages never sleep.
    QTest::qWait(Settle * 3);
    QVERIFY(!activity.asleep());

    // Out of sight, not at once -- a peek at the home screen is not leaving -- but
    // after the settle delay.
    activity.setBackground(true);
    activity.setBackground(true);
    QCOMPARE(background.count(), 1);
    QVERIFY(!activity.asleep());
    QTRY_VERIFY(activity.asleep());
    QCOMPARE(asleep.count(), 1);
}

void tst_pageactivity::backInSightWakesAtOnce()
{
    PageActivity activity(Settle, Grace);
    activity.setBackground(true);
    QTRY_VERIFY(activity.asleep());
    activity.setBackground(false);
    QVERIFY(!activity.asleep());

    // A return before the delay ran out cancels it.
    activity.setBackground(true);
    activity.setBackground(false);
    QTest::qWait(Settle * 3);
    QVERIFY(!activity.asleep());
}

void tst_pageactivity::soundKeepsPagesAwake()
{
    PageActivity activity(Settle, Grace);
    QSignalSpy audible(&activity, &PageActivity::audibleChanged);
    activity.observe(DecoderTopic, meta(QStringLiteral("0x1"), true, false));
    QVERIFY(!activity.audible());
    activity.observe(DecoderTopic, decoder(QStringLiteral("0x1"), QStringLiteral("play")));
    QVERIFY(activity.audible());
    QCOMPARE(audible.count(), 1);

    activity.setBackground(true);
    QTest::qWait(Settle * 5);
    QVERIFY(!activity.asleep());

    // The music stops: the pages sleep, but only after the longer grace a player needs
    // to start its next track.
    activity.observe(DecoderTopic, decoder(QStringLiteral("0x1"), QStringLiteral("pause")));
    QVERIFY(!activity.audible());
    QTest::qWait(Settle * 5);
    QVERIFY(!activity.asleep());
    QTRY_VERIFY(activity.asleep());

    // The next track in time keeps them awake.
    activity.setBackground(false);
    activity.observe(DecoderTopic, decoder(QStringLiteral("0x1"), QStringLiteral("play")));
    activity.setBackground(true);
    activity.observe(DecoderTopic, decoder(QStringLiteral("0x1"), QStringLiteral("pause")));
    activity.observe(DecoderTopic, meta(QStringLiteral("0x2"), true, false));
    activity.observe(DecoderTopic, decoder(QStringLiteral("0x2"), QStringLiteral("play")));
    QTest::qWait(Grace + Settle * 5);
    QVERIFY(!activity.asleep());
}

void tst_pageactivity::silentVideoDoesNot()
{
    // A muted hero video on a busy page is exactly what should not keep it running,
    // and a stream with no sound in it is the one kind the engine can say is silent.
    PageActivity activity(Settle, Grace);
    activity.observe(DecoderTopic, meta(QStringLiteral("0x3"), false, true));
    activity.observe(DecoderTopic, decoder(QStringLiteral("0x3"), QStringLiteral("play")));
    QVERIFY(!activity.audible());
    activity.setBackground(true);
    QTRY_VERIFY(activity.asleep());
}

void tst_pageactivity::soundStartingCancelsSleep()
{
    PageActivity activity(Settle, Grace);
    activity.setBackground(true);
    activity.observe(DecoderTopic, meta(QStringLiteral("0x4"), true, true));
    activity.observe(DecoderTopic, decoder(QStringLiteral("0x4"), QStringLiteral("play")));
    QTest::qWait(Settle * 5);
    QVERIFY(!activity.asleep());
}

void tst_pageactivity::callsKeepPagesAwake()
{
    PageActivity activity(Settle, Grace);
    activity.observe(CallTopic, call(true, false));
    QVERIFY(activity.audible());
    activity.observe(CallTopic, call(false, true));
    QVERIFY(activity.audible());
    activity.setBackground(true);
    QTest::qWait(Settle * 5);
    QVERIFY(!activity.asleep());
    activity.observe(CallTopic, call(false, false));
    QVERIFY(!activity.audible());
    QTRY_VERIFY(activity.asleep());
}

void tst_pageactivity::unknownDecodersCountAsSound()
{
    // Playing before its metadata was seen: sound until the engine says otherwise.
    PageActivity activity(Settle, Grace);
    activity.observe(DecoderTopic, decoder(QStringLiteral("0x5"), QStringLiteral("play")));
    QVERIFY(activity.audible());
    activity.observe(DecoderTopic, meta(QStringLiteral("0x5"), false, true));
    QVERIFY(!activity.audible());
}

void tst_pageactivity::forgottenDecodersAreBounded()
{
    PageActivity activity(Settle, Grace);
    // A silent decoder that is playing is remembered through a clear-out ...
    activity.observe(DecoderTopic, meta(QStringLiteral("playing"), false, true));
    activity.observe(DecoderTopic, decoder(QStringLiteral("playing"), QStringLiteral("play")));
    activity.observe(DecoderTopic, meta(QStringLiteral("paused"), false, true));
    for (int i = 0; i < 300; ++i) {
        activity.observe(DecoderTopic, meta(QStringLiteral("clip-%1").arg(i), false, true));
    }
    QVERIFY(!activity.audible());
    // ... one that was not is forgotten, and playing again it counts as sound.
    activity.observe(DecoderTopic, decoder(QStringLiteral("paused"), QStringLiteral("play")));
    QVERIFY(activity.audible());
}

void tst_pageactivity::otherMessagesIgnored()
{
    PageActivity activity(Settle, Grace);
    QSignalSpy audible(&activity, &PageActivity::audibleChanged);
    activity.observe(QStringLiteral("embed:download"), call(true, true));
    activity.observe(DecoderTopic, decoder(QString(), QStringLiteral("play")));
    activity.observe(DecoderTopic, QVariant(QStringLiteral("not a map")));
    activity.observe(CallTopic, QVariant());
    QVERIFY(!activity.audible());
    QCOMPARE(audible.count(), 0);
}

// Every change of a decoder's play state is told on, for PageMedia to ask the pages
// what plays; what a decoder's stream holds, and a call, are not.
void tst_pageactivity::playStateChangesAreTold()
{
    PageActivity activity(Settle, Grace);
    QSignalSpy changed(&activity, &PageActivity::playStateChanged);
    activity.observe(DecoderTopic, meta(QStringLiteral("0x1"), true, true));
    activity.observe(CallTopic, call(true, false));
    QCOMPARE(changed.count(), 0);
    activity.observe(DecoderTopic, decoder(QStringLiteral("0x1"), QStringLiteral("play")));
    QCOMPARE(changed.count(), 1);
    activity.observe(DecoderTopic, decoder(QStringLiteral("0x1"), QStringLiteral("pause")));
    QCOMPARE(changed.count(), 2);
    activity.observe(DecoderTopic, decoder(QString(), QStringLiteral("play")));
    QCOMPARE(changed.count(), 2);
}

QTEST_GUILESS_MAIN(tst_pageactivity)
#include "tst_pageactivity.moc"
