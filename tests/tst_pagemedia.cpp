// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "engine/PageMedia.h"
#include "tabs/TabModel.h"

#include <QJSEngine>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QtTest>

using Salama::PageMedia;
using Salama::TabModel;

namespace {

const int Delay = 20;

QStringList requests(const QSignalSpy &spy)
{
    QStringList list;
    for (const QList<QVariant> &arguments : spy) {
        list.append(
            QStringLiteral("%1:%2").arg(arguments.at(0).toInt()).arg(arguments.at(1).toInt()));
    }
    return list;
}

QString request(int tabId, PageMedia::Command command)
{
    return QStringLiteral("%1:%2").arg(tabId).arg(static_cast<int>(command));
}

// Minimal DOM for script, run in JS engine. `media`: element playing/paused, muted or not,
// hasAudio -> <video>, else <audio> (Gecko has mozHasAudio on video only). `page`: document
// of elements and frames; cross-site frame has no document. `navigator`: empty Media Session.
const char *const FakeDom = R"(
function media(options) {
  var m = { paused: true, ended: false, muted: false, volume: 1, plays: 0,
            localName: 'hasAudio' in options ? 'video' : 'audio', style: { visibility: '' } };
  for (var key in options) { m[key] = options[key]; }
  if ('hasAudio' in options) { m.mozHasAudio = options.hasAudio; delete m.hasAudio; }
  m.pause = function () { this.paused = true; };
  m.play = function () { this.paused = false; this.plays++; return { catch: function () {} }; };
  return m;
}
function page(elements, frames) {
  return { querySelectorAll: function (selector) {
    return selector === 'audio, video' ? elements : (frames || []);
  } };
}
function run(script, doc) {
  document = doc;
  return new Function(script)();
}
var document = null;
var navigator = { mediaSession: { metadata: null } };
)";

} // namespace

class tst_pagemedia : public QObject
{
    Q_OBJECT

private slots:
    void scriptCarriesTheCommandAndTheMute();
    void scriptOverAPage();
    void scriptSaysWhatPlays();
    void answersBecomeTheTabsState();
    void theTabInFrontPausesTheOthers();
    void aTabBehindIsPausedWhileTheFrontPlays();
    void aTabBehindMayPlayWhileTheFrontIsSilent();
    void toggleMuted();
    void aTabLeftIsHeldUntilItIsBack();
    void outOfSightThePagesAreAskedAgain();
    void refreshAsksEveryPageOnce();
    void anotherTabInFrontAsksAgain();
};

void tst_pagemedia::scriptCarriesTheCommandAndTheMute()
{
    TabModel tabs(nullptr);
    PageMedia media(&tabs, Delay);
    const int id = tabs.newTab(QStringLiteral("https://a.example/"));
    QCOMPARE(media.queryDelay(), Delay);
    QCOMPARE(PageMedia(&tabs).queryDelay(), PageMedia::DefaultQueryDelay);

    // Function body, as every engine script: must return.
    const QString query = media.script(id, PageMedia::Command::Query);
    QVERIFY(query.contains(QLatin1String("return ")));
    QVERIFY(
        query.contains(QLatin1String("var command = 'query', muted = false, concealed = false;")));
    QVERIFY(media.script(id, PageMedia::Command::Pause).contains(QLatin1String("'pause'")));
    QVERIFY(media.script(id, PageMedia::Command::Play).contains(QLatin1String("'play'")));
    QVERIFY(query.contains(QLatin1String("contentDocument")));
    QVERIFY(query.contains(QLatin1String("'audio, video'")));

    tabs.setMuted(id, true);
    QVERIFY(media.script(id, PageMedia::Command::Query).contains(QLatin1String("muted = true,")));
    media.setBackground(true);
    QVERIFY(
        media.script(id, PageMedia::Command::Query).contains(QLatin1String("concealed = true;")));
}

