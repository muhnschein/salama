// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "SearchWords.h"

#include <QPair>
#include <QVector>
#include <algorithm>

namespace Salama {

SearchWords::SearchWords(const QString &text)
{
    // simplified() trims the ends and turns every run of whitespace, Unicode's as well
    // as ASCII's, into one space; what is left splits into words without empty ones.
    const QString simple = text.simplified();
    if (!simple.isEmpty()) {
        m_words = simple.split(QLatin1Char(' '));
    }
}

const QStringList &SearchWords::words() const
{
    return m_words;
}

bool SearchWords::isEmpty() const
{
    return m_words.isEmpty();
}

bool SearchWords::matches(const QStringList &fields) const
{
    return std::all_of(m_words.cbegin(), m_words.cend(), [&fields](const QString &word) {
        return std::any_of(fields.cbegin(), fields.cend(), [&word](const QString &field) {
            return field.contains(word, Qt::CaseInsensitive);
        });
    });
}

bool SearchWords::prefixes(const QString &text) const
{
    return !m_words.isEmpty() && text.startsWith(m_words.first(), Qt::CaseInsensitive);
}

bool SearchWords::prefixesAWordOf(const QString &text) const
{
    if (m_words.isEmpty()) {
        return false;
    }
    const QString &first = m_words.first();
    for (int at = text.indexOf(first, 0, Qt::CaseInsensitive); at >= 0;
         at = text.indexOf(first, at + 1, Qt::CaseInsensitive)) {
        if (at == 0 || !text.at(at - 1).isLetterOrNumber()) {
            return true;
        }
    }
    return false;
}

QString SearchWords::marked(const QString &text) const
{
    // Where each word appears, as [start, end), in order of where they start.
    QVector<QPair<int, int>> spans;
    for (const QString &word : m_words) {
        for (int at = text.indexOf(word, 0, Qt::CaseInsensitive); at >= 0;
             at = text.indexOf(word, at + word.length(), Qt::CaseInsensitive)) {
            spans.append(qMakePair(at, at + word.length()));
        }
    }
    std::sort(spans.begin(), spans.end());

    QString styled;
    int written = 0;
    int next = 0;
    while (next < spans.count()) {
        const int start = spans.at(next).first;
        int end = spans.at(next).second;
        // The places after it that begin before it ends are part of it.
        for (++next; next < spans.count() && spans.at(next).first <= end; ++next) {
            end = std::max(end, spans.at(next).second);
        }
        styled += text.mid(written, start - written).toHtmlEscaped();
        styled += QLatin1String("<b>") + text.mid(start, end - start).toHtmlEscaped() +
                  QLatin1String("</b>");
        written = end;
    }
    return styled + text.mid(written).toHtmlEscaped();
}

} // namespace Salama
