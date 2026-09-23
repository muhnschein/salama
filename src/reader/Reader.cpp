// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "Reader.h"

#include "settings/Settings.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QtMath>

// Resources compiled into a static library have to be asked for by name, or the linker
// leaves them out of what links it; and from outside any namespace, as Qt's
// documentation of Q_INIT_RESOURCE writes it ("Using Resources in a Library").
inline void initReaderResources()
{
    Q_INIT_RESOURCE(reader);
}

namespace Salama {

namespace {

QString resource(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

bool isWebScheme(const QString &scheme)
{
    return scheme == QLatin1String("http") || scheme == QLatin1String("https");
}

// Firefox's Readerable._blockedHosts: "some high-profile pages have false positives".
const char *const BlockedHosts[] = {
    "amazon.com", "github.com",  "mail.google.com", "pinterest.com",
    "reddit.com", "twitter.com", "youtube.com",     "app.slack.com",
};

// Reading speeds from ReaderMode._getReadingSpeedForLanguage, in characters a minute
// and how far either side of that a reader may be, from the study it cites
// (http://iovs.arvojournals.org/article.aspx?articleid=2166061). English for any
// language not here.
struct ReadingSpeed
{
    const char *language;
    int cpm;
    int variance;
};

const ReadingSpeed ReadingSpeeds[] = {
    {"en", 987, 118},  {"ar", 612, 88},  {"de", 920, 86},  {"es", 1025, 127}, {"fi", 1078, 121},
    {"fr", 998, 126},  {"he", 833, 130}, {"it", 950, 140}, {"ja", 357, 56},   {"nl", 978, 143},
    {"pl", 916, 126},  {"pt", 913, 145}, {"ru", 986, 175}, {"sl", 885, 145},  {"sv", 917, 156},
    {"tr", 1054, 156}, {"zh", 255, 29},
};

const ReadingSpeed &readingSpeed(const QString &language)
{
    const QString primary = language.section(QLatin1Char('-'), 0, 0)
                                .section(QLatin1Char('_'), 0, 0)
                                .trimmed()
                                .toLower();
    for (const ReadingSpeed &speed : ReadingSpeeds) {
        if (QLatin1String(speed.language) == primary) {
            return speed;
        }
    }
    return ReadingSpeeds[0];
}

// The background each reader theme paints, from the style sheet, for the document's
// theme-color: the strip beside the display's cutout is painted in it
// (docs/DECISIONS/0013-screen-cutout.md).
QString themeBackground(const QString &scheme)
{
    if (scheme == QLatin1String("dark")) {
        return QStringLiteral("#1c1b22");
    }
    if (scheme == QLatin1String("sepia")) {
        return QStringLiteral("#f4ecd8");
    }
    return QStringLiteral("#ffffff");
}

// What the document says in an attribute, escaped for one in double quotes.
QString attribute(const QString &value)
{
    return value.toHtmlEscaped();
}

QString unescapeAttribute(QString value)
{
    value.replace(QLatin1String("&quot;"), QLatin1String("\""));
    value.replace(QLatin1String("&lt;"), QLatin1String("<"));
    value.replace(QLatin1String("&gt;"), QLatin1String(">"));
    value.replace(QLatin1String("&amp;"), QLatin1String("&"));
    return value;
}

// The head of every reader view, as far as the page it was made from: sourceUrl()
// reads it back from the address the engine reports.
const char *const HeadStart = "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
                              "<meta name=\"salama-reader\" content=\"";

// How much of a reader view's address sourceUrl() reads: the start of the document,
// percent-encoded, which is up to three times its length. Enough for any address a
// page is read from.
const int SourceWindow = 16384;

// The engine loads what it is handed as a data: url of this form: qtmozembed's
// QuickMozView::loadText, which loadHtml calls.
const char *const LoadedPrefix = "data:text/html;charset=utf-8,";

// No script of the article's runs, and nothing loads but its pictures and media: the
// reader view is a document of its own, with none of the page's code. Evaluated
// scripts stay allowed, because that is what the engine's runJavaScript is, and it is
// how the favicon, the theme colour and a change of style are asked of the view.
const char *const ContentSecurityPolicy =
    "default-src 'none'; script-src 'unsafe-eval'; style-src 'unsafe-inline'; "
    "img-src * data: blob:; media-src * data: blob:; font-src * data:";

// ReaderMode.sys.mjs's CLASSES_TO_PRESERVE, which aboutReader.css styles; and what
// about:reader's own parser utils leave out of an article on the way into its page
// (nsIParserUtils.parseFragment with SanitizerDropForms): form controls go, forms
// themselves leave their contents behind, and neither handlers nor script urls
// survive. Here the article is a document of its own, and the same goes for frames,
// which that document could not load anyway.
const char *const ArticleScript = R"JS(
var classesToPreserve = ["caption", "emoji", "hidden", "invisible", "sr-only",
    "visually-hidden", "visuallyhidden", "wp-caption", "wp-caption-text", "wp-smiley"];
function sanitize(root) {
    var drop = root.querySelectorAll("script, noscript, style, link, meta, base, template, " +
        "iframe, frame, frameset, object, embed, applet, input, button, select, " +
        "textarea, datalist, output");
    for (var i = 0; i < drop.length; ++i) {
        drop[i].remove();
    }
    var unwrap = root.querySelectorAll("form, fieldset");
    for (var j = 0; j < unwrap.length; ++j) {
        unwrap[j].replaceWith.apply(unwrap[j], Array.prototype.slice.call(unwrap[j].childNodes));
    }
    var all = root.querySelectorAll("*");
    for (var k = 0; k < all.length; ++k) {
        var attributes = Array.prototype.slice.call(all[k].attributes);
        for (var a = 0; a < attributes.length; ++a) {
            var name = attributes[a].name.toLowerCase();
            if (name.indexOf("on") === 0 || name === "srcdoc" || name === "formaction"
                    || /^\s*(javascript|vbscript):/i.test(attributes[a].value)) {
                all[k].removeAttribute(attributes[a].name);
            }
        }
    }
    return root.innerHTML;
}
var article = new Readability(document.cloneNode(true), {
    classesToPreserve: classesToPreserve,
    serializer: sanitize
}).parse();
if (!article || !article.content) {
    return "";
}
return JSON.stringify({
    title: article.title || "",
    byline: article.byline || "",
    dir: article.dir || "",
    lang: article.lang || "",
    content: article.content,
    length: article.length || 0
});
)JS";

// Readerable.isProbablyReaderable: real HTML documents only, and a node counts only if
// it takes up room on the screen (Readerable._isNodeVisible).
const char *const ReaderableScript = R"JS(
if (!(document instanceof HTMLDocument) || document.contentType === "application/pdf") {
    return false;
}
return isProbablyReaderable(document, function (node) {
    return node.clientHeight > 0 && node.clientWidth > 0;
});
)JS";

// A global `module` the page may have would otherwise be handed Readability as its
// exports, by the lines at the foot of both files.
const char *const ShadowModule = "var module;\n";

} // namespace

