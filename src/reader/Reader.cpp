// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "Reader.h"

#include "settings/ReaderSettings.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QtMath>
#include <array>

// Static-lib resources must be inited by name or linker drops them; outside any namespace per
// Qt docs.
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
const std::array<const char *, 8> BlockedHosts{{
    "amazon.com",
    "github.com",
    "mail.google.com",
    "pinterest.com",
    "reddit.com",
    "twitter.com",
    "youtube.com",
    "app.slack.com",
}};

// Firefox reading speeds (chars/min, variance) from
// http://iovs.arvojournals.org/article.aspx?articleid=2166061. Unknown language = English.
struct ReadingSpeed
{
    const char *language;
    int cpm;
    int variance;
};

const std::array<ReadingSpeed, 17> ReadingSpeeds{{
    {"en", 987, 118},
    {"ar", 612, 88},
    {"de", 920, 86},
    {"es", 1025, 127},
    {"fi", 1078, 121},
    {"fr", 998, 126},
    {"he", 833, 130},
    {"it", 950, 140},
    {"ja", 357, 56},
    {"nl", 978, 143},
    {"pl", 916, 126},
    {"pt", 913, 145},
    {"ru", 986, 175},
    {"sl", 885, 145},
    {"sv", 917, 156},
    {"tr", 1054, 156},
    {"zh", 255, 29},
}};

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
    return ReadingSpeeds.front();
}

QColor themeColor(const QVariantMap &ambience, const char *name, const QColor &otherwise)
{
    const QColor color = ambience.value(QLatin1String(name)).value<QColor>();
    return color.isValid() ? color : otherwise;
}

// Keeps alpha: Silica secondary colours are faded primaries.
QString cssColor(const QColor &color)
{
    return QStringLiteral("rgba(%1, %2, %3, %4)")
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue())
        .arg(QString::number(color.alphaF(), 'g', 3));
}

// Only letters/digits/spaces/hyphens: name can't break out of quotes or attribute.
QString cssFamily(const QVariantMap &ambience, const char *name)
{
    QString family = ambience.value(QLatin1String(name)).toString();
    family.remove(QRegularExpression(QStringLiteral("[^A-Za-z0-9 _-]")));
    family = family.trimmed();
    return family.isEmpty() ? QStringLiteral("sans-serif")
                            : QLatin1Char('\'') + family + QLatin1Char('\'');
}

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

// sourceUrl() parses source back from engine-reported address.
const char *const HeadStart = R"(<!DOCTYPE html><html><head><meta charset="utf-8">)"
                              R"(<meta name="salama-reader" content=")";

// Doc start percent-encoded, up to 3x length.
const int SourceWindow = 16384;

// qtmozembed QuickMozView::loadText form.
const char *const LoadedPrefix = "data:text/html;charset=utf-8,";

// No article script runs. 'unsafe-eval' kept: runJavaScript is eval.
const char *const ContentSecurityPolicy =
    "default-src 'none'; script-src 'unsafe-eval'; style-src 'unsafe-inline'; "
    "img-src * data: blob:; media-src * data: blob:; font-src * data:";

// Sanitize like about:reader (SanitizerDropForms): drop controls, unwrap forms, strip
// handlers/script urls, plus frames.
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

// Node counts only if visible (Readerable._isNodeVisible).
const char *const ReaderableScript = R"JS(
if (!(document instanceof HTMLDocument) || document.contentType === "application/pdf") {
    return false;
}
return isProbablyReaderable(document, function (node) {
    return node.clientHeight > 0 && node.clientWidth > 0;
});
)JS";

// Shadow page's global `module`, else Readability exports into it.
const char *const ShadowModule = "var module;\n";

} // namespace

Reader::Reader(const ReaderSettings &settings, QObject *parent)
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

    connect(&settings, &ReaderSettings::colorsChanged, this, &Reader::styleChanged);
    connect(&settings, &ReaderSettings::typefaceChanged, this, &Reader::styleChanged);
    connect(&settings, &ReaderSettings::textSizeChanged, this, &Reader::styleChanged);
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
    return schemeFor(m_settings.colors(), darkAmbience);
}

QString Reader::schemeFor(int colors, bool darkAmbience)
{
    switch (colors) {
    case ReaderSettings::Light:
        return QStringLiteral("light");
    case ReaderSettings::Sepia:
        return QStringLiteral("sepia");
    case ReaderSettings::Dark:
        return QStringLiteral("dark");
    case ReaderSettings::Ambience:
        return QStringLiteral("ambience");
    default:
        return darkAmbience ? QStringLiteral("dark") : QStringLiteral("light");
    }
}

QString Reader::bodyClass(const QVariantMap &ambience) const
{
    const bool dark = isDarkAmbience(themeColor(ambience, "primaryColor", Qt::white));
    const QString scheme = colorScheme(dark);
    QString classes = scheme;
    if (scheme == QLatin1String("ambience")) {
        classes += dark ? QStringLiteral(" ambience-dark") : QStringLiteral(" ambience-light");
    }
    const QString typeface = m_settings.typeface() == ReaderSettings::Serif
                                 ? QStringLiteral("serif")
                                 : QStringLiteral("sans-serif");
    return classes + QLatin1Char(' ') + typeface;
}

QList<QPair<QString, QString>> Reader::bodyProperties(const QVariantMap &ambience) const
{
    const QColor highlightBackground =
        themeColor(ambience, "highlightBackgroundColor", QColor(QStringLiteral("#e8872e")));
    QColor selection = highlightBackground;
    selection.setAlphaF(0.3);
    return {
        {QStringLiteral("--font-size"),
         QString::number(fontSizeFor(m_settings.textSize())) + QStringLiteral("px")},
        {QStringLiteral("--ambience-primary"),
         cssColor(themeColor(ambience, "primaryColor", Qt::white))},
        {QStringLiteral("--ambience-secondary"),
         cssColor(themeColor(ambience, "secondaryColor", QColor(255, 255, 255, 176)))},
        {QStringLiteral("--ambience-highlight"),
         cssColor(themeColor(ambience, "highlightColor", QColor(QStringLiteral("#ffc480"))))},
        {QStringLiteral("--ambience-secondary-highlight"),
         cssColor(themeColor(ambience, "secondaryHighlightColor", QColor(255, 196, 128, 176)))},
        {QStringLiteral("--ambience-selection"), cssColor(selection)},
        {QStringLiteral("--ambience-top"),
         cssColor(themeColor(ambience, "highlightDimmerColor", QColor(QStringLiteral("#4a2309"))))},
        {QStringLiteral("--ambience-bottom"), cssColor(ambienceBackground(ambience))},
        {QStringLiteral("--ambience-font"), cssFamily(ambience, "fontFamily")},
        {QStringLiteral("--ambience-heading-font"), cssFamily(ambience, "fontFamilyHeading")},
    };
}

// Paints cutout strip.
QString Reader::themeBackground(const QVariantMap &ambience) const
{
    const QString scheme =
        colorScheme(isDarkAmbience(themeColor(ambience, "primaryColor", Qt::white)));
    if (scheme == QLatin1String("ambience")) {
        return themeColor(ambience, "highlightDimmerColor", QColor(QStringLiteral("#4a2309")))
            .name();
    }
    return backgroundOf(scheme).name();
}

QColor Reader::ambienceBackground(const QVariantMap &ambience)
{
    const QColor top =
        themeColor(ambience, "highlightDimmerColor", QColor(QStringLiteral("#4a2309")));
    const QColor overlay = themeColor(ambience, "overlayBackgroundColor", Qt::black);
    return QColor::fromRgbF((top.redF() + overlay.redF()) / 2,
                            (top.greenF() + overlay.greenF()) / 2,
                            (top.blueF() + overlay.blueF()) / 2);
}

QColor Reader::backgroundOf(const QString &scheme)
{
    if (scheme == QLatin1String("dark")) {
        return {28, 27, 34};
    }
    if (scheme == QLatin1String("sepia")) {
        return {244, 236, 216};
    }
    return {255, 255, 255};
}

QColor Reader::textColorOf(const QString &scheme)
{
    if (scheme == QLatin1String("dark")) {
        return {251, 251, 254};
    }
    if (scheme == QLatin1String("sepia")) {
        return {91, 70, 54};
    }
    return {21, 20, 26};
}

QColor Reader::linkColorOf(const QString &scheme)
{
    return scheme == QLatin1String("dark") ? QColor(0, 221, 255) : QColor(0, 97, 224);
}

int Reader::fontSizeFor(int step)
{
    return 10 + 2 * step;
}

QString Reader::page(const QString &article, const QString &pageUrl, const QString &pageTitle,
                     const QString &favicon, const QVariantMap &ambience) const
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

    // Only valid language tag / direction allowed in attributes.
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

    QString style;
    for (const auto &property : bodyProperties(ambience)) {
        style += property.first + QStringLiteral(": ") + property.second + QStringLiteral("; ");
    }
    QString html;
    html += QLatin1String(HeadStart) + attribute(pageUrl) + QStringLiteral("\">");
    html += QStringLiteral("<meta http-equiv=\"Content-Security-Policy\" content=\"%1\">")
                .arg(QLatin1String(ContentSecurityPolicy));
    html += QStringLiteral("<meta name=\"viewport\" content=\"width=device-width, "
                           "initial-scale=1\">");
    html +=
        QStringLiteral("<meta name=\"theme-color\" content=\"%1\">").arg(themeBackground(ambience));
    html += iconLink;
    html += QStringLiteral("<title>%1</title>").arg(documentTitle.toHtmlEscaped());
    html += QStringLiteral("<style>") + m_styleSheet + QStringLiteral("</style></head>");
    html += QStringLiteral("<body class=\"%1\" style=\"%2\">")
                .arg(bodyClass(ambience), attribute(style.trimmed()));
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
    // Append, not arg(): article may contain "%1".
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

QString Reader::styleScript(const QVariantMap &ambience) const
{
    // Only quotes are cssFamily()'s, escaped here.
    QString properties;
    for (const auto &property : bodyProperties(ambience)) {
        QString value = property.second;
        value.replace(QLatin1Char('\''), QLatin1String("\\'"));
        properties += QStringLiteral(" document.body.style.setProperty('%1', '%2');")
                          .arg(property.first, value);
    }
    return QStringLiteral(
               " if (!document.body || !document.querySelector('meta[name=\"salama-reader\"]'))"
               " { return ''; }"
               " document.body.className = '%1';"
               "%2"
               " var color = document.querySelector('meta[name=\"theme-color\"]');"
               " if (color) { color.content = '%3'; }"
               " return '';")
        .arg(bodyClass(ambience), properties, themeBackground(ambience));
}

QString Reader::readingTime(int length, const QString &language)
{
    if (length <= 0) {
        return {};
    }
    const ReadingSpeed &speed = readingSpeed(language);
    const int slow = qCeil(double(length) / (speed.cpm - speed.variance));
    const int fast = qCeil(double(length) / (speed.cpm + speed.variance));
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
