// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

class QLocale;
class QString;
class QTranslator;

namespace Salama {

// Falls back to harbour-salama.qm (English plurals for unknown languages).
// Install before QML loads: Qt 5.6 no retranslate of built pages.
bool loadTranslations(QTranslator &translator, const QLocale &locale, const QString &directory);

} // namespace Salama