void tst_pagemedia::scriptOverAPage()
{
    TabModel tabs(nullptr);
    PageMedia media(&tabs, Delay);
    const int id = tabs.newTab(QStringLiteral("https://a.example/"));
    QJSEngine engine;
    QVERIFY(!engine.evaluate(QString::fromUtf8(FakeDom)).isError());
    const auto run = [&](PageMedia::Command command, const QString &page) {
        engine.globalObject().setProperty(QStringLiteral("script"), media.script(id, command));
        const QJSValue result =
            engine.evaluate(QStringLiteral("JSON.parse(run(script, %1)).state").arg(page));
        return result.isError() ? QStringLiteral("error: ") + result.toString() : result.toString();
    };
    const auto js = [&](const QString &code) { return engine.evaluate(code); };

    QCOMPARE(run(PageMedia::Command::Query, QStringLiteral("page([])")), QString());

    js(QStringLiteral("var video = media({ paused: false, hasAudio: true });"
                      "var audio = media({ paused: false });"
                      "var silent = media({ paused: false, hasAudio: false });"
                      "var hushed = media({ paused: false, hasAudio: true, muted: true });"
                      "var quiet = media({ paused: false, hasAudio: true, volume: 0 });"));
    QCOMPARE(run(PageMedia::Command::Query, QStringLiteral("page([video])")),
             QStringLiteral("playing"));
    QCOMPARE(run(PageMedia::Command::Query, QStringLiteral("page([audio])")),
             QStringLiteral("playing"));
    QCOMPARE(run(PageMedia::Command::Query, QStringLiteral("page([silent, hushed, quiet])")),
             QString());

    js(QStringLiteral("var framed = page([], [{ contentDocument: page([audio]) }]);"
                      "var foreign = page([], [{ contentDocument: null },"
                      " { get contentDocument() { throw new Error('denied'); } }]);"));
    QCOMPARE(run(PageMedia::Command::Query, QStringLiteral("framed")), QStringLiteral("playing"));
    QCOMPARE(run(PageMedia::Command::Query, QStringLiteral("foreign")), QString());

    // Pause: playing media paused and marked. Page's own paused media not marked, not resumed.
    js(QStringLiteral("var own = media({ paused: true, hasAudio: true });"));
    QCOMPARE(run(PageMedia::Command::Pause, QStringLiteral("page([video, own, silent])")),
             QStringLiteral("paused"));
    QVERIFY(js(QStringLiteral("video.paused && video.salamaPaused === true")).toBool());
    QVERIFY(js(QStringLiteral("own.salamaPaused === undefined")).toBool());
    QVERIFY(!js(QStringLiteral("silent.paused")).toBool());
    QCOMPARE(run(PageMedia::Command::Play, QStringLiteral("page([video, own])")),
             QStringLiteral("playing"));
    QVERIFY(js(QStringLiteral("!video.paused && video.salamaPaused === undefined")).toBool());
    QVERIFY(js(QStringLiteral("own.paused && own.plays === 0")).toBool());
    // play() without promise (older engines) still works.
    js(QStringLiteral("var old = media({ paused: false, hasAudio: true });"
                      "old.play = function () { this.paused = false; };"));
    run(PageMedia::Command::Pause, QStringLiteral("page([old])"));
    QCOMPARE(run(PageMedia::Command::Play, QStringLiteral("page([old])")),
             QStringLiteral("playing"));
    run(PageMedia::Command::Pause, QStringLiteral("page([video])"));
    js(QStringLiteral("video.ended = true;"));
    QCOMPARE(run(PageMedia::Command::Query, QStringLiteral("page([video])")), QString());
    js(QStringLiteral("video.ended = false; video.paused = false;"));

    // Mute: all not page-muted get muted + marked. Still reports playing: tab shows muted,
    // not silent.
    tabs.setMuted(id, true);
    QCOMPARE(run(PageMedia::Command::Query, QStringLiteral("page([video, hushed])")),
             QStringLiteral("playing"));
    QVERIFY(js(QStringLiteral("video.muted && video.salamaMuted === true")).toBool());
    QVERIFY(js(QStringLiteral("hushed.muted && hushed.salamaMuted === undefined")).toBool());
    tabs.setMuted(id, false);
    QCOMPARE(run(PageMedia::Command::Query, QStringLiteral("page([video, hushed])")),
             QStringLiteral("playing"));
    QVERIFY(js(QStringLiteral("!video.muted && video.salamaMuted === undefined")).toBool());
    QVERIFY(js(QStringLiteral("hushed.muted")).toBool());

    tabs.setMuted(id, true);
    QCOMPARE(run(PageMedia::Command::Pause, QStringLiteral("page([video])")),
             QStringLiteral("paused"));
    QVERIFY(
        js(QStringLiteral("video.paused && video.muted && video.salamaPaused === true")).toBool());
    tabs.setMuted(id, false);
    QCOMPARE(run(PageMedia::Command::Play, QStringLiteral("page([video])")),
             QStringLiteral("playing"));
    QVERIFY(js(QStringLiteral("!video.paused && !video.muted")).toBool());

    // Background: playing video hidden, page's visibility kept; paused video and <audio> untouched.
    media.setBackground(true);
    js(QStringLiteral("video.style.visibility = 'visible';"
                      "var still = media({ paused: true, hasAudio: true });"));
    QCOMPARE(run(PageMedia::Command::Query, QStringLiteral("page([video, still, audio])")),
             QStringLiteral("playing"));
    QVERIFY(js(QStringLiteral("video.style.visibility === 'hidden'"
                              " && video.salamaConcealed === 'visible'"))
                .toBool());
    QVERIFY(js(QStringLiteral("still.style.visibility === '' && !('salamaConcealed' in still)"))
                .toBool());
    QVERIFY(js(QStringLiteral("audio.style.visibility === '' && !('salamaConcealed' in audio)"))
                .toBool());
    run(PageMedia::Command::Query, QStringLiteral("page([video])"));
    QVERIFY(js(QStringLiteral("video.salamaConcealed === 'visible'")).toBool());
    media.setBackground(false);
    run(PageMedia::Command::Query, QStringLiteral("page([video])"));
    QVERIFY(js(QStringLiteral("video.style.visibility === 'visible'"
                              " && !('salamaConcealed' in video)"))
                .toBool());
}

