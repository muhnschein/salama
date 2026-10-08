// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "PageMedia.h"

#include "tabs/TabModel.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

namespace Salama {

namespace {

const QString Playing = QStringLiteral("playing");
const QString Paused = QStringLiteral("paused");

// Muted flag removed only where we set it (salamaMuted); play resumes only what we paused
// (salamaPaused). concealed: hide playing video via visibility so Gecko stops decoding;
// salamaConcealed restores page's value.
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
 var playing = false, paused = false, poster = '';
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
   else { return; }
   if (!poster && m.localName === 'video' && m.poster) { poster = m.poster; }
 });
 var said = { state: playing ? 'playing' : paused ? 'paused' : '' };
 var session = said.state && typeof navigator !== 'undefined' && navigator.mediaSession
     ? navigator.mediaSession.metadata : null;
 if (session) {
   said.title = session.title || '';
   said.artist = session.artist || '';
   var widest = -1;
   (session.artwork || []).forEach(function (art) {
     var width = 0;
     String(art.sizes || '').split(/\s+/).forEach(function (size) {
       width = Math.max(width, size === 'any' ? Infinity : parseInt(size, 10) || 0);
     });
     if (art.src && width >= widest) { widest = width; said.artwork = art.src; }
   });
 }
 if (said.state && !said.artwork && poster) { said.artwork = poster; }
 return JSON.stringify(said);)";

// "" = failed answer.
struct Answer
{
    TabModel::MediaState state = TabModel::NoMedia;
    TabModel::MediaMetadata metadata;
};

Answer readAnswer(const QVariant &answer)
{
    Answer read;
    if (answer.userType() != QMetaType::QString) {
        return read;
    }
    const QString text = answer.toString();
    const QJsonDocument json = QJsonDocument::fromJson(text.toUtf8());
    const QJsonObject said = json.object();
    const QString state = json.isObject() ? said.value(QStringLiteral("state")).toString() : text;
    if (state == Playing) {
        read.state = TabModel::MediaPlaying;
    } else if (state == Paused) {
        read.state = TabModel::MediaPaused;
    }
    read.metadata.title = said.value(QStringLiteral("title")).toString().simplified();
    read.metadata.artist = said.value(QStringLiteral("artist")).toString().simplified();
    // Cover fetches itself: no blob:, no data:.
    const QUrl artwork(said.value(QStringLiteral("artwork")).toString());
    if (artwork.isValid() &&
        (artwork.scheme() == QLatin1String("https") || artwork.scheme() == QLatin1String("http"))) {
        read.metadata.artwork = artwork.toString();
    }
    return read;
}

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
    // Pause while still on screen: page told it's hidden may pause itself for good.
    connect(m_tabs, &TabModel::activeTabLeaving, this, [this](int tabId) {
        if (m_tabs->mediaState(tabId) == TabModel::MediaPlaying) {
            m_held.insert(tabId);
            request(tabId, Command::Pause);
        }
    });
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
    const Answer said = readAnswer(answer);
    m_tabs->setMediaState(tabId, said.state);
    m_tabs->setMediaMetadata(tabId, said.metadata);
    if (command == Command::Pause || m_tabs->mediaState(tabId) != TabModel::MediaPlaying) {
        return;
    }
    // Front tab pauses others, and any behind that starts meanwhile.
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
    // Engine suspends media behind front: front it, play there.
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
    // Not debounced: pictures needed as soon as page shown.
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
