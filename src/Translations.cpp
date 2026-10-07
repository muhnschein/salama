// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "Translations.h"

#include <QLocale>
#include <QString>
#include <QTranslator>

namespace Salama {

bool loadTranslations(QTranslator &translator, const QLocale &locale, const QString &directory)
{
    // load() tries most specific first (pt_BR before pt), falls back to harbour-salama.qm.
    // Suffix must be explicit: default suffix makes fallback lookup miss.
    return translator.load(locale, QStringLiteral("harbour-salama"), QStringLiteral("-"), directory,
                           QStringLiteral(".qm"));
}

} // namespace Salama
