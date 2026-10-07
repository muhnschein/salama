# Translating

Salama is translated into every language Sailfish OS ships in. English is the source.

## How a reader gets their language

`translations/harbour-salama-<lang>.ts` compiles to `harbour-salama-<lang>.qm`, installed
under `share/harbour-salama/translations`. At start-up `Salama::loadTranslations`
(`src/Translations.h`) loads the one for the locale the system starts the app under, which
is the reader's Language setting: `harbour-salama-pt_BR.qm` for Brazil, `-pt.qm` for
Portugal, `-de.qm` for every German locale. A language with no catalog of its own gets
`harbour-salama.qm`, the English source catalog, whose only translations are the English
plural forms ("1 page", "2 pages" rather than "2 page(s)").

## The files

- `harbour-salama.ts` is the source catalog: every `qsTr()` and `tr()` in `qml/` and `src/`,
  with the English plurals filled in.
- `harbour-salama-<lang>.ts` is one language. Its `language` attribute is the file's
  `<lang>`; lupdate counts the plural forms from it.

Nobody edits the strings' list by hand. `make translations` regenerates every catalog from
the source in one run, so a string added to the source arrives unfinished in every language
at once, and one removed from the source leaves every catalog. To add a language, write its
header to `translations/harbour-salama-<lang>.ts` and run `make translations`:

```xml
<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="<lang>"></TS>
```

The build picks every catalog in `translations/` up by itself.

## What the gates hold a catalog to

- `ci/packaging-lint.sh`: every catalog compiles with lrelease without a warning (a plural
  form too few is one), and regenerating them all from the source changes none of them.
- `tests/tst_translations.cpp`: every language Sailfish OS ships in has a catalog; each
  carries exactly the source catalog's strings, under its own `language`, with none left
  unfinished; the reader's locale installs the right one, a country's own before its
  language's; plurals are counted by the language's rule; and a language without a catalog
  gets English.

So a changed English string fails the build until every catalog has it again: it orphans its
translation in all of them, and the cost of getting the English wrong is paid thirty-nine
times. Write it once.

## Writing the English

The English is not only what a reader sees; it is the source text a translator works from,
and English that leans on metaphor, ellipsis or an ambiguous phrasal verb does not survive
the trip. A string says what the thing does, in plain words:

- Literal verbs, and a plain verb where there is a phrasal one.
- Firefox's wording for a browser concept where Firefox has one: its translations then
  have an answer in every language.
- A `//:` comment above anything a translator could read two ways: where it is shown,
  what `%1` is, whether "Clear" is a verb.
- Sentence case, `…` rather than three dots, and `%n` for a count so it can be a plural.

## Filling a catalog

Each language follows two references, in this order:

1. **Piirit's catalog for the language** (`translations/piirit-<lang>.ts` in
   [Piirit](https://github.com/muhnschein/piirit)) for register and style: how the reader is
   addressed, whether a button is an imperative, an infinitive or a noun, quotation marks,
   punctuation. German says *du* because Piirit does, though German Firefox says *Sie*.
2. **Firefox** for the words. Firefox for Android's catalog (the
   [android-l10n](https://github.com/mozilla-l10n/android-l10n) repository) first, desktop
   Firefox's where Android's is thin. Firefox is the browser upstream of Salama's engine,
   and a reader who has used it already knows its words for tabs, bookmarks, history,
   downloads, tracking protection and site permissions.

*Salama* is a name and is never translated. Placeholders (`%1`, `%n`) stay; every plural
form keeps its `%n`, because Qt picks a form by rule and in some languages the "one" form
also serves 21 and 31.