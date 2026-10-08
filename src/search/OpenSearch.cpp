// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "OpenSearch.h"

#include <QPair>
#include <QRegularExpression>
#include <QUrl>
#include <QVector>
#include <QXmlStreamReader>

namespace Salama {

namespace {

// Real descriptions are ~100s of bytes.
const int MaxDescription = 64 * 1024;

// Substituted for words when checking template: if in host, words would pick host.
const char *const Probe = "searchtermsprobe";

struct UrlElement
{
    QString type;
    QString method;
    QString rel;
    QString urlTemplate;
    QVector<QPair<QString, QString>> params;
};

UrlElement readUrl(QXmlStreamReader &reader)
{
    const QXmlStreamAttributes attributes = reader.attributes();
    UrlElement url;
    url.type = attributes.value(QLatin1String("type")).toString().trimmed().toLower();
    url.method = attributes.value(QLatin1String("method")).toString().trimmed().toLower();
    url.rel = attributes.value(QLatin1String("rel")).toString().trimmed().toLower();
    url.urlTemplate = attributes.value(QLatin1String("template")).toString().trimmed();
    while (!reader.atEnd()) {
        const QXmlStreamReader::TokenType token = reader.readNext();
        if (token == QXmlStreamReader::EndElement) {
            break;
        }
        if (token != QXmlStreamReader::StartElement) {
            continue;
        }
        if (reader.name() == QLatin1String("Param")) {
            url.params.append({reader.attributes().value(QLatin1String("name")).toString(),
                               reader.attributes().value(QLatin1String("value")).toString()});
        }
        reader.skipCurrentElement();
    }
    return url;
}

bool isResultsPage(const UrlElement &url)
{
    const bool results =
        url.rel.isEmpty() || url.rel.split(QLatin1Char(' ')).contains(QLatin1String("results"));
    const bool get = url.method.isEmpty() || url.method == QLatin1String("get");
    return url.type == QLatin1String("text/html") && get && results;
}

// Other params optional per OpenSearch; nothing to fill them with.
QString withOnlyTheWords(const QString &text)
{
    static const QRegularExpression parameter(QStringLiteral("\\{([^{}]*)\\}"));
    QString result;
    int end = 0;
    QRegularExpressionMatchIterator matches = parameter.globalMatch(text);
    while (matches.hasNext()) {
        const QRegularExpressionMatch match = matches.next();
        result += text.mid(end, match.capturedStart() - end);
        QString name = match.captured(1);
        if (name.endsWith(QLatin1Char('?'))) {
            name.chop(1);
        }
        if (name == QLatin1String("searchTerms")) {
            result += OpenSearch::marker();
        }
        end = match.capturedEnd();
    }
    return result + text.mid(end);
}

QString withParams(QString urlTemplate, const QVector<QPair<QString, QString>> &params)
{
    for (const QPair<QString, QString> &param : params) {
        if (param.first.isEmpty()) {
            continue;
        }
        // Keep {}? unencoded for withOnlyTheWords().
        urlTemplate += urlTemplate.contains(QLatin1Char('?')) ? QLatin1Char('&') : QLatin1Char('?');
        urlTemplate += QString::fromLatin1(QUrl::toPercentEncoding(param.first)) +
                       QLatin1Char('=') +
                       QString::fromLatin1(QUrl::toPercentEncoding(param.second, "{}?"));
    }
    return urlTemplate;
}

bool isWebAddress(const QString &urlTemplate)
{
    QString probed = urlTemplate;
    probed.replace(OpenSearch::marker(), QLatin1String(Probe));
    const QUrl url(probed, QUrl::TolerantMode);
    const QString scheme = url.scheme();
    return (scheme == QLatin1String("http") || scheme == QLatin1String("https")) &&
           !url.host().isEmpty() && !url.host().contains(QLatin1String(Probe));
}

} // namespace

bool OpenSearchEngine::isValid() const
{
    return !urlTemplate.isEmpty();
}

QString OpenSearch::marker()
{
    return QStringLiteral("{searchTerms}");
}

OpenSearchEngine OpenSearch::parse(const QString &xml)
{
    if (xml.size() > MaxDescription) {
        return {};
    }
    QXmlStreamReader reader(xml);
    OpenSearchEngine engine;
    bool named = false;
    while (!reader.atEnd()) {
        if (reader.readNext() != QXmlStreamReader::StartElement) {
            continue;
        }
        if (reader.name() == QLatin1String("ShortName") && !named) {
            engine.name = reader.readElementText().trimmed();
            named = true;
        } else if (reader.name() == QLatin1String("Url") && engine.urlTemplate.isEmpty()) {
            const UrlElement url = readUrl(reader);
            if (isResultsPage(url)) {
                engine.urlTemplate = withOnlyTheWords(withParams(url.urlTemplate, url.params));
            }
        }
    }
    // Partial XML not trusted even for earlier part.
    if (reader.hasError() || !isTemplate(engine.urlTemplate)) {
        return {};
    }
    return engine;
}

bool OpenSearch::isTemplate(const QString &urlTemplate)
{
    return urlTemplate.contains(marker()) && isWebAddress(urlTemplate);
}

QString OpenSearch::fill(const QString &urlTemplate, const QString &words)
{
    QString filled = urlTemplate;
    filled.replace(marker(), QString::fromLatin1(QUrl::toPercentEncoding(words.trimmed())));
    return filled;
}

} // namespace Salama