void tst_pagemedia::scriptSaysWhatPlays()
{
    TabModel tabs(nullptr);
    PageMedia media(&tabs, Delay);
    const int id = tabs.newTab(QStringLiteral("https://a.example/"));
    QJSEngine engine;
    QVERIFY(!engine.evaluate(QString::fromUtf8(FakeDom)).isError());
    const auto said = [&](const QString &page) {
        engine.globalObject().setProperty(QStringLiteral("script"),
                                          media.script(id, PageMedia::Command::Query));
        const QJSValue result = engine.evaluate(QStringLiteral("run(script, %1)").arg(page));
        return QJsonDocument::fromJson(result.toString().toUtf8()).object();
    };
    const auto js = [&](const QString &code) { return engine.evaluate(code); };
    const QString title = QStringLiteral("title");
    const QString artist = QStringLiteral("artist");
    const QString artwork = QStringLiteral("artwork");

    js(QStringLiteral("var song = media({ paused: false });"
                      "var film = media({ paused: false, hasAudio: true,"
                      " poster: 'https://a.example/poster.jpg' });"));
    QCOMPARE(said(QStringLiteral("page([song])")),
             QJsonObject({{QStringLiteral("state"), QStringLiteral("playing")}}));

    QCOMPARE(said(QStringLiteral("page([song, film])")).value(artwork).toString(),
             QStringLiteral("https://a.example/poster.jpg"));

    js(QStringLiteral(
        "navigator.mediaSession.metadata = { title: 'Symphony No. 5', artist: 'Beethoven',"
        " artwork: [{ src: 'https://a.example/96.png', sizes: '96x96' },"
        "           { src: 'https://a.example/512.png', sizes: '256x256 512x512' },"
        "           { src: 'https://a.example/128.png', sizes: '128x128' }] };"));
    QJsonObject answer = said(QStringLiteral("page([film])"));
    QCOMPARE(answer.value(title).toString(), QStringLiteral("Symphony No. 5"));
    QCOMPARE(answer.value(artist).toString(), QStringLiteral("Beethoven"));
    QCOMPARE(answer.value(artwork).toString(), QStringLiteral("https://a.example/512.png"));
    js(QStringLiteral("navigator.mediaSession.metadata.artwork = ["
                      " { src: 'https://a.example/any.svg', sizes: 'any' },"
                      " { src: 'https://a.example/512.png', sizes: '512x512' }];"));
    QCOMPARE(said(QStringLiteral("page([film])")).value(artwork).toString(),
             QStringLiteral("https://a.example/any.svg"));
    js(QStringLiteral(
        "navigator.mediaSession.metadata.artwork = ["
        " { src: 'https://a.example/one.png' }, { src: 'https://a.example/two.png' }];"));
    QCOMPARE(said(QStringLiteral("page([film])")).value(artwork).toString(),
             QStringLiteral("https://a.example/two.png"));
    js(QStringLiteral("navigator.mediaSession.metadata.artwork = [];"));
    QCOMPARE(said(QStringLiteral("page([film])")).value(artwork).toString(),
             QStringLiteral("https://a.example/poster.jpg"));

    js(QStringLiteral("film.paused = true; film.salamaPaused = true;"));
    QCOMPARE(said(QStringLiteral("page([film])")).value(title).toString(),
             QStringLiteral("Symphony No. 5"));
    js(QStringLiteral("delete film.salamaPaused;"));
    QCOMPARE(said(QStringLiteral("page([film])")),
             QJsonObject({{QStringLiteral("state"), QString()}}));

    js(QStringLiteral("navigator = {};"));
    QCOMPARE(said(QStringLiteral("page([song])")).value(QStringLiteral("state")).toString(),
             QStringLiteral("playing"));
}

