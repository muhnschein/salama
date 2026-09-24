// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "engine/PageMedia.h"
#include "tabs/TabModel.h"

#include <QJSEngine>
#include <QSignalSpy>
#include <QtTest>

using Salama::PageMedia;
using Salama::TabModel;

namespace {

// Short enough to wait out.
const int Delay = 20;

// The requests raised so far, as "tab:command".
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

// A page for the script to run over, in a JavaScript engine rather than a browser's:
// just the parts of the DOM it touches. `media` makes an element: playing or paused,
// muted or not, and with what the engine says of its sound (hasAudio), a <video> -- or
// an <audio> when that is left out, Gecko having mozHasAudio on a video only. `page`
// makes a document of elements and frames; a frame from another site has no document.
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
)";

} // namespace

class tst_pagemedia : public QObject
{
    Q_OBJECT

private slots:
    void scriptCarriesTheCommandAndTheMute();
    void scriptOverAPage();
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

    // The body of a function, as every script the engine runs: it has to return.
    const QString query = media.script(id, PageMedia::Query);
    QVERIFY(query.contains(QLatin1String("return ")));
    QVERIFY(
        query.contains(QLatin1String("var command = 'query', muted = false, concealed = false;")));
    QVERIFY(media.script(id, PageMedia::Pause).contains(QLatin1String("'pause'")));
    QVERIFY(media.script(id, PageMedia::Play).contains(QLatin1String("'play'")));
    // It reaches frames, and every kind of media element.
    QVERIFY(query.contains(QLatin1String("contentDocument")));
    QVERIFY(query.contains(QLatin1String("'audio, video'")));

    // A muted tab's script mutes what it finds; out of sight, it hides what plays.
    tabs.setMuted(id, true);
    QVERIFY(media.script(id, PageMedia::Query).contains(QLatin1String("muted = true,")));
    media.setBackground(true);
    QVERIFY(media.script(id, PageMedia::Query).contains(QLatin1String("concealed = true;")));
}

