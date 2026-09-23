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
const char *const ScriptTemplate = R"( var command = '%1', muted = %2;
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
   if (m.mozHasAudio === false || m.volume === 0 || (m.muted && !m.salamaMuted)) { return; }
   if (command === 'pause' && !m.paused) { m.pause(); m.salamaPaused = true; }
   else if (command === 'play' && m.salamaPaused) { var p = m.play(); if (p) { p.catch(function () {}); } }
   if (!m.paused && !m.ended) { playing = true; delete m.salamaPaused; }
   else if (m.salamaPaused && !m.ended) { paused = true; }
 });
 return playing ? 'playing' : paused ? 'paused' : '';)";

QString commandName(int command)
{
    switch (command) {
    case PageMedia::Pause:
        return QStringLiteral("pause");
    case PageMedia::Play:
        return QStringLiteral("play");
    default:
        return QStringLiteral("query");
    }
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
    connect(&m_queryTimer, &QTimer::timeout, this, [this]() { emit requested(0, Query); });
    // A page brought to the front plays again what the engine held while it was
    // behind, and the one left may still say it plays: both are asked.
    m_frontTabId = m_tabs->activeTabId();
    connect(m_tabs, &TabModel::activeTabChanged, this, [this]() {
        if (m_tabs->activeTabId() != m_frontTabId) {
            m_frontTabId = m_tabs->activeTabId();
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
    return QString::fromUtf8(ScriptTemplate)
        .arg(commandName(command),
             m_tabs->isMuted(tabId) ? QStringLiteral("true") : QStringLiteral("false"));
}

void PageMedia::answer(int tabId, int command, const QVariant &answer)
{
    const QString said = answer.userType() == QMetaType::QString ? answer.toString() : QString();
    TabModel::MediaState state = TabModel::NoMedia;
    if (said == Playing) {
        state = TabModel::MediaPlaying;
    } else if (said == Paused) {
        state = TabModel::MediaPaused;
    }
    m_tabs->setMediaState(tabId, state);
    if (command == Pause || m_tabs->mediaState(tabId) != TabModel::MediaPlaying) {
        return;
    }
    // One tab plays at a time, and it is the one in front: it pauses the others, and
    // one behind it that plays while it does is paused.
    const int front = m_tabs->activeTabId();
    if (tabId != front) {
        if (m_tabs->mediaState(front) == TabModel::MediaPlaying) {
            emit requested(tabId, Pause);
        }
        return;
    }
    for (const Tab &tab : m_tabs->tabs()) {
        if (tab.id != front && m_tabs->mediaState(tab.id) == TabModel::MediaPlaying) {
            emit requested(tab.id, Pause);
        }
    }
}

void PageMedia::forget(int tabId)
{
    m_tabs->setMediaState(tabId, TabModel::NoMedia);
}

void PageMedia::togglePlayback(int tabId)
{
    const TabModel::MediaState state = m_tabs->mediaState(tabId);
    if (state == TabModel::NoMedia) {
        return;
    }
    // Behind the one in front a page's media is held, whatever it says: it is played
    // by bringing it to the front, where the engine lets go of what it held, and what
    // was paused from here is played with it.
    if (tabId != m_tabs->activeTabId()) {
        m_tabs->activateTabById(tabId);
        emit requested(tabId, Play);
        return;
    }
    emit requested(tabId, state == TabModel::MediaPlaying ? Pause : Play);
}

void PageMedia::toggleMuted(int tabId)
{
    if (m_tabs->indexOf(tabId) < 0) {
        return;
    }
    m_tabs->setMuted(tabId, !m_tabs->isMuted(tabId));
    emit requested(tabId, Query);
}

void PageMedia::refresh()
{
    if (!m_queryTimer.isActive()) {
        m_queryTimer.start();
    }
}

} // namespace Salama
