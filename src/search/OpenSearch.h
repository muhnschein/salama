// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include <QString>

namespace Salama {

// A search engine as an OpenSearch description says it: a name, and the address of a page
// of results with the words in it as {searchTerms}
// (docs/DECISIONS/0041-search-engines-found.md).
struct OpenSearchEngine
{
    QString name;
    QString urlTemplate;

    // A description can leave out its name and still say where results are; one that says
    // no such place says nothing.
    bool isValid() const;
};

// Reading what a site offers when it says <link rel="search"
// type="application/opensearchdescription+xml">: the document at the link's address.
// sailfish-browser has the platform fetch and keep it as a file, and reads the file with
// its own configuration code (apps/browser/settings/searchenginemodel.cpp); here the
// text is read in plain Qt, for the one kind of page a browser can use, and anything not
// understood is refused rather than half used.
class OpenSearch
{
public:
    // What stands for the words searched for in a template, as OpenSearch writes it.
    static QString marker();

    // The first <Url> that is a page of results -- type text/html, method GET, the rel
    // "results" it has by default -- and the <ShortName>. Parameters the browser has no
    // value for ({count?}, {language} and the rest) are left empty, and <Param> children
    // are added to the query. A template is on http or https and has {searchTerms} in it,
    // but not in its host; anything else, and text that is not well-formed XML or is more
    // than a description needs, is an engine that is not valid.
    static OpenSearchEngine parse(const QString &xml);

    // Whether text is a template parse() would give: on the web, with {searchTerms} in
    // it and not in its host. What is read back from a settings file is held to it too.
    static bool isTemplate(const QString &urlTemplate);

    // The template with the words in it, percent-encoded as one query value is.
    static QString fill(const QString &urlTemplate, const QString &words);
};

} // namespace Salama