// The script itself, run over a page made of the parts of the DOM it reads.
void tst_pagemedia::scriptOverAPage()
{
    TabModel tabs(nullptr);
    PageMedia media(&tabs, Delay);
    const int id = tabs.newTab(QStringLiteral("https://a.example/"));
    QJSEngine engine;
    QVERIFY(!engine.evaluate(QString::fromUtf8(FakeDom)).isError());
    // The script for a command, run over a page; what it answers, or its error.
    const auto run = [&](PageMedia::Command command, const QString &page) {
        engine.globalObject().setProperty(QStringLiteral("script"), media.script(id, command));
        const QJSValue result = engine.evaluate(QStringLiteral("run(script, %1)").arg(page));
        return result.isError() ? QStringLiteral("error: ") + result.toString() : result.toString();
    };
    const auto js = [&](const QString &code) { return engine.evaluate(code); };

    // Nothing on the page, nothing playing.
    QCOMPARE(run(PageMedia::Query, QStringLiteral("page([])")), QString());

    // A video with sound, playing; an <audio>, which has nothing but sound, playing.
    js(QStringLiteral("var video = media({ paused: false, hasAudio: true });"
                      "var audio = media({ paused: false });"
                      "var silent = media({ paused: false, hasAudio: false });"
                      "var hushed = media({ paused: false, hasAudio: true, muted: true });"
                      "var quiet = media({ paused: false, hasAudio: true, volume: 0 });"));
    QCOMPARE(run(PageMedia::Query, QStringLiteral("page([video])")), QStringLiteral("playing"));
    QCOMPARE(run(PageMedia::Query, QStringLiteral("page([audio])")), QStringLiteral("playing"));
    // Nothing anyone hears: no sound track, muted by the page, turned down to nothing.
    QCOMPARE(run(PageMedia::Query, QStringLiteral("page([silent, hushed, quiet])")), QString());

    // Frames from the same site are searched; one from another has no document, and
    // one whose document throws is passed over.
    js(QStringLiteral("var framed = page([], [{ contentDocument: page([audio]) }]);"
                      "var foreign = page([], [{ contentDocument: null },"
                      " { get contentDocument() { throw new Error('denied'); } }]);"));
    QCOMPARE(run(PageMedia::Query, QStringLiteral("framed")), QStringLiteral("playing"));
    QCOMPARE(run(PageMedia::Query, QStringLiteral("foreign")), QString());

    // Paused: what played is paused and marked, and the page says so. What the page
    // itself had paused is not marked, and not played again below.
    js(QStringLiteral("var own = media({ paused: true, hasAudio: true });"));
    QCOMPARE(run(PageMedia::Pause, QStringLiteral("page([video, own, silent])")),
             QStringLiteral("paused"));
    QVERIFY(js(QStringLiteral("video.paused && video.salamaPaused === true")).toBool());
    QVERIFY(js(QStringLiteral("own.salamaPaused === undefined")).toBool());
    // A silent video is not the page's sound, and is left to play.
    QVERIFY(!js(QStringLiteral("silent.paused")).toBool());
    QCOMPARE(run(PageMedia::Play, QStringLiteral("page([video, own])")), QStringLiteral("playing"));
    QVERIFY(js(QStringLiteral("!video.paused && video.salamaPaused === undefined")).toBool());
    QVERIFY(js(QStringLiteral("own.paused && own.plays === 0")).toBool());
    // A play() without a promise, as older engines had, is played all the same.
    js(QStringLiteral("var old = media({ paused: false, hasAudio: true });"
                      "old.play = function () { this.paused = false; };"));
    run(PageMedia::Pause, QStringLiteral("page([old])"));
    QCOMPARE(run(PageMedia::Play, QStringLiteral("page([old])")), QStringLiteral("playing"));
    // Paused from here and ended since: nothing to play again.
    run(PageMedia::Pause, QStringLiteral("page([video])"));
    js(QStringLiteral("video.ended = true;"));
    QCOMPARE(run(PageMedia::Query, QStringLiteral("page([video])")), QString());
    js(QStringLiteral("video.ended = false; video.paused = false;"));

    // Muted: every element the page has not muted itself is muted, and marked. What is
    // muted from here still plays, and says so: the tab shows it muted, not silent.
    tabs.setMuted(id, true);
    QCOMPARE(run(PageMedia::Query, QStringLiteral("page([video, hushed])")),
             QStringLiteral("playing"));
    QVERIFY(js(QStringLiteral("video.muted && video.salamaMuted === true")).toBool());
    QVERIFY(js(QStringLiteral("hushed.muted && hushed.salamaMuted === undefined")).toBool());
    // Unmuted: only what was muted from here is heard again.
    tabs.setMuted(id, false);
    QCOMPARE(run(PageMedia::Query, QStringLiteral("page([video, hushed])")),
             QStringLiteral("playing"));
    QVERIFY(js(QStringLiteral("!video.muted && video.salamaMuted === undefined")).toBool());
    QVERIFY(js(QStringLiteral("hushed.muted")).toBool());

    // Muted from the control, and paused in the same run: the tab says it is paused,
    // and what was paused is muted as well. Unmuted, it plays again, and is heard.
    tabs.setMuted(id, true);
    QCOMPARE(run(PageMedia::Pause, QStringLiteral("page([video])")), QStringLiteral("paused"));
    QVERIFY(
        js(QStringLiteral("video.paused && video.muted && video.salamaPaused === true")).toBool());
    tabs.setMuted(id, false);
    QCOMPARE(run(PageMedia::Play, QStringLiteral("page([video])")), QStringLiteral("playing"));
    QVERIFY(js(QStringLiteral("!video.paused && !video.muted")).toBool());

    // Out of sight, a video that plays is hidden, and what the page had set for its
    // visibility kept; one that is paused, and an <audio>, are left as they are.
    media.setBackground(true);
    js(QStringLiteral("video.style.visibility = 'visible';"
                      "var still = media({ paused: true, hasAudio: true });"));
    QCOMPARE(run(PageMedia::Query, QStringLiteral("page([video, still, audio])")),
             QStringLiteral("playing"));
    QVERIFY(js(QStringLiteral("video.style.visibility === 'hidden'"
                              " && video.salamaConcealed === 'visible'"))
                .toBool());
    QVERIFY(js(QStringLiteral("still.style.visibility === '' && !('salamaConcealed' in still)"))
                .toBool());
    QVERIFY(js(QStringLiteral("audio.style.visibility === '' && !('salamaConcealed' in audio)"))
                .toBool());
    // Asked again, it stays hidden, and what the page had is not lost.
    run(PageMedia::Query, QStringLiteral("page([video])"));
    QVERIFY(js(QStringLiteral("video.salamaConcealed === 'visible'")).toBool());
    // Back on the screen, it is shown as the page had it.
    media.setBackground(false);
    run(PageMedia::Query, QStringLiteral("page([video])"));
    QVERIFY(js(QStringLiteral("video.style.visibility === 'visible'"
                              " && !('salamaConcealed' in video)"))
                .toBool());
}

