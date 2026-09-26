// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "PageMedia.h"

#include "tabs/TabModel.h"

namespace Salama {

namespace {

// What the script answers.
const QString Playing = QStringLiteral("playing");
const QString Paused = QStringLiteral("paused");

// The page's own media, and that of its frames from the same site: a frame from
// another site has a document the page cannot reach, and a null contentDocument says
// so. Of each element: the tab's muted flag applied to it -- and taken off again only
// where it was this browser that put it on (salamaMuted); then the command. Paused
// from here, an element is marked (salamaPaused), and play resumes what is marked and
// nothing else: the button undoes the pause, it does not start what the page has not.
// What counts is what makes a sound: an element the page has muted or turned down to
// nothing, or a video the engine says has no sound track (Gecko's mozHasAudio,
// undefined on an <audio>, which has nothing else), plays nothing anyone hears.
// While the application is out of sight (concealed), a video that plays is hidden --
// its own visibility, so the page's layout stays as it is -- and shown again once the
// application is back (salamaConcealed keeps what the page had set): Gecko stops
// decoding the pictures of a video nobody can see, and what plays on is its sound.
const char *const ScriptTemplate = R"( var command = '%1', muted = %2, concealed = %3;
 var media = [];
 var collect = function (doc) {
   var found = doc.querySelectorAll('audio, video');
   for (var i = 0; i < found.length; ++i) { media.push(found[i]); }
   var frames = doc.querySelectorAll('iframe, frame');
   for (var j = 0; j < frames.length; ++j) {
     try { if (frames[j].contentDocument) { collect(frames[j].contentDocument); } } catch (e) {}
   }
 };
 collect(document);
 var playing = false, paused = false;
 media.forEach(function (m) {
   if (muted && !m.muted) { m.muted = true; m.salamaMuted = true; }
   else if (!muted && m.salamaMuted) { m.muted = false; delete m.salamaMuted; }
   if (concealed && m.localName === 'video' && !m.paused && !('salamaConcealed' in m)) {
     m.salamaConcealed = m.style.visibility; m.style.visibility = 'hidden';
   } else if (!concealed && 'salamaConcealed' in m) {
     m.style.visibility = m.salamaConcealed; delete m.salamaConcealed;
   }
   if (m.mozHasAudio === false || m.volume === 0 || (m.muted && !m.salamaMuted)) { return; }
   if (command === 'pause' && !m.paused) { m.pause(); m.salamaPaused = true; }
   else if (command === 'play' && m.salamaPaused) { var p = m.play(); if (p) { p.catch(function () {}); } }
   if (!m.paused && !m.ended) { playing = true; delete m.salamaPaused; }
   else if (m.salamaPaused && !m.ended) { paused = true; }
 });
 return playing ? 'playing' : paused ? 'paused' : '';)";

QString commandName(PageMedia::Command command)
{
    switch (command) {
    case PageMedia::Command::Pause:
        return QStringLiteral("pause");
    case PageMedia::Command::Play:
        return QStringLiteral("play");
    case PageMedia::Command::Query:
        break;
    }
    return QStringLiteral("query");
}

} // namespace

PageMedia::PageMedia(TabModel *tabs, QObject *parent)
    : PageMedia(tabs, DefaultQueryDelay, parent)
{
}

PageMedia::PageMedia(TabModel *tabs, int queryDelay, QObject *parent)
    : QObject(parent)
    , m_tabs(tabs)
{
    m_queryTimer.setSingleShot(true);
    m_queryTimer.setInterval(queryDelay);
    connect(&m_queryTimer, &QTimer::timeout, this, [this]() { request(0, Command::Query); });
    // A tab being left is paused while its page is still the one on the screen: told
    // it is hidden, a page may pause itself where nothing here can play it again.
    connect(m_tabs, &TabModel::activeTabLeaving, this, [this](int tabId) {
        if (m_tabs->mediaState(tabId) == TabModel::MediaPlaying) {
            m_held.insert(tabId);
            request(tabId, Command::Pause);
        }
    });
    // Back in front, it plays again what was held; and every page is asked.
    m_frontTabId = m_tabs->activeTabId();
    connect(m_tabs, &TabModel::activeTabChanged, this, [this]() {
        if (m_tabs->activeTabId() != m_frontTabId) {
            m_frontTabId = m_tabs->activeTabId();
            if (m_held.remove(m_frontTabId)) {
                request(m_frontTabId, Command::Play);
            }
            refresh();
        }
    });
}

int PageMedia::queryDelay() const
{
    return m_queryTimer.interval();
}

QString PageMedia::script(int tabId, int command) const
{
    return script(tabId, static_cast<Command>(command));
}

QString PageMedia::script(int tabId, Command command) const
{
    return QString::fromUtf8(ScriptTemplate)
        .arg(commandName(command),
             m_tabs->isMuted(tabId) ? QStringLiteral("true") : QStringLiteral("false"),
             m_background ? QStringLiteral("true") : QStringLiteral("false"));
}

void PageMedia::answer(int tabId, int command, const QVariant &answer)
{
    this->answer(tabId, static_cast<Command>(command), answer);
}

void PageMedia::answer(int tabId, Command command, const QVariant &answer)
{
    const QString said = answer.userType() == QMetaType::QString ? answer.toString() : QString();
    TabModel::MediaState state = TabModel::NoMedia;
    if (said == Playing) {
        state = TabModel::MediaPlaying;
    } else if (said == Paused) {
        state = TabModel::MediaPaused;
    }
    m_tabs->setMediaState(tabId, state);
    if (command == Command::Pause || m_tabs->mediaState(tabId) != TabModel::MediaPlaying) {
        return;
    }
    // One tab plays at a time, and it is the one in front: it pauses the others, and
    // one behind it that plays while it does is paused.
    const int front = m_tabs->activeTabId();
    if (tabId != front) {
        if (m_tabs->mediaState(front) == TabModel::MediaPlaying) {
            request(tabId, Command::Pause);
        }
        return;
    }
    for (const Tab &tab : m_tabs->tabs()) {
        if (tab.id != front && m_tabs->mediaState(tab.id) == TabModel::MediaPlaying) {
            request(tab.id, Command::Pause);
        }
    }
}

void PageMedia::forget(int tabId)
{
    m_held.remove(tabId);
    m_tabs->setMediaState(tabId, TabModel::NoMedia);
}

void PageMedia::toggleMuted(int tabId)
{
    if (m_tabs->indexOf(tabId) < 0) {
        return;
    }
    if (isHeard(tabId)) {
        m_tabs->setMuted(tabId, true);
        request(tabId, Command::Pause);
        return;
    }
    m_tabs->setMuted(tabId, false);
    // Behind the front the engine holds whatever the page plays: it is played there.
    if (tabId != m_tabs->activeTabId()) {
        m_held.insert(tabId);
        m_tabs->activateTabById(tabId);
        return;
    }
    request(tabId, Command::Play);
}

bool PageMedia::isHeard(int tabId) const
{
    return m_tabs->shownMediaState(tabId) == TabModel::MediaPlaying && !m_tabs->isMuted(tabId);
}

void PageMedia::setBackground(bool background)
{
    // At once, not with the engine's words: on the way back, the pictures are wanted
    // the moment the page is.
    if (m_background != background) {
        m_background = background;
        request(0, Command::Query);
    }
}

void PageMedia::refresh()
{
    if (!m_queryTimer.isActive()) {
        m_queryTimer.start();
    }
}

void PageMedia::request(int tabId, Command command)
{
    emit requested(tabId, static_cast<int>(command));
}

} // namespace Salama
