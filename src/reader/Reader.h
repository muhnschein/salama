// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include <QColor>
#include <QList>
#include <QObject>
#include <QPair>
#include <QString>
#include <QUrl>
#include <QVariant>
#include <QVariantMap>

namespace Salama {

class ReaderSettings;

// Firefox-style reader view: Readability runs in page; page() builds data: url document.
class Reader : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString readerableScript READ readerableScript CONSTANT)
    Q_PROPERTY(QString articleScript READ articleScript CONSTANT)

public:
    explicit Reader(const ReaderSettings &settings, QObject *parent = nullptr);

    QString readerableScript() const;
    // Firefox sanitizer rules: nothing runnable survives. "" if none.
    QString articleScript() const;

    // Skips front pages and known false positives.
    Q_INVOKABLE static bool checksUrl(const QString &url);

    Q_INVOKABLE static bool readerable(const QVariant &answer);

    Q_INVOKABLE static bool isDarkAmbience(const QColor &primaryColor);

    Q_INVOKABLE QString colorScheme(bool darkAmbience) const;
    Q_INVOKABLE static QString schemeFor(int colors, bool darkAmbience);

    Q_INVOKABLE static QColor backgroundOf(const QString &scheme);
    Q_INVOKABLE static QColor textColorOf(const QString &scheme);
    Q_INVOKABLE static QColor linkColorOf(const QString &scheme);

    // ambience = Silica Theme values by name (QML-only). Missing = dark ambience values.
    Q_INVOKABLE static QColor ambienceBackground(const QVariantMap &ambience);

    // CSS px.
    Q_INVOKABLE static int fontSizeFor(int step);

    // "" if no article.
    Q_INVOKABLE QString page(const QString &article, const QString &pageUrl,
                             const QString &pageTitle, const QString &favicon,
                             const QVariantMap &ambience) const;

    // Decoded from data: url, no list, so back/forward still recognised.
    Q_INVOKABLE static QString sourceUrl(const QUrl &url);

    Q_INVOKABLE QString styleScript(const QVariantMap &ambience) const;

    static QString readingTime(int length, const QString &language);

    static QString displayHost(const QString &host);

signals:
    void styleChanged();

private:
    QString bodyClass(const QVariantMap &ambience) const;
    QList<QPair<QString, QString>> bodyProperties(const QVariantMap &ambience) const;
    QString themeBackground(const QVariantMap &ambience) const;

    const ReaderSettings &m_settings;
    QString m_readerableScript;
    QString m_articleScript;
    QString m_styleSheet;
};

} // namespace Salama
