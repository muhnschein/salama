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

// When pages may sleep out of sight (engine only slows background scripts). Stays awake while
// sound plays: next track or call needs scripts. "media-decoder-info" comes from Sailfish
// gecko-dev patch 0056; owner = decoder address.
class PageActivity : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList topics READ topics CONSTANT)
    Q_PROPERTY(bool background READ background WRITE setBackground NOTIFY backgroundChanged)
    Q_PROPERTY(bool audible READ audible NOTIFY audibleChanged)
    Q_PROPERTY(bool asleep READ asleep NOTIFY asleepChanged)
    Q_PROPERTY(int settleDelay READ settleDelay CONSTANT)
    Q_PROPERTY(int soundGraceDelay READ soundGraceDelay CONSTANT)

public:
    // 1 s: dialogs/peeks hide app briefly. 5 s: next track needs buffering time on mobile.
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

    Q_INVOKABLE void observe(const QString &topic, const QVariant &data);

signals:
    void backgroundChanged();
    void audibleChanged();
    void asleepChanged();
    // Engine doesn't say which page.
    void playStateChanged();

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
    QHash<QString, bool> m_hasSound;
    QSet<QString> m_playing;
};

} // namespace Salama