void tst_pagemedia::answersBecomeTheTabsState()
{
    TabModel tabs(nullptr);
    PageMedia media(&tabs, Delay);
    const int id = tabs.newTab(QStringLiteral("https://a.example/"));

    media.answer(id, PageMedia::Command::Query, QStringLiteral("playing"));
    QCOMPARE(tabs.mediaState(id), TabModel::MediaPlaying);
    media.answer(id, PageMedia::Command::Query, QStringLiteral("paused"));
    QCOMPARE(tabs.mediaState(id), TabModel::MediaPaused);
    media.answer(id, PageMedia::Command::Query, QString());
    QCOMPARE(tabs.mediaState(id), TabModel::NoMedia);

    // Only known words; anything else (failed script too) = not playing.
    media.answer(id, PageMedia::Command::Query, QStringLiteral("playing"));
    media.answer(id, PageMedia::Command::Query, QVariant());
    QCOMPARE(tabs.mediaState(id), TabModel::NoMedia);
    media.answer(id, PageMedia::Command::Query, QStringLiteral("playing"));
    media.answer(id, PageMedia::Command::Query, 1);
    QCOMPARE(tabs.mediaState(id), TabModel::NoMedia);
    media.answer(id, PageMedia::Command::Query, QStringLiteral("Playing"));
    QCOMPARE(tabs.mediaState(id), TabModel::NoMedia);

    media.answer(
        id, PageMedia::Command::Query,
        QStringLiteral("{\"state\":\"playing\",\"title\":\" Symphony\\n No. 5 \","
                       "\"artist\":\"Beethoven\",\"artwork\":\"https://a.example/5.png\"}"));
    QCOMPARE(tabs.mediaState(id), TabModel::MediaPlaying);
    QCOMPARE(tabs.activeMediaTitle(), QStringLiteral("Symphony No. 5"));
    QCOMPARE(tabs.activeMediaArtist(), QStringLiteral("Beethoven"));
    QCOMPARE(tabs.activeMediaArtwork(), QStringLiteral("https://a.example/5.png"));
    for (const QString &unreachable :
         {QStringLiteral("blob:https://a.example/1"), QStringLiteral("data:image/png;base64,AA"),
          QStringLiteral("file:///etc/passwd"), QStringLiteral("5.png")}) {
        media.answer(id, PageMedia::Command::Query,
                     QStringLiteral("{\"state\":\"paused\",\"artwork\":\"%1\"}").arg(unreachable));
        QCOMPARE(tabs.mediaState(id), TabModel::MediaPaused);
        QCOMPARE(tabs.activeMediaArtwork(), QString());
    }
    QCOMPARE(tabs.activeMediaTitle(), QString());
    media.answer(id, PageMedia::Command::Query,
                 QStringLiteral("{\"state\":\"playing\",\"title\":5,\"artist\":null}"));
    QCOMPARE(tabs.mediaState(id), TabModel::MediaPlaying);
    QCOMPARE(tabs.activeMediaTitle(), QString());
    media.answer(id, PageMedia::Command::Query, QStringLiteral("{\"state\":1,\"title\":\"A\"}"));
    QCOMPARE(tabs.mediaState(id), TabModel::NoMedia);
    QCOMPARE(tabs.activeMediaTitle(), QString());

    media.answer(id, PageMedia::Command::Query,
                 QStringLiteral("{\"state\":\"playing\",\"title\":\"Symphony No. 5\"}"));
    media.forget(id);
    QCOMPARE(tabs.mediaState(id), TabModel::NoMedia);
    QCOMPARE(tabs.activeMediaTitle(), QString());

    media.answer(id + 1, PageMedia::Command::Query, QStringLiteral("playing"));
    QCOMPARE(tabs.mediaState(id + 1), TabModel::NoMedia);
}

