// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

class QLocale;
class QString;
class QTranslator;

namespace Salama {

// Loads into `translator` the catalog for `locale` from `directory`, and says whether there
// was one. Which file that is follows the reader's Language setting, through the locale the
// system starts the app under: harbour-salama-de_DE.qm if there is one, else
// harbour-salama-de.qm, else harbour-salama.qm -- the English source catalog, which carries
// the English plural forms for a language Salama has no catalog for.
//
// Installed before any QML is loaded: Qt 5.6 does not retranslate a page already built.
bool loadTranslations(QTranslator &translator, const QLocale &locale, const QString &directory);

} // namespace Salama
