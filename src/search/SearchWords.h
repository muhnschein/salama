// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include <QString>
#include <QStringList>

namespace Salama {

// What a search typed into the browser means, in one place, so that every list searched
// agrees on it: the grid's "Search tabs" and the address bar's suggestions
// (docs/DECISIONS/0027-omnibar.md).
//
// The text is taken as words, split on whitespace, and a candidate matches when every
// word appears somewhere in it -- one word in the title and another in the address will
// do -- as Firefox's address bar matches. A single string searched for whole would miss
// "news helsinki" in a page titled "Helsinki news", which is how people remember pages.
// Each word is compared with Qt::CaseInsensitive, which folds case by the Unicode
// tables and not only ASCII's: "äly" finds "Älypuhelin". That is also why nothing is
// handed to SQLite's LIKE, whose case folding is ASCII's alone.
class SearchWords
{
public:
    explicit SearchWords(const QString &text = QString());

    const QStringList &words() const;
    bool isEmpty() const;

    // Every word in one field or another. No words at all match everything: a search
    // not yet typed hides nothing.
    bool matches(const QStringList &fields) const;

    // How a match is ranked: whether the text begins with the first word typed, and
    // whether one of the text's own words does -- the first word being what the reader
    // began with, and so the likeliest start of what they are after. A word begins
    // where the text does or after anything that is not a letter or a digit, so
    // "wiki" begins a word of "Sailfish (Wikipedia)". False while nothing is typed.
    bool prefixes(const QString &text) const;
    bool prefixesAWordOf(const QString &text) const;

    // The text as Qt's StyledText, every place a word typed appears in it in bold, as
    // Firefox's address bar makes the words typed stand out in what it found. The text
    // is a page's own, so all of it is escaped: nothing in it can be taken for markup.
    // Case folds as it does in matches(); places that overlap are one.
    QString marked(const QString &text) const;

private:
    QStringList m_words;
};

} // namespace Salama