void tst_pagemedia::theTabInFrontPausesTheOthers()
{
    TabModel tabs(nullptr);
    PageMedia media(&tabs, Delay);
    const int first = tabs.newTab(QStringLiteral("https://a.example/"));
    const int second = tabs.newTab(QStringLiteral("https://b.example/"));
    const int third = tabs.newTab(QStringLiteral("https://c.example/"));
    QSignalSpy spy(&media, &PageMedia::requested);

    // Two background tabs claim playing (stale page keeps saying so); no action while front silent.
    media.answer(first, PageMedia::Command::Query, QStringLiteral("playing"));
    media.answer(second, PageMedia::Command::Query, QStringLiteral("playing"));
    QVERIFY(spy.isEmpty());

    QCOMPARE(tabs.activeTabId(), third);
    media.answer(third, PageMedia::Command::Query, QStringLiteral("playing"));
    QCOMPARE(requests(spy), QStringList({request(first, PageMedia::Command::Pause),
                                         request(second, PageMedia::Command::Pause)}));
    spy.clear();
    media.answer(first, PageMedia::Command::Pause, QStringLiteral("paused"));
    media.answer(second, PageMedia::Command::Pause, QStringLiteral("paused"));
    QVERIFY(spy.isEmpty());
    QCOMPARE(tabs.mediaState(first), TabModel::MediaPaused);

    media.answer(third, PageMedia::Command::Query, QStringLiteral("playing"));
    QVERIFY(spy.isEmpty());
}

