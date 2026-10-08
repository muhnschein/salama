// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include <QString>

namespace Salama {

struct OpenSearchEngine
{
    QString name;
    QString urlTemplate;

    bool isValid() const;
};

// Anything not understood refused, not half used.
class OpenSearch
{
public:
    static QString marker();

    // First text/html GET results <Url>. Other params dropped; <Param> appended. Invalid unless
    // http(s) with {searchTerms} not in host, well-formed, size-bounded.
    static OpenSearchEngine parse(const QString &xml);

    // Also applied to settings file values.
    static bool isTemplate(const QString &urlTemplate);

    static QString fill(const QString &urlTemplate, const QString &words);
};

} // namespace Salama
