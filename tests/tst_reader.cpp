// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "reader/Reader.h"
#include "settings/ReaderSettings.h"

#include <QColor>
#include <QFile>
#include <QJSEngine>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

using Salama::Reader;
using Salama::ReaderSettings;

namespace {

QString thirdParty(const QString &name)
{
    QFile file(QStringLiteral(SALAMA_SOURCE_DIR "/third_party/readability/") + name);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

QString article(const QJsonObject &fields)
{
    QJsonObject object{
        {QStringLiteral("title"), QStringLiteral("An article")},
        {QStringLiteral("byline"), QStringLiteral("A. Writer")},
        {QStringLiteral("dir"), QString()},
        {QStringLiteral("lang"), QStringLiteral("en")},
        {QStringLiteral("content"), QStringLiteral("<p>Words.</p>")},
        {QStringLiteral("length"), 5000},
    };
    for (auto it = fields.begin(); it != fields.end(); ++it) {
        object.insert(it.key(), it.value());
    }
    return QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact));
}

// Url engine reports for HTML-loaded doc (qtmozembed QuickMozView::loadText).
QUrl loaded(const QString &html)
{
    return QUrl(QStringLiteral("data:text/html;charset=utf-8,") +
                QString::fromLatin1(QUrl::toPercentEncoding(html)));
}

QVariantMap darkAmbience()
{
    return {
        {QStringLiteral("primaryColor"), QColor(Qt::white)},
        {QStringLiteral("secondaryColor"), QColor(255, 255, 255, 176)},
        {QStringLiteral("highlightColor"), QColor(QStringLiteral("#ffc480"))},
        {QStringLiteral("secondaryHighlightColor"), QColor(255, 196, 128, 176)},
        {QStringLiteral("highlightBackgroundColor"), QColor(QStringLiteral("#e8872e"))},
        {QStringLiteral("highlightDimmerColor"), QColor(QStringLiteral("#4a2408"))},
        {QStringLiteral("overlayBackgroundColor"), QColor(Qt::black)},
        {QStringLiteral("fontFamily"), QStringLiteral("Sail Sans Pro")},
        {QStringLiteral("fontFamilyHeading"), QStringLiteral("Sail Sans Pro Light")},
    };
}

QVariantMap lightAmbience()
{
    QVariantMap ambience = darkAmbience();
    ambience.insert(QStringLiteral("primaryColor"), QColor(Qt::black));
    ambience.insert(QStringLiteral("highlightColor"), QColor(QStringLiteral("#7a3c00")));
    ambience.insert(QStringLiteral("highlightDimmerColor"), QColor(QStringLiteral("#fbe5c9")));
    ambience.insert(QStringLiteral("overlayBackgroundColor"), QColor(Qt::white));
    return ambience;
}

} // namespace

class tst_reader : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void scriptsAreMozillasReadability();
    void scriptsParse();
    void checksUrl_data();
    void checksUrl();
    void readerableAnswer();
    void colors();
    void ambienceLook();
    void refusesWhatIsNoArticle();
    void setsTheArticle();
    void keepsWhatThePageSaysOutOfTheMarkup();
    void sourceUrlRoundTrip();
    void sourceUrlOfOtherAddresses();
    void styleFollowsSettings();
    void readingTime_data();
    void readingTime();
    void displayHost();

private:
    QScopedPointer<QTemporaryDir> m_dir;
    QScopedPointer<QSettings> m_file;
    QScopedPointer<ReaderSettings> m_settings;
    QScopedPointer<Reader> m_reader;
};

void tst_reader::init()
{
    m_dir.reset(new QTemporaryDir);
    m_file.reset(
        new QSettings(m_dir->path() + QStringLiteral("/salama.conf"), QSettings::IniFormat));
    m_settings.reset(new ReaderSettings(*m_file));
    m_reader.reset(new Reader(*m_settings));
}

void tst_reader::cleanup()
{
    m_reader.reset();
    m_settings.reset();
    m_file.reset();
    m_dir.reset();
}

