// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include <QString>
#include <QStringList>

namespace Salama {

// Every word in any field: "news helsinki" finds "Helsinki news". Qt::CaseInsensitive folds
// Unicode ("äly" finds "Älypuhelin"); SQLite LIKE is ASCII only, so not used.
class SearchWords
{
public:
    explicit SearchWords(const QString &text = QString());

    const QStringList &words() const;
    bool isEmpty() const;

    bool matches(const QStringList &fields) const;

    // First typed word only. Word boundary = after non-alnum. False if empty.
    bool prefixes(const QString &text) const;
    bool prefixesAWordOf(const QString &text) const;

    // StyledText; page text fully escaped.
    QString marked(const QString &text) const;

private:
    QStringList m_words;
};

} // namespace Salama
