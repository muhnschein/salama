// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "Translations.h"

#include <QLocale>
#include <QString>
#include <QTranslator>

namespace Salama {

bool loadTranslations(QTranslator &translator, const QLocale &locale, const QString &directory)
{
    // "-" sits between the name and the locale in the file name. load() tries the locale's
    // languages from the most specific down, so harbour-salama-pt_BR.qm wins over
    // harbour-salama-pt.qm, and ends at harbour-salama.qm when no language matched. That
    // last only with the suffix given: left to its default, load() looks for the fallback
    // as "harbour-salama", without one, and finds nothing.
    return translator.load(locale, QStringLiteral("harbour-salama"), QStringLiteral("-"), directory,
                           QStringLiteral(".qm"));
}

} // namespace Salama