// Page runs Mozilla's Readability whole: third_party/ files = what scripts carry.
void tst_reader::scriptsAreMozillasReadability()
{
    const QString readability = thirdParty(QStringLiteral("Readability.js"));
    const QString readerable = thirdParty(QStringLiteral("Readability-readerable.js"));
    QVERIFY(readability.contains(QLatin1String("function Readability(doc, options)")));
    QVERIFY(readerable.contains(QLatin1String("function isProbablyReaderable(doc, options")));

    QVERIFY(m_reader->articleScript().contains(readability));
    QVERIFY(m_reader->readerableScript().contains(readerable));
    // Page's own global `module` must not get Readability exports.
    QVERIFY(m_reader->articleScript().startsWith(QLatin1String("var module;\n")));
    QVERIFY(m_reader->readerableScript().startsWith(QLatin1String("var module;\n")));
    QVERIFY(m_reader->articleScript().contains(QLatin1String("return JSON.stringify(")));
    QVERIFY(m_reader->readerableScript().contains(QLatin1String("return isProbablyReaderable(")));
    QVERIFY(m_reader->articleScript().contains(QLatin1String("\"wp-caption-text\"")));
}

// Our wrapper compiles as runJavaScript function body. Readability itself uses Gecko-only
// syntax host QJSEngine lacks (optional catch binding, `??`), so stripped first (Mozilla
// tests it). Rest compiled with stand-ins for callees.
void tst_reader::scriptsParse()
{
    const QString readability = thirdParty(QStringLiteral("Readability.js"));
    const QString readerable = thirdParty(QStringLiteral("Readability-readerable.js"));
    QString article = m_reader->articleScript();
    QString check = m_reader->readerableScript();
    QCOMPARE(article.count(readability), 1);
    QCOMPARE(check.count(readerable), 1);
    article.replace(readability, QStringLiteral("function Readability() {}\n"));
    check.replace(readerable, QStringLiteral("function isProbablyReaderable() {}\n"));

    QJSEngine engine;
    for (const QString &script : {article, check, m_reader->styleScript(darkAmbience())}) {
        const QJSValue function =
            engine.evaluate(QStringLiteral("(function () {\n") + script + QStringLiteral("\n})"));
        QVERIFY2(!function.isError(), qPrintable(function.toString() + QLatin1Char('\n') + script));
        QVERIFY(function.isCallable());
    }
}

void tst_reader::checksUrl_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<bool>("checked");

    QTest::newRow("article") << "https://example.com/2026/09/an-article" << true;
    QTest::newRow("http") << "http://example.com/story.html" << true;
    QTest::newRow("front page") << "https://example.com/" << false;
    QTest::newRow("front page without a slash") << "https://example.com" << false;
    QTest::newRow("front page with a query") << "https://example.com/?page=2" << false;
    QTest::newRow("about") << "about:blank" << false;
    QTest::newRow("data") << "data:text/html;charset=utf-8,%3Cp%3E" << false;
    QTest::newRow("file") << "file:///home/defaultuser/page.html" << false;
    QTest::newRow("not a url") << "" << false;
    QTest::newRow("amazon") << "https://www.amazon.com/dp/B000" << false;
    QTest::newRow("youtube") << "https://m.youtube.com/watch?v=x" << false;
    QTest::newRow("reddit") << "https://old.reddit.com/r/sailfishos/" << false;
    QTest::newRow("github") << "https://github.com/mozilla/readability" << true;
    QTest::newRow("github issues") << "https://github.com/mozilla/readability/issues/1" << false;
    QTest::newRow("github projects") << "https://github.com/orgs/x/projects/1" << false;
    QTest::newRow("gist") << "https://gist.github.com/x/1" << false;
}

void tst_reader::checksUrl()
{
    QFETCH(QString, url);
    QFETCH(bool, checked);
    QCOMPARE(Reader::checksUrl(url), checked);
}

