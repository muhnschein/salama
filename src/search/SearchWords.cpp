// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "SearchWords.h"

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

} // namespace Salama