void tst_pagemedia::answersBecomeTheTabsState()
{
    TabModel tabs(nullptr);
    PageMedia media(&tabs, Delay);
    const int id = tabs.newTab(QStringLiteral("https://a.example/"));

    media.answer(id, PageMedia::Query, QStringLiteral("playing"));
    QCOMPARE(tabs.mediaState(id), TabModel::MediaPlaying);
    media.answer(id, PageMedia::Query, QStringLiteral("paused"));
    QCOMPARE(tabs.mediaState(id), TabModel::MediaPaused);
    media.answer(id, PageMedia::Query, QString());
    QCOMPARE(tabs.mediaState(id), TabModel::NoMedia);

    // Only the words the script says: anything else, a failed script included, is
    // nothing playing.
    media.answer(id, PageMedia::Query, QStringLiteral("playing"));
    media.answer(id, PageMedia::Query, QVariant());
    QCOMPARE(tabs.mediaState(id), TabModel::NoMedia);
    media.answer(id, PageMedia::Query, QStringLiteral("playing"));
    media.answer(id, PageMedia::Query, 1);
    QCOMPARE(tabs.mediaState(id), TabModel::NoMedia);
    media.answer(id, PageMedia::Query, QStringLiteral("Playing"));
    QCOMPARE(tabs.mediaState(id), TabModel::NoMedia);

    // A page going takes what it played with it.
    media.answer(id, PageMedia::Query, QStringLiteral("playing"));
    media.forget(id);
    QCOMPARE(tabs.mediaState(id), TabModel::NoMedia);

    // A tab that is not there has nothing to say.
    media.answer(id + 1, PageMedia::Query, QStringLiteral("playing"));
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

    // Two tabs behind that say they play -- a page left behind keeps saying so --
    // and nothing is done while the one in front is silent.
    media.answer(first, PageMedia::Query, QStringLiteral("playing"));
    media.answer(second, PageMedia::Query, QStringLiteral("playing"));
    QVERIFY(spy.isEmpty());

    // The one in front starts, and both are paused.
    QCOMPARE(tabs.activeTabId(), third);
    media.answer(third, PageMedia::Query, QStringLiteral("playing"));
    QCOMPARE(requests(spy),
             QStringList({request(first, PageMedia::Pause), request(second, PageMedia::Pause)}));
    // What they answer to that is paused, which asks nothing more.
    spy.clear();
    media.answer(first, PageMedia::Pause, QStringLiteral("paused"));
    media.answer(second, PageMedia::Pause, QStringLiteral("paused"));
    QVERIFY(spy.isEmpty());
    QCOMPARE(tabs.mediaState(first), TabModel::MediaPaused);

    // Asked again, the front still plays, and there is nothing left to pause.
    media.answer(third, PageMedia::Query, QStringLiteral("playing"));
    QVERIFY(spy.isEmpty());
}