void tst_pagemedia::aTabBehindIsPausedWhileTheFrontPlays()
{
    TabModel tabs(nullptr);
    PageMedia media(&tabs, Delay);
    const int behind = tabs.newTab(QStringLiteral("https://a.example/"));
    const int front = tabs.newTab(QStringLiteral("https://b.example/"));
    QSignalSpy spy(&media, &PageMedia::requested);

    media.answer(front, PageMedia::Command::Query, QStringLiteral("playing"));
    QVERIFY(spy.isEmpty());
    media.answer(behind, PageMedia::Command::Query, QStringLiteral("playing"));
    QCOMPARE(requests(spy), QStringList({request(behind, PageMedia::Command::Pause)}));

    // Page refusing pause still plays, not re-asked (would loop forever). Next engine event
    // asks once more.
    spy.clear();
    media.answer(behind, PageMedia::Command::Pause, QStringLiteral("playing"));
    QCOMPARE(tabs.mediaState(behind), TabModel::MediaPlaying);
    QVERIFY(spy.isEmpty());
    media.answer(behind, PageMedia::Command::Query, QStringLiteral("playing"));
    QCOMPARE(requests(spy), QStringList({request(behind, PageMedia::Command::Pause)}));
}

void tst_pagemedia::aTabBehindMayPlayWhileTheFrontIsSilent()
{
    TabModel tabs(nullptr);
    PageMedia media(&tabs, Delay);
    const int behind = tabs.newTab(QStringLiteral("https://a.example/"));
    const int front = tabs.newTab(QStringLiteral("https://b.example/"));
    QSignalSpy spy(&media, &PageMedia::requested);

    media.answer(front, PageMedia::Command::Query, QStringLiteral("paused"));
    media.answer(behind, PageMedia::Command::Query, QStringLiteral("playing"));
    QVERIFY(spy.isEmpty());
    QCOMPARE(tabs.mediaState(behind), TabModel::MediaPlaying);
}

void tst_pagemedia::toggleMuted()
{
    TabModel tabs(nullptr);
    PageMedia media(&tabs, Delay);
    const int behind = tabs.newTab(QStringLiteral("https://a.example/"));
    const int id = tabs.newTab(QStringLiteral("https://b.example/"));
    QSignalSpy spy(&media, &PageMedia::requested);

    media.answer(id, PageMedia::Command::Query, QStringLiteral("playing"));
    QVERIFY(media.isHeard(id));
    media.toggleMuted(id);
    QVERIFY(tabs.isMuted(id));
    QVERIFY(!media.isHeard(id));
    QCOMPARE(requests(spy), QStringList({request(id, PageMedia::Command::Pause)}));

    spy.clear();
    media.answer(id, PageMedia::Command::Pause, QStringLiteral("paused"));
    media.toggleMuted(id);
    QVERIFY(!tabs.isMuted(id));
    QCOMPARE(requests(spy), QStringList({request(id, PageMedia::Command::Play)}));
    spy.clear();
    media.answer(id, PageMedia::Command::Play, QStringLiteral("paused"));
    QVERIFY(!media.isHeard(id));
    media.toggleMuted(id);
    QCOMPARE(requests(spy), QStringList({request(id, PageMedia::Command::Play)}));
    spy.clear();
    tabs.setMuted(id, true);
    media.answer(id, PageMedia::Command::Query, QStringLiteral("playing"));
    QVERIFY(!media.isHeard(id));
    media.toggleMuted(id);
    QVERIFY(!tabs.isMuted(id));
    QCOMPARE(requests(spy), QStringList({request(id, PageMedia::Command::Play)}));

    spy.clear();
    QVERIFY(media.isHeard(id));
    media.answer(behind, PageMedia::Command::Query, QStringLiteral("paused"));
    QVERIFY(!media.isHeard(behind));
    media.toggleMuted(behind);
    QCOMPARE(tabs.activeTabId(), behind);
    QCOMPARE(requests(spy), QStringList({request(id, PageMedia::Command::Pause),
                                         request(behind, PageMedia::Command::Play)}));

    spy.clear();
    media.toggleMuted(id + 1);
    QVERIFY(!tabs.isMuted(id + 1));
    QVERIFY(spy.isEmpty());
}