Reader::Reader(const Settings &settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
{
    initReaderResources();
    m_readerableScript =
        QLatin1String(ShadowModule) +
        resource(QStringLiteral(":/reader/readability/Readability-readerable.js")) +
        QLatin1String(ReaderableScript);
    m_articleScript = QLatin1String(ShadowModule) +
                      resource(QStringLiteral(":/reader/readability/Readability.js")) +
                      QLatin1String(ArticleScript);
    m_styleSheet = resource(QStringLiteral(":/reader/reader.css"));

    connect(&settings, &Settings::readerColorsChanged, this, &Reader::styleChanged);
    connect(&settings, &Settings::readerTypefaceChanged, this, &Reader::styleChanged);
    connect(&settings, &Settings::readerTextSizeChanged, this, &Reader::styleChanged);
}

QString Reader::readerableScript() const
{
    return m_readerableScript;
}

QString Reader::articleScript() const
{
    return m_articleScript;
}

bool Reader::checksUrl(const QString &url)
{
    const QUrl parsed(url, QUrl::TolerantMode);
    if (!parsed.isValid() || !isWebScheme(parsed.scheme())) {
        return false;
    }
    const QString host = parsed.host().toLower();
    const QString path = parsed.path().isEmpty() ? QStringLiteral("/") : parsed.path();
    for (const char *blocked : BlockedHosts) {
        if (host.endsWith(QLatin1String(blocked))) {
            // "Allow github on non-project pages"
            return host == QLatin1String("github.com") &&
                   !path.contains(QLatin1String("/projects")) &&
                   !path.contains(QLatin1String("/issues"));
        }
    }
    return path != QLatin1String("/");
}

bool Reader::readerable(const QVariant &answer)
{
    return answer.type() == QVariant::Bool && answer.toBool();
}

bool Reader::isDarkAmbience(const QColor &primaryColor)
{
    return primaryColor.lightnessF() > 0.5;
}

QString Reader::colorScheme(bool darkAmbience) const
{
    switch (m_settings.readerColors()) {
    case Settings::ReaderLight:
        return QStringLiteral("light");
    case Settings::ReaderSepia:
        return QStringLiteral("sepia");
    case Settings::ReaderDark:
        return QStringLiteral("dark");
    default:
        return darkAmbience ? QStringLiteral("dark") : QStringLiteral("light");
    }
}

QString Reader::bodyClass(bool darkAmbience) const
{
    const QString typeface = m_settings.readerTypeface() == Settings::ReaderSerif
                                 ? QStringLiteral("serif")
                                 : QStringLiteral("sans-serif");
    return colorScheme(darkAmbience) + QLatin1Char(' ') + typeface;
}

// AboutReader._setFontSize: 10 + 2 * the step, in css pixels.
int Reader::fontSize() const
{
    return 10 + 2 * m_settings.readerTextSize();
}

QString Reader::page(const QString &article, const QString &pageUrl, const QString &pageTitle,
                     const QString &favicon, bool darkAmbience) const
{
    const QJsonDocument json = QJsonDocument::fromJson(article.toUtf8());
    if (!json.isObject()) {
        return {};
    }
    const QJsonObject object = json.object();
    const QString content = object.value(QStringLiteral("content")).toString();
    const QUrl source(pageUrl, QUrl::TolerantMode);
    if (content.trimmed().isEmpty() || !source.isValid() || !isWebScheme(source.scheme())) {
        return {};
    }

    const QString host = displayHost(source.host());
    QString title = object.value(QStringLiteral("title")).toString().trimmed();
    if (title.isEmpty()) {
        title = host;
    }
    const QString documentTitle = pageTitle.trimmed().isEmpty() ? title : pageTitle.trimmed();
    const QString byline = object.value(QStringLiteral("byline")).toString().trimmed();
    const QString language = object.value(QStringLiteral("lang")).toString().trimmed();
    const QString direction = object.value(QStringLiteral("dir")).toString().trimmed().toLower();
    const int length = object.value(QStringLiteral("length")).toInt();

    // Only what an attribute of that name can say: a language tag, a direction.
    static const QRegularExpression languageTag(
        QStringLiteral("^[A-Za-z]{1,8}(-[A-Za-z0-9]{1,8})*$"));
    QString textAttributes;
    if (languageTag.match(language).hasMatch()) {
        textAttributes += QStringLiteral(" lang=\"%1\"").arg(language);
    }
    if (direction == QLatin1String("ltr") || direction == QLatin1String("rtl")) {
        textAttributes += QStringLiteral(" dir=\"%1\"").arg(direction);
    }

    const QUrl icon(favicon, QUrl::StrictMode);
    QString iconLink;
    if (icon.isValid() && (isWebScheme(icon.scheme()) || icon.scheme() == QLatin1String("data"))) {
        iconLink = QStringLiteral("<link rel=\"icon\" href=\"%1\">").arg(attribute(favicon));
    }

    const QString scheme = colorScheme(darkAmbience);
    QString html;
    html += QLatin1String(HeadStart) + attribute(pageUrl) + QStringLiteral("\">");
    html += QStringLiteral("<meta http-equiv=\"Content-Security-Policy\" content=\"%1\">")
                .arg(QLatin1String(ContentSecurityPolicy));
    html += QStringLiteral("<meta name=\"viewport\" content=\"width=device-width, "
                           "initial-scale=1\">");
    html +=
        QStringLiteral("<meta name=\"theme-color\" content=\"%1\">").arg(themeBackground(scheme));
    html += iconLink;
    html += QStringLiteral("<title>%1</title>").arg(documentTitle.toHtmlEscaped());
    html += QStringLiteral("<style>") + m_styleSheet + QStringLiteral("</style></head>");
    html += QStringLiteral("<body class=\"%1\" style=\"--font-size: %2px\">")
                .arg(bodyClass(darkAmbience))
                .arg(fontSize());
    html += QStringLiteral("<div class=\"container\"%1>").arg(textAttributes);
    html += QStringLiteral("<div class=\"header reader-header\"%1>").arg(textAttributes);
    html += QStringLiteral("<a class=\"domain reader-domain\" href=\"%1\">%2</a>")
                .arg(attribute(pageUrl), host.toHtmlEscaped());
    html += QStringLiteral("<h1 class=\"reader-title\">%1</h1>").arg(title.toHtmlEscaped());
    html += QStringLiteral("<div class=\"credits reader-credits\">%1</div>")
                .arg(byline.toHtmlEscaped());
    html += QStringLiteral("<div class=\"meta-data\"><div class=\"reader-estimated-time\">%1"
                           "</div></div></div><hr>")
                .arg(readingTime(length, language).toHtmlEscaped());
    // Appended rather than put in with arg(): an article is free to contain "%1".
    html += QStringLiteral("<div class=\"content\"><div class=\"moz-reader-content\">");
    html += content;
    html += QStringLiteral("</div></div></div></body></html>");
    return html;
}

QString Reader::sourceUrl(const QUrl &url)
{
    if (url.scheme() != QLatin1String("data")) {
        return {};
    }
    const QString head = QUrl::fromPercentEncoding(url.toEncoded().left(SourceWindow));
    const QString start = QLatin1String(LoadedPrefix) + QLatin1String(HeadStart);
    if (!head.startsWith(start, Qt::CaseInsensitive)) {
        return {};
    }
    const int end = head.indexOf(QLatin1Char('"'), start.length());
    if (end < 0) {
        return {};
    }
    QString source = unescapeAttribute(head.mid(start.length(), end - start.length()));
    const QUrl parsed(source, QUrl::TolerantMode);
    if (!parsed.isValid() || !isWebScheme(parsed.scheme())) {
        return {};
    }
    return source;
}

QString Reader::styleScript(bool darkAmbience) const
{
    return QStringLiteral(
               " if (!document.body || !document.querySelector('meta[name=\"salama-reader\"]'))"
               " { return ''; }"
               " document.body.className = '%1';"
               " document.body.style.setProperty('--font-size', '%2px');"
               " var color = document.querySelector('meta[name=\"theme-color\"]');"
               " if (color) { color.content = '%3'; }"
               " return '';")
        .arg(bodyClass(darkAmbience))
        .arg(fontSize())
        .arg(themeBackground(colorScheme(darkAmbience)));
}

QString Reader::readingTime(int length, const QString &language)
{
    if (length <= 0) {
        return {};
    }
    const ReadingSpeed &speed = readingSpeed(language);
    const int slow = qCeil(double(length) / (speed.cpm - speed.variance));
    const int fast = qCeil(double(length) / (speed.cpm + speed.variance));
    // In hours once the slower estimate is two of them, as Firefox does.
    if (slow >= 120) {
        const int slowHours = qRound(slow / 60.0);
        const int fastHours = qRound(fast / 60.0);
        const QString range = fastHours == slowHours
                                  ? QString::number(slowHours)
                                  : QStringLiteral("%1–%2").arg(fastHours).arg(slowHours);
        //: How long an article takes to read: a number of hours, or a range of them.
        return tr("%1 hour(s)", nullptr, slowHours).arg(range);
    }
    const QString range =
        fast == slow ? QString::number(slow) : QStringLiteral("%1–%2").arg(fast).arg(slow);
    //: How long an article takes to read: a number of minutes, or a range of them.
    return tr("%1 minute(s)", nullptr, slow).arg(range);
}

QString Reader::displayHost(const QString &host)
{
    for (const char *prefix : {"www.", "m.", "mobile."}) {
        if (host.startsWith(QLatin1String(prefix))) {
            return host.mid(int(qstrlen(prefix)));
        }
    }
    return host;
}

} // namespace Salama
