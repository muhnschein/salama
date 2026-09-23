// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "PageActivity.h"

namespace Salama {

namespace {

const QString DecoderTopic = QStringLiteral("media-decoder-info");
const QString CallTopic = QStringLiteral("webrtc-media-info");

// Decoders remembered beyond those playing. A page that plays one clip after another
// makes a decoder for each, and each is an address that is never heard of again.
const int RememberedDecoders = 256;

} // namespace

PageActivity::PageActivity(QObject *parent)
    : PageActivity(DefaultSettleDelay, DefaultSoundGraceDelay, parent)
{
}

PageActivity::PageActivity(int settleDelay, int soundGraceDelay, QObject *parent)
    : QObject(parent)
    , m_settleDelay(settleDelay)
    , m_soundGraceDelay(soundGraceDelay)
{
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, [this]() {
        if (m_background && !m_audible) {
            setAsleep(true);
        }
    });
}

QStringList PageActivity::topics() const
{
    return {DecoderTopic, CallTopic};
}

bool PageActivity::background() const
{
    return m_background;
}

void PageActivity::setBackground(bool background)
{
    if (m_background == background) {
        return;
    }
    m_background = background;
    if (background) {
        if (!m_audible) {
            fallAsleepAfter(m_settleDelay);
        }
    } else {
        m_timer.stop();
        setAsleep(false);
    }
    emit backgroundChanged();
}

bool PageActivity::audible() const
{
    return m_audible;
}

bool PageActivity::asleep() const
{
    return m_asleep;
}

int PageActivity::settleDelay() const
{
    return m_settleDelay;
}

int PageActivity::soundGraceDelay() const
{
    return m_soundGraceDelay;
}

void PageActivity::observe(const QString &topic, const QVariant &data)
{
    const QVariantMap info = data.toMap();
    if (topic == DecoderTopic) {
        observeDecoder(info);
    } else if (topic == CallTopic) {
        m_inCall = info.value(QStringLiteral("audio")).toBool() ||
                   info.value(QStringLiteral("video")).toBool();
    } else {
        return;
    }
    updateAudible();
}

void PageActivity::observeDecoder(const QVariantMap &info)
{
    const QString owner = info.value(QStringLiteral("owner")).toString();
    const QString state = info.value(QStringLiteral("state")).toString();
    if (owner.isEmpty()) {
        return;
    }
    if (state == QLatin1String("meta")) {
        if (m_hasSound.size() >= RememberedDecoders) {
            forgetIdleDecoders();
        }
        m_hasSound.insert(owner, info.value(QStringLiteral("a")).toBool());
    } else if (state == QLatin1String("play")) {
        m_playing.insert(owner);
        emit playStateChanged();
    } else {
        // "pause" is every state but playing, the decoder's shutdown included.
        m_playing.remove(owner);
        emit playStateChanged();
    }
}

void PageActivity::forgetIdleDecoders()
{
    for (auto it = m_hasSound.begin(); it != m_hasSound.end();) {
        if (m_playing.contains(it.key())) {
            ++it;
        } else {
            it = m_hasSound.erase(it);
        }
    }
}

void PageActivity::updateAudible()
{
    // A decoder whose metadata was never seen counts as sound: the engine sends it
    // before playing starts, so this is a decoder forgotten above or an engine that
    // changed its order, and a silent video kept running costs less than music cut off.
    bool audible = m_inCall;
    for (auto it = m_playing.constBegin(); it != m_playing.constEnd(); ++it) {
        audible = audible || m_hasSound.value(*it, true);
    }
    if (m_audible == audible) {
        return;
    }
    m_audible = audible;
    if (audible) {
        m_timer.stop();
    } else if (m_background && !m_asleep) {
        fallAsleepAfter(m_soundGraceDelay);
    }
    emit audibleChanged();
}

void PageActivity::setAsleep(bool asleep)
{
    if (m_asleep != asleep) {
        m_asleep = asleep;
        emit asleepChanged();
    }
}

void PageActivity::fallAsleepAfter(int delay)
{
    m_timer.start(delay);
}

} // namespace Salama
