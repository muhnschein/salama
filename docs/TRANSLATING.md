# Translating

Translated into every language Sailfish OS ships. English is source.

## Language selection

`translations/harbour-salama-<lang>.ts` compiles to `harbour-salama-<lang>.qm`, installed
under `share/harbour-salama/translations`. At start `Salama::loadTranslations`
(`src/Translations.h`) loads catalog for system locale (reader's Language setting):
`-pt_BR.qm` Brazil, `-pt.qm` Portugal, `-de.qm` every German locale. No catalog →
`harbour-salama.qm`, English source, holding only English plural forms ("1 page", "2 pages").

## Files

- `harbour-salama.ts`: source catalog. Every `qsTr()`/`tr()` in `qml/` and `src/`, English plurals filled.
- `harbour-salama-<lang>.ts`: one language. `language` attribute = `<lang>`; lupdate counts plural forms from it.

Never edit string list by hand. `make translations` regenerates all catalogs in one run:
new string lands unfinished everywhere, removed string leaves everywhere. New language: write
header to `translations/harbour-salama-<lang>.ts`, run `make translations`:

```xml
<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="<lang>"></TS>
```

Build picks up every catalog in `translations/`.

## Gates

- `ci/packaging-lint.sh`: every catalog compiles with lrelease, no warning (missing plural
  form is one); regenerating from source changes none.
- `tests/tst_translations.cpp`: catalog per Sailfish OS language; each has exactly source
  strings, own `language`, none unfinished; locale installs right one, country before
  language; plurals by language rule; no catalog → English.

Changed English string fails build until every catalog has it again. Cost paid 39 times.
Get English right once.

## Writing English

English is translator's source. Metaphor, ellipsis, ambiguous phrasal verbs don't survive.
Say what thing does, plain words:

- Literal verbs; plain verb over phrasal.
- Firefox's wording for browser concepts: translations exist in every language.
- `//:` comment above anything readable two ways: where shown, what `%1` is, whether "Clear" is verb.
- Sentence case, `…` not three dots, `%n` for counts so plural works.

## Filling a catalog

Two references, in order:

1. **Piirit's catalog** (`translations/piirit-<lang>.ts` in
   [Piirit](https://github.com/muhnschein/piirit)) for register and style: address form,
   button as imperative/infinitive/noun, quotes, punctuation. German uses *du* (Piirit does),
   though German Firefox uses *Sie*.
2. **Firefox** for words. Firefox for Android catalog
   ([android-l10n](https://github.com/mozilla-l10n/android-l10n)) first, desktop Firefox
   where Android thin. Firefox is engine upstream; readers know its words for tabs,
   bookmarks, history, downloads, tracking protection, site permissions.

*Salama* is a name, never translated. Placeholders (`%1`, `%n`) stay; every plural form
keeps `%n`: Qt picks form by rule, some languages' "one" form also serves 21, 31.