void tst_pagemedia::aTabLeftIsHeldUntilItIsBack()
{
    TabModel tabs(nullptr);
    PageMedia media(&tabs, Delay);
    const int first = tabs.newTab(QStringLiteral("https://a.example/"));
    const int second = tabs.newTab(QStringLiteral("https://b.example/"));
    QSignalSpy spy(&media, &PageMedia::requested);
    // Asked while leaving page still front: before view and page learn it's hidden.
    int frontWhenAsked = 0;
    connect(&media, &PageMedia::requested, this,
            [&tabs, &frontWhenAsked]() { frontWhenAsked = tabs.activeTabId(); });

    tabs.activateTabById(first);
    QVERIFY(spy.isEmpty());

    tabs.activateTabById(second);
    media.answer(second, PageMedia::Command::Query, QStringLiteral("playing"));
    tabs.activateTabById(first);
    QCOMPARE(requests(spy), QStringList({request(second, PageMedia::Command::Pause)}));
    QCOMPARE(frontWhenAsked, second);
    media.answer(second, PageMedia::Command::Pause, QStringLiteral("paused"));
    QCOMPARE(tabs.shownMediaState(second), TabModel::MediaPaused);
    QVERIFY(!media.isHeard(second));

    spy.clear();
    tabs.activateTabById(second);
    QCOMPARE(requests(spy), QStringList({request(second, PageMedia::Command::Play)}));
    QCOMPARE(frontWhenAsked, second);
    spy.clear();
    media.answer(second, PageMedia::Command::Play, QStringLiteral("paused"));
    tabs.activateTabById(first);
    tabs.activateTabById(second);
    QVERIFY(spy.isEmpty());

    media.answer(second, PageMedia::Command::Query, QStringLiteral("playing"));
    tabs.activateTabById(first);
    media.forget(second);
    spy.clear();
    tabs.activateTabById(second);
    QVERIFY(spy.isEmpty());
}

void tst_pagemedia::outOfSightThePagesAreAskedAgain()
{
    TabModel tabs(nullptr);
    tabs.newTab(QStringLiteral("https://a.example/"));
    PageMedia media(&tabs, Delay);
    QSignalSpy spy(&media, &PageMedia::requested);

    media.setBackground(true);
    QCOMPARE(requests(spy), QStringList({request(0, PageMedia::Command::Query)}));
    spy.clear();
    media.setBackground(true);
    QTest::qWait(Delay * 5);
    QVERIFY(spy.isEmpty());
    media.setBackground(false);
    QCOMPARE(requests(spy), QStringList({request(0, PageMedia::Command::Query)}));
}

void tst_pagemedia::refreshAsksEveryPageOnce()
{
    TabModel tabs(nullptr);
    tabs.newTab(QStringLiteral("https://a.example/"));
    PageMedia media(&tabs, Delay);
    QSignalSpy spy(&media, &PageMedia::requested);

    // Several decoders at once -> pages asked once, after delay.
    media.refresh();
    media.refresh();
    media.refresh();
    QVERIFY(spy.isEmpty());
    QVERIFY(spy.wait(Delay * 50));
    QTest::qWait(Delay * 3);
    QCOMPARE(requests(spy), QStringList({request(0, PageMedia::Command::Query)}));
}

void tst_pagemedia::anotherTabInFrontAsksAgain()
{
    TabModel tabs(nullptr);
    const int first = tabs.newTab(QStringLiteral("https://a.example/"));
    tabs.newTab(QStringLiteral("https://b.example/"));
    PageMedia media(&tabs, Delay);
    QSignalSpy spy(&media, &PageMedia::requested);

    tabs.activateTabById(first);
    QVERIFY(spy.wait(Delay * 50));
    QCOMPARE(requests(spy), QStringList({request(0, PageMedia::Command::Query)}));

    spy.clear();
    tabs.moveTab(0, 1);
    QTest::qWait(Delay * 5);
    QVERIFY(spy.isEmpty());
}

QTEST_GUILESS_MAIN(tst_pagemedia)
#include "tst_pagemedia.moc"
