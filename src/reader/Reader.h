// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include <QColor>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QVariant>

namespace Salama {

class Settings;

// The reader view: a page's article alone, set the way Firefox's reader view sets it
// (docs/DECISIONS/0024-reader-view.md).
//
// Firefox runs Mozilla's Readability over the page, and shows what it finds in a page
// of its own, about:reader, styled by aboutReader.css. The same two halves here: the
// scripts below hand the engine Readability itself -- third_party/readability, as
// Mozilla publishes it -- to run in the page, and page() sets what it answers in a
// document of this application's making, which the view loads the way the platform
// loads any HTML it is handed (qtmozembed QuickMozView::loadHtml, a data: url). Every
// script here is the body of a function and returns, for the reason
// EngineMessages::faviconScript() gives.
class Reader : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString readerableScript READ readerableScript CONSTANT)
    Q_PROPERTY(QString articleScript READ articleScript CONSTANT)

public:
    explicit Reader(const Settings &settings, QObject *parent = nullptr);

    // Whether the page is worth offering the reader view of: Readability's
    // isProbablyReaderable() with Firefox's visibility test
    // (toolkit/components/reader/Readerable.js), answering a boolean.
    QString readerableScript() const;
    // Readability over a copy of the page, with Firefox's options
    // (toolkit/components/reader/ReaderMode.sys.mjs), answering the article as a JSON
    // string, or an empty one when there is none. Nothing that could run survives into
    // the article: what Firefox's sanitizer takes out of it on the way into
    // about:reader is taken out here.
    QString articleScript() const;

    // Whether a page at this address is looked at at all: Firefox's
    // Readerable.shouldCheckUri(). A site's front page is not an article, and a few
    // sites Firefox names read as articles and are not.
    Q_INVOKABLE static bool checksUrl(const QString &url);

    // What readerableScript's answer says. A boolean from the engine; anything else
    // says no.
    Q_INVOKABLE static bool readerable(const QVariant &answer);

    // Whether the ambience is a dark one, from its primary text colour: light text is
    // written on a dark ambience.
    Q_INVOKABLE static bool isDarkAmbience(const QColor &primaryColor);

    // The reader theme the settings ask for -- "light", "sepia" or "dark" -- with the
    // ambience's own being light or dark as the ambience is.
    Q_INVOKABLE QString colorScheme(bool darkAmbience) const;
    // The same for a colours setting given, a Settings::ReaderColors: what the reader
    // settings' preview follows, as a binding on the setting.
    Q_INVOKABLE static QString schemeFor(int colors, bool darkAmbience);

    // What a reader theme paints its page, its text and its links in, from the style
    // sheet (reader.css): the colours the reader settings' preview is drawn in.
    Q_INVOKABLE static QColor backgroundOf(const QString &scheme);
    Q_INVOKABLE static QColor textColorOf(const QString &scheme);
    Q_INVOKABLE static QColor linkColorOf(const QString &scheme);

    // The article's text size for a Settings::readerTextSize step, in css pixels:
    // AboutReader._setFontSize's 10 + 2 * the step.
    Q_INVOKABLE static int fontSizeFor(int step);

    // The reader view of an article: articleScript's answer, set in Firefox's
    // about:reader markup and style sheet as the settings ask, headed by the site it
    // came from, its title, its byline and how long it takes to read. pageUrl is the
    // page the article was read from, and the document carries it, for sourceUrl();
    // pageTitle is the document's title, so the tab keeps the page's own. Empty when
    // the answer holds no article.
    Q_INVOKABLE QString page(const QString &article, const QString &pageUrl,
                             const QString &pageTitle, const QString &favicon,
                             bool darkAmbience) const;

    // The page a reader view was made from, when this is the address of one; empty
    // for any other. The engine reports the view's address as the data: url it
    // loaded, and this reads the page back out of it rather than keeping a list, so a
    // reader view reached by going back or forward is known for what it is too.
    Q_INVOKABLE static QString sourceUrl(const QUrl &url);

    // A script that sets a reader view already on the screen as the settings now ask,
    // without loading it again: Firefox's reader view changes its own classes and
    // properties the same way.
    Q_INVOKABLE QString styleScript(bool darkAmbience) const;

    // Firefox's estimate of how long an article of this many characters takes to
    // read, in a language (ReaderMode._assignReadTime), written out: "3–4 minutes".
    // Empty for an empty article.
    static QString readingTime(int length, const QString &language);

    // A host as the reader view's header names it: Firefox's _stripHost, without the
    // www., m. or mobile. in front.
    static QString displayHost(const QString &host);

signals:
    // Something the reader view is set by has changed.
    void styleChanged();

private:
    QString bodyClass(bool darkAmbience) const;

    const Settings &m_settings;
    QString m_readerableScript;
    QString m_articleScript;
    QString m_styleSheet;
};

} // namespace Salama