void tst_pagemedia::aTabBehindIsPausedWhileTheFrontPlays()
{
    TabModel tabs(nullptr);
    PageMedia media(&tabs, Delay);
    const int behind = tabs.newTab(QStringLiteral("https://a.example/"));
    const int front = tabs.newTab(QStringLiteral("https://b.example/"));
    QSignalSpy spy(&media, &PageMedia::requested);

    media.answer(front, PageMedia::Query, QStringLiteral("playing"));
    QVERIFY(spy.isEmpty());
    media.answer(behind, PageMedia::Query, QStringLiteral("playing"));
    QCOMPARE(requests(spy), QStringList({request(behind, PageMedia::Pause)}));

    // A page that will not pause says it still plays, and is not asked again: that
    // would be asking for ever. The next word from the engine asks it once more.
    spy.clear();
    media.answer(behind, PageMedia::Pause, QStringLiteral("playing"));
    QCOMPARE(tabs.mediaState(behind), TabModel::MediaPlaying);
    QVERIFY(spy.isEmpty());
    media.answer(behind, PageMedia::Query, QStringLiteral("playing"));
    QCOMPARE(requests(spy), QStringList({request(behind, PageMedia::Pause)}));
}

void tst_pagemedia::aTabBehindMayPlayWhileTheFrontIsSilent()
{
    TabModel tabs(nullptr);
    PageMedia media(&tabs, Delay);
    const int behind = tabs.newTab(QStringLiteral("https://a.example/"));
    const int front = tabs.newTab(QStringLiteral("https://b.example/"));
    QSignalSpy spy(&media, &PageMedia::requested);

    media.answer(front, PageMedia::Query, QStringLiteral("paused"));
    media.answer(behind, PageMedia::Query, QStringLiteral("playing"));
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

    // Heard: muted, and paused at once, which mutes it as it pauses.
    media.answer(id, PageMedia::Query, QStringLiteral("playing"));
    QVERIFY(media.isHeard(id));
    media.toggleMuted(id);
    QVERIFY(tabs.isMuted(id));
    QVERIFY(!media.isHeard(id));
    QCOMPARE(requests(spy), QStringList({request(id, PageMedia::Pause)}));

    // Not heard: unmuted, and what was paused is played again.
    spy.clear();
    media.answer(id, PageMedia::Pause, QStringLiteral("paused"));
    media.toggleMuted(id);
    QVERIFY(!tabs.isMuted(id));
    QCOMPARE(requests(spy), QStringList({request(id, PageMedia::Play)}));
    // Paused and not muted is not heard either, and is played; so is one that plays
    // muted, which the page started on its own.
    spy.clear();
    media.answer(id, PageMedia::Play, QStringLiteral("paused"));
    QVERIFY(!media.isHeard(id));
    media.toggleMuted(id);
    QCOMPARE(requests(spy), QStringList({request(id, PageMedia::Play)}));
    spy.clear();
    tabs.setMuted(id, true);
    media.answer(id, PageMedia::Query, QStringLiteral("playing"));
    QVERIFY(!media.isHeard(id));
    media.toggleMuted(id);
    QVERIFY(!tabs.isMuted(id));
    QCOMPARE(requests(spy), QStringList({request(id, PageMedia::Play)}));

    // A tab behind the front is played in front: it is brought there, and what played
    // in front is held.
    spy.clear();
    QVERIFY(media.isHeard(id));
    media.answer(behind, PageMedia::Query, QStringLiteral("paused"));
    QVERIFY(!media.isHeard(behind));
    media.toggleMuted(behind);
    QCOMPARE(tabs.activeTabId(), behind);
    QCOMPARE(requests(spy),
             QStringList({request(id, PageMedia::Pause), request(behind, PageMedia::Play)}));

    // A tab that is not there is left alone.
    spy.clear();
    media.toggleMuted(id + 1);
    QVERIFY(!tabs.isMuted(id + 1));
    QVERIFY(spy.isEmpty());
}

