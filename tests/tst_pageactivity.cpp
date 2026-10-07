// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "engine/PageActivity.h"

#include <QSignalSpy>
#include <QtTest>

using Salama::PageActivity;

namespace {

// Short to wait out, far enough apart that slow machine can't confuse them: "nothing yet"
// waits are few settles long, well inside grace.
const int Settle = 20;
const int Grace = 1000;

QVariantMap decoder(const QString &owner, const QString &state)
{
    return {{QStringLiteral("owner"), owner}, {QStringLiteral("state"), state}};
}

// Engine message once decoder knows stream. Flags numeric after JSON parse.
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

    QTest::qWait(Settle * 3);
    QVERIFY(!activity.asleep());

    // Background: not at once (home-screen peek isn't leaving), after settle delay.
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

    // Music stops: sleep only after longer grace, for player to start next track.
    activity.observe(DecoderTopic, decoder(QStringLiteral("0x1"), QStringLiteral("pause")));
    QVERIFY(!activity.audible());
    QTest::qWait(Settle * 5);
    QVERIFY(!activity.asleep());
    QTRY_VERIFY(activity.asleep());

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
    // Muted hero video must not keep busy page running; no-audio stream is only kind engine
    // can call silent.
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
    // Playing before metadata: assume sound until engine says otherwise.
    PageActivity activity(Settle, Grace);
    activity.observe(DecoderTopic, decoder(QStringLiteral("0x5"), QStringLiteral("play")));
    QVERIFY(activity.audible());
    activity.observe(DecoderTopic, meta(QStringLiteral("0x5"), false, true));
    QVERIFY(!activity.audible());
}

void tst_pageactivity::forgottenDecodersAreBounded()
{
    PageActivity activity(Settle, Grace);
    activity.observe(DecoderTopic, meta(QStringLiteral("playing"), false, true));
    activity.observe(DecoderTopic, decoder(QStringLiteral("playing"), QStringLiteral("play")));
    activity.observe(DecoderTopic, meta(QStringLiteral("paused"), false, true));
    for (int i = 0; i < 300; ++i) {
        activity.observe(DecoderTopic, meta(QStringLiteral("clip-%1").arg(i), false, true));
    }
    QVERIFY(!activity.audible());
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

// Every play-state change notified (PageMedia then asks pages); stream contents and calls not.
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