void tst_reader::readerableAnswer()
{
    QVERIFY(Reader::readerable(QVariant(true)));
    QVERIFY(!Reader::readerable(QVariant(false)));
    QVERIFY(!Reader::readerable(QVariant(QStringLiteral("true"))));
    QVERIFY(!Reader::readerable(QVariant(1)));
    QVERIFY(!Reader::readerable(QVariant()));
}

void tst_reader::colors()
{
    QVERIFY(Reader::isDarkAmbience(QColor(Qt::white)));
    QVERIFY(Reader::isDarkAmbience(QColor(QStringLiteral("#e0e0e0"))));
    QVERIFY(!Reader::isDarkAmbience(QColor(Qt::black)));
    QVERIFY(!Reader::isDarkAmbience(QColor(QStringLiteral("#202020"))));

    QCOMPARE(m_reader->colorScheme(true), QStringLiteral("ambience"));
    QCOMPARE(m_reader->colorScheme(false), QStringLiteral("ambience"));
    m_settings->setColors(ReaderSettings::Automatic);
    QCOMPARE(m_reader->colorScheme(true), QStringLiteral("dark"));
    QCOMPARE(m_reader->colorScheme(false), QStringLiteral("light"));
    m_settings->setColors(ReaderSettings::Sepia);
    QCOMPARE(m_reader->colorScheme(true), QStringLiteral("sepia"));
    QCOMPARE(m_reader->colorScheme(false), QStringLiteral("sepia"));
    m_settings->setColors(ReaderSettings::Light);
    QCOMPARE(m_reader->colorScheme(true), QStringLiteral("light"));
    m_settings->setColors(ReaderSettings::Dark);
    QCOMPARE(m_reader->colorScheme(false), QStringLiteral("dark"));

    QCOMPARE(Reader::schemeFor(ReaderSettings::Automatic, true), QStringLiteral("dark"));
    QCOMPARE(Reader::schemeFor(ReaderSettings::Automatic, false), QStringLiteral("light"));
    QCOMPARE(Reader::schemeFor(ReaderSettings::Ambience, false), QStringLiteral("ambience"));
    QCOMPARE(Reader::schemeFor(ReaderSettings::Sepia, true), QStringLiteral("sepia"));
    QCOMPARE(Reader::schemeFor(99, false), QStringLiteral("light"));

    QCOMPARE(Reader::backgroundOf(QStringLiteral("light")), QColor(QStringLiteral("#ffffff")));
    QCOMPARE(Reader::backgroundOf(QStringLiteral("sepia")), QColor(244, 236, 216));
    QCOMPARE(Reader::backgroundOf(QStringLiteral("dark")), QColor(28, 27, 34));
    QCOMPARE(Reader::textColorOf(QStringLiteral("light")), QColor(21, 20, 26));
    QCOMPARE(Reader::textColorOf(QStringLiteral("sepia")), QColor(91, 70, 54));
    QCOMPARE(Reader::textColorOf(QStringLiteral("dark")), QColor(251, 251, 254));
    QCOMPARE(Reader::linkColorOf(QStringLiteral("light")), QColor(0, 97, 224));
    QCOMPARE(Reader::linkColorOf(QStringLiteral("sepia")), QColor(0, 97, 224));
    QCOMPARE(Reader::linkColorOf(QStringLiteral("dark")), QColor(0, 221, 255));
    QFile sheet(QStringLiteral(SALAMA_SOURCE_DIR "/src/reader/reader.css"));
    QVERIFY(sheet.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString css = QString::fromUtf8(sheet.readAll());
    QVERIFY(css.contains(QStringLiteral("--dark-theme-background: rgb(28, 27, 34);")));
    QVERIFY(css.contains(QStringLiteral("--main-foreground: rgb(91, 70, 54);")));
    QVERIFY(css.contains(QStringLiteral("--primary-color: rgb(0, 221, 255);")));

    // Firefox sizes: 10 + 2 per step.
    QCOMPARE(Reader::fontSizeFor(ReaderSettings::TextSizeMin), 12);
    QCOMPARE(Reader::fontSizeFor(ReaderSettings::TextSizeDefault), 20);
    QCOMPARE(Reader::fontSizeFor(ReaderSettings::TextSizeMax), 28);
}

void tst_reader::ambienceLook()
{
    const QString url = QStringLiteral("https://example.com/story");
    const QString dark = m_reader->page(article({}), url, QString(), QString(), darkAmbience());
    QVERIFY(dark.contains(QLatin1String("<body class=\"ambience ambience-dark sans-serif\" "
                                        "style=\"--font-size: 20px; ")));
    QVERIFY(dark.contains(QLatin1String("--ambience-primary: rgba(255, 255, 255, 1);")));
    QVERIFY(dark.contains(QLatin1String("--ambience-secondary: rgba(255, 255, 255, 0.69);")));
    QVERIFY(dark.contains(QLatin1String("--ambience-highlight: rgba(255, 196, 128, 1);")));
    QVERIFY(
        dark.contains(QLatin1String("--ambience-secondary-highlight: rgba(255, 196, 128, 0.69);")));
    QVERIFY(dark.contains(QLatin1String("--ambience-selection: rgba(232, 135, 46, 0.3);")));
    QVERIFY(dark.contains(QLatin1String("--ambience-top: rgba(74, 36, 8, 1);")));
    QVERIFY(dark.contains(QLatin1String("--ambience-bottom: rgba(37, 18, 4, 1);")));
    QVERIFY(dark.contains(QLatin1String("--ambience-font: &#39;Sail Sans Pro&#39;;")) ||
            dark.contains(QLatin1String("--ambience-font: 'Sail Sans Pro';")));
    QVERIFY(dark.contains(QLatin1String("Sail Sans Pro Light")));
    QVERIFY(dark.contains(QLatin1String("<meta name=\"theme-color\" content=\"#4a2408\">")));
    QVERIFY(dark.contains(QLatin1String("body.ambience {")));
    QVERIFY(dark.contains(QLatin1String("color: var(--ambience-highlight);")));

    // Halfway dimmer highlight -> overlay: darker on dark, lighter on light.
    QCOMPARE(Reader::ambienceBackground(darkAmbience()), QColor(37, 18, 4));
    QCOMPARE(Reader::ambienceBackground(lightAmbience()), QColor(253, 242, 228));
    const QString light = m_reader->page(article({}), url, QString(), QString(), lightAmbience());
    QVERIFY(light.contains(QLatin1String("<body class=\"ambience ambience-light sans-serif\"")));
    QVERIFY(light.contains(QLatin1String("--ambience-primary: rgba(0, 0, 0, 1);")));

    const QString bare = m_reader->page(article({}), url, QString(), QString(), QVariantMap());
    QVERIFY(bare.contains(QLatin1String("<body class=\"ambience ambience-dark sans-serif\"")));
    QVERIFY(bare.contains(QLatin1String("--ambience-font: sans-serif;")));
    QVariantMap hostile = darkAmbience();
    hostile.insert(QStringLiteral("fontFamily"), QStringLiteral("Evil'; } body { x: \"<y>"));
    const QString script = m_reader->styleScript(hostile);
    QVERIFY(
        script.contains(QLatin1String("setProperty('--ambience-font', '\\'Evil  body  x y\\'');")));
    QJSEngine engine;
    const QJSValue function =
        engine.evaluate(QStringLiteral("(function () {\n") + script + QStringLiteral("\n})"));
    QVERIFY2(!function.isError(), qPrintable(function.toString()));
    const QString page = m_reader->page(article({}), url, QString(), QString(), hostile);
    QVERIFY(!page.contains(QLatin1String("<y>")));
    QVERIFY(!page.contains(QLatin1String("Evil'")));
}

void tst_reader::refusesWhatIsNoArticle()
{
    const QString url = QStringLiteral("https://example.com/story");
    QVERIFY(m_reader->page(QString(), url, QString(), QString(), lightAmbience()).isEmpty());
    QVERIFY(m_reader->page(QStringLiteral("not json"), url, QString(), QString(), lightAmbience())
                .isEmpty());
    QVERIFY(
        m_reader->page(QStringLiteral("[]"), url, QString(), QString(), lightAmbience()).isEmpty());
    QVERIFY(
        m_reader->page(QStringLiteral("{}"), url, QString(), QString(), lightAmbience()).isEmpty());
    QVERIFY(m_reader
                ->page(article({{QStringLiteral("content"), QStringLiteral("  ")}}), url, QString(),
                       QString(), lightAmbience())
                .isEmpty());
    QVERIFY(m_reader
                ->page(article({}), QStringLiteral("about:blank"), QString(), QString(),
                       lightAmbience())
                .isEmpty());
    QVERIFY(m_reader
                ->page(article({}), QStringLiteral("javascript:alert(1)"), QString(), QString(),
                       lightAmbience())
                .isEmpty());
    QVERIFY(!m_reader->page(article({}), url, QString(), QString(), lightAmbience()).isEmpty());
}

void tst_reader::setsTheArticle()
{
    m_settings->setColors(ReaderSettings::Automatic);
    const QString html =
        m_reader->page(article({{QStringLiteral("lang"), QStringLiteral("en-GB")},
                                {QStringLiteral("dir"), QStringLiteral("RTL")},
                                {QStringLiteral("content"),
                                 QStringLiteral("<p>100%1 <a href=\"#note\">here</a></p>")}}),
                       QStringLiteral("https://www.example.com/story?id=1&page=2"),
                       QStringLiteral("An article | Example"),
                       QStringLiteral("https://www.example.com/icon.png"), lightAmbience());

    QVERIFY(html.startsWith(QLatin1String("<!DOCTYPE html><html><head><meta charset=\"utf-8\">")));
    QVERIFY(html.contains(
        QLatin1String("<a class=\"domain reader-domain\" "
                      "href=\"https://www.example.com/story?id=1&amp;page=2\">example.com</a>")));
    QVERIFY(html.contains(QLatin1String("<h1 class=\"reader-title\">An article</h1>")));
    QVERIFY(html.contains(QLatin1String("<div class=\"credits reader-credits\">A. Writer</div>")));
    QVERIFY(
        html.contains(QStringLiteral("<div class=\"reader-estimated-time\">5–6 minute(s)</div>")));
    QVERIFY(html.contains(QLatin1String(
        "<div class=\"moz-reader-content\"><p>100%1 <a href=\"#note\">here</a></p></div>")));
    QVERIFY(html.contains(QLatin1String("<div class=\"container\" lang=\"en-GB\" dir=\"rtl\">")));
    QVERIFY(html.contains(QLatin1String("<title>An article | Example</title>")));
    QVERIFY(html.contains(
        QLatin1String("<link rel=\"icon\" href=\"https://www.example.com/icon.png\">")));
    QVERIFY(html.contains(QLatin1String(".moz-reader-content blockquote {")));
    QVERIFY(html.contains(
        QLatin1String("<body class=\"light sans-serif\" style=\"--font-size: 20px; ")));
    QVERIFY(html.contains(QLatin1String("<meta name=\"theme-color\" content=\"#ffffff\">")));
    QVERIFY(
        html.contains(QLatin1String("<meta http-equiv=\"Content-Security-Policy\" "
                                    "content=\"default-src 'none'; script-src 'unsafe-eval';")));
    QVERIFY(html.contains(QLatin1String("<meta name=\"viewport\" content=\"width=device-width")));

    const QString untitled = m_reader->page(article({{QStringLiteral("title"), QString()}}),
                                            QStringLiteral("https://m.example.org/a"), QString(),
                                            QStringLiteral("javascript:alert(1)"), darkAmbience());
    QVERIFY(untitled.contains(QLatin1String("<title>example.org</title>")));
    QVERIFY(untitled.contains(QLatin1String("<h1 class=\"reader-title\">example.org</h1>")));
    QVERIFY(!untitled.contains(QLatin1String("rel=\"icon\"")));
    QVERIFY(untitled.contains(QLatin1String("<body class=\"dark sans-serif\"")));
    QVERIFY(untitled.contains(QLatin1String("<meta name=\"theme-color\" content=\"#1c1b22\">")));
}

void tst_reader::keepsWhatThePageSaysOutOfTheMarkup()
{
    const QString html = m_reader->page(
        article({{QStringLiteral("title"), QStringLiteral("<script>t()</script> & \"more\"")},
                 {QStringLiteral("byline"), QStringLiteral("<img src=x onerror=b()>")},
                 {QStringLiteral("lang"), QStringLiteral("en\" onclick=\"x()")},
                 {QStringLiteral("dir"), QStringLiteral("ltr\" onclick=\"x()")}}),
        QStringLiteral("https://example.com/a\"b"), QStringLiteral("</title><script>p()</script>"),
        QString(), lightAmbience());
    QVERIFY(
        html.contains(QLatin1String("<h1 class=\"reader-title\">&lt;script&gt;t()&lt;/script&gt; "
                                    "&amp; &quot;more&quot;</h1>")));
    QVERIFY(html.contains(QLatin1String("&lt;img src=x onerror=b()&gt;")));
    QVERIFY(html.contains(
        QLatin1String("<title>&lt;/title&gt;&lt;script&gt;p()&lt;/script&gt;</title>")));
    QVERIFY(html.contains(QLatin1String("<div class=\"container\">")));
    QVERIFY(!html.contains(QLatin1String("onclick")));
    QVERIFY(!html.contains(QLatin1String("<script>")));
}

void tst_reader::sourceUrlRoundTrip()
{
    const QStringList pages{
        QStringLiteral("https://example.com/story"),
        QStringLiteral("http://example.com/a?b=1&c=\"2\"#part"),
        QStringLiteral("https://esimerkki.fi/p%C3%A4iv%C3%A4n-uutiset"),
        QStringLiteral("https://esimerkki.fi/päivän-uutiset"),
    };
    for (const QString &page : pages) {
        const QString html =
            m_reader->page(article({}), page, QStringLiteral("Title"), QString(), lightAmbience());
        QVERIFY(!html.isEmpty());
        const QUrl url = loaded(html);
        QCOMPARE(Reader::sourceUrl(url), page);
        QCOMPARE(Reader::sourceUrl(QUrl(url.toString())), page);
        QCOMPARE(Reader::sourceUrl(QUrl(url.toString() + QStringLiteral("#note"))), page);
        QCOMPARE(Reader::sourceUrl(QUrl::fromEncoded(url.toEncoded())), page);
    }
}

void tst_reader::sourceUrlOfOtherAddresses()
{
    QVERIFY(Reader::sourceUrl(QUrl(QStringLiteral("https://example.com/story"))).isEmpty());
    QVERIFY(Reader::sourceUrl(QUrl()).isEmpty());
    QVERIFY(Reader::sourceUrl(loaded(QStringLiteral("<p>A page</p>"))).isEmpty());
    QVERIFY(Reader::sourceUrl(QUrl(QStringLiteral("data:text/plain,hello"))).isEmpty());
    QVERIFY(Reader::sourceUrl(loaded(QStringLiteral("<!DOCTYPE html><html><head><meta "
                                                    "charset=\"utf-8\"><meta "
                                                    "name=\"salama-reader\" "
                                                    "content=\"javascript:x()\">")))
                .isEmpty());
    QVERIFY(Reader::sourceUrl(loaded(QStringLiteral("<!DOCTYPE html><html><head><meta "
                                                    "charset=\"utf-8\"><meta "
                                                    "name=\"salama-reader\" content=\"https:")))
                .isEmpty());
}

void tst_reader::styleFollowsSettings()
{
    m_settings->setColors(ReaderSettings::Automatic);
    QSignalSpy changed(m_reader.data(), &Reader::styleChanged);
    QString script = m_reader->styleScript(lightAmbience());
    QVERIFY(script.contains(QLatin1String("document.body.className = 'light sans-serif';")));
    QVERIFY(script.contains(QLatin1String("setProperty('--font-size', '20px')")));
    QVERIFY(script.contains(QLatin1String("color.content = '#ffffff'")));
    QVERIFY(script.contains(QLatin1String("meta[name=\"salama-reader\"]")));

    m_settings->setColors(ReaderSettings::Sepia);
    m_settings->setTypeface(ReaderSettings::Serif);
    m_settings->setTextSize(ReaderSettings::TextSizeMax);
    QCOMPARE(changed.count(), 3);
    script = m_reader->styleScript(darkAmbience());
    QVERIFY(script.contains(QLatin1String("document.body.className = 'sepia serif';")));
    QVERIFY(script.contains(QLatin1String("setProperty('--font-size', '28px')")));
    QVERIFY(script.contains(QLatin1String("color.content = '#f4ecd8'")));

    const QString html = m_reader->page(article({}), QStringLiteral("https://example.com/a"),
                                        QString(), QString(), darkAmbience());
    QVERIFY(
        html.contains(QLatin1String("<body class=\"sepia serif\" style=\"--font-size: 28px; ")));
    m_settings->setTextSize(ReaderSettings::TextSizeMin);
    QVERIFY(m_reader->styleScript(darkAmbience()).contains(QLatin1String("'12px'")));
    m_settings->setColors(ReaderSettings::Ambience);
    QCOMPARE(changed.count(), 5);
    script = m_reader->styleScript(darkAmbience());
    QVERIFY(script.contains(
        QLatin1String("document.body.className = 'ambience ambience-dark serif';")));
    QVERIFY(script.contains(
        QLatin1String("setProperty('--ambience-highlight', 'rgba(255, 196, 128, 1)')")));
    QVERIFY(script.contains(QLatin1String("color.content = '#4a2408'")));
}

void tst_reader::readingTime_data()
{
    QTest::addColumn<int>("length");
    QTest::addColumn<QString>("language");
    QTest::addColumn<QString>("expected");

    QTest::newRow("empty") << 0 << "en" << QString();
    // English, 987 ± 118 chars/min.
    QTest::newRow("a minute") << 869 << "en" << "1 minute(s)";
    QTest::newRow("a range") << 5000 << "en" << "5–6 minute(s)";
    QTest::newRow("a region's variant") << 5000 << "en-US" << "5–6 minute(s)";
    QTest::newRow("unknown language") << 5000 << "xx" << "5–6 minute(s)";
    QTest::newRow("no language") << 5000 << "" << "5–6 minute(s)";
    // Finnish 1078 ± 121; Chinese 255 ± 29.
    QTest::newRow("finnish") << 12000 << "fi" << "11–13 minute(s)";
    QTest::newRow("chinese") << 5000 << "zh_CN" << "18–23 minute(s)";
    QTest::newRow("hours") << 130350 << "en" << "2–3 hour(s)";
    QTest::newRow("hours alike") << 173800 << "en" << "3 hour(s)";
}

void tst_reader::readingTime()
{
    QFETCH(int, length);
    QFETCH(QString, language);
    QFETCH(QString, expected);
    QCOMPARE(Reader::readingTime(length, language), expected);
}

void tst_reader::displayHost()
{
    QCOMPARE(Reader::displayHost(QStringLiteral("www.example.com")), QStringLiteral("example.com"));
    QCOMPARE(Reader::displayHost(QStringLiteral("m.example.com")), QStringLiteral("example.com"));
    QCOMPARE(Reader::displayHost(QStringLiteral("mobile.example.com")),
             QStringLiteral("example.com"));
    QCOMPARE(Reader::displayHost(QStringLiteral("news.example.com")),
             QStringLiteral("news.example.com"));
    QCOMPARE(Reader::displayHost(QString()), QString());
}

QTEST_GUILESS_MAIN(tst_reader)
#include "tst_reader.moc"