// The engine silences a tab behind the front; this pauses it first, as it is left,
// and plays it again when it is back.
void tst_pagemedia::aTabLeftIsHeldUntilItIsBack()
{
    TabModel tabs(nullptr);
    PageMedia media(&tabs, Delay);
    const int first = tabs.newTab(QStringLiteral("https://a.example/"));
    const int second = tabs.newTab(QStringLiteral("https://b.example/"));
    QSignalSpy spy(&media, &PageMedia::requested);
    // Asked while the page being left is still the one in front: before its view hears
    // that it is not, and the page that it is hidden.
    int frontWhenAsked = 0;
    connect(&media, &PageMedia::requested, this,
            [&tabs, &frontWhenAsked]() { frontWhenAsked = tabs.activeTabId(); });

    // Silent, it is left as it is.
    tabs.activateTabById(first);
    QVERIFY(spy.isEmpty());

    // Playing, it is paused as it is left, and shown paused behind the front.
    tabs.activateTabById(second);
    media.answer(second, PageMedia::Query, QStringLiteral("playing"));
    tabs.activateTabById(first);
    QCOMPARE(requests(spy), QStringList({request(second, PageMedia::Pause)}));
    QCOMPARE(frontWhenAsked, second);
    media.answer(second, PageMedia::Pause, QStringLiteral("paused"));
    QCOMPARE(tabs.shownMediaState(second), TabModel::MediaPaused);
    QVERIFY(!media.isHeard(second));

    // Back in front, it plays again, once.
    spy.clear();
    tabs.activateTabById(second);
    QCOMPARE(requests(spy), QStringList({request(second, PageMedia::Play)}));
    QCOMPARE(frontWhenAsked, second);
    spy.clear();
    media.answer(second, PageMedia::Play, QStringLiteral("paused"));
    tabs.activateTabById(first);
    tabs.activateTabById(second);
    QVERIFY(spy.isEmpty());

    // A page that loads while it is held takes the hold with it.
    media.answer(second, PageMedia::Query, QStringLiteral("playing"));
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

    // Out of sight and back, each is carried to every page by asking it once more, at
    // once.
    media.setBackground(true);
    QCOMPARE(requests(spy), QStringList({request(0, PageMedia::Query)}));
    spy.clear();
    media.setBackground(true);
    QTest::qWait(Delay * 5);
    QVERIFY(spy.isEmpty());
    media.setBackground(false);
    QCOMPARE(requests(spy), QStringList({request(0, PageMedia::Query)}));
}

void tst_pagemedia::refreshAsksEveryPageOnce()
{
    TabModel tabs(nullptr);
    tabs.newTab(QStringLiteral("https://a.example/"));
    PageMedia media(&tabs, Delay);
    QSignalSpy spy(&media, &PageMedia::requested);

    // Several decoders report at once; the pages are asked once, a moment later.
    media.refresh();
    media.refresh();
    media.refresh();
    QVERIFY(spy.isEmpty());
    QVERIFY(spy.wait(Delay * 50));
    QTest::qWait(Delay * 3);
    QCOMPARE(requests(spy), QStringList({request(0, PageMedia::Query)}));
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
    QCOMPARE(requests(spy), QStringList({request(0, PageMedia::Query)}));

    // A tab moved in the grid is the same tab in front: nothing is asked.
    spy.clear();
    tabs.moveTab(0, 1);
    QTest::qWait(Delay * 5);
    QVERIFY(spy.isEmpty());
}

QTEST_GUILESS_MAIN(tst_pagemedia)
#include "tst_pagemedia.moc"
