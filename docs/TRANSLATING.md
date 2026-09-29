# Translating Salama

Salama ships in the forty languages Sailfish OS ships in. Every `qsTr()` in
`qml/` and `src/` has a catalog entry in each of them, and a catalog with an
`unfinished` string is a line a reader meets in English — which, across forty
files, nobody notices from the diff. So the catalogs are held to that, not left
to review.

## The words on screen

Every English string was written to be the source a translator works from, not
only what an English reader sees. The rules are the ones piirit and vuo keep,
because the three apps share a voice:

- **A finished sentence with a subject and a verb.** No verbless coda, no
  epigram, no rule of three. "... Every profile, from now on" is fine in English
  and broken in an inflected language.
- **Literal verbs.** "Formats", not "draws"; "stores", not "holds". A figure a
  translator can only take literally comes out wrong.
- **No phrasal verb where a plain one exists.** "Copy", not "take over";
  "Remove", not "take away".
- **Sentence case**, not Title Case. `…` rather than three dots.
- **Say what the thing does, once.** A button, a heading or a line under one has
  no room for a second clause.
- Where the sibling apps have a noun for a thing — they are Sailfish apps and
  they ship in the same languages — use it. A string shared with them, like a
  menu name or `%n tab(s)`, is the same source text, and its translation is
  theirs too.

The `extracomment` beside a string says where it appears and what it must
convey; it is the thing a one-word dictionary translation gets wrong. The
`context` is the QML type or C++ class it lives in.

## The catalogs

`translations/harbour-salama.ts` is the untranslated source catalog: lupdate
reads the strings from it and it is never compiled. One
`translations/harbour-salama-<lang>.ts` sits beside it per language, and
`harbour-salama-en.ts` exists only for English's plural forms — everything else
in English falls back to the source text.

    make translations

regenerates them all from the `qsTr()` calls, through
`scripts/update-translations.sh`. `lrelease` compiles each into a `.qm` at build
time (`translations/CMakeLists.txt`), and `main.cpp` loads the one for the
reader's locale with `QTranslator`.

A string added to the source appears as `unfinished` in every catalog at once.
Change a source string and its translation is orphaned in all forty: `make
translations` fills the new one as unfinished, and
`tests/tst_translations.cpp` fails until each is refilled. The cost of getting
the English wrong is paid forty times, so write it once.

## How it is guarded

| Guard | What it proves |
|---|---|
| `tests/tst_translations.cpp` | every catalog carries the source catalog's strings, its `language` attribute names it, and no string is left `unfinished` (English: only its plurals must be filled). At least forty catalogs exist. |
| `ci/packaging-lint.sh` | `lrelease` compiles every catalog, each carries the same source strings as `harbour-salama.ts`, and the source catalog is not stale against lupdate. |
| `ci/qml-lint.sh` | no user-visible string reaches a page without `qsTr()`. |

All three are part of `make check`, so a language added without its translation,
or a string added without a catalog entry, is a red build and not a delivery.

## Plurals

A `%n` string carries as many `<numerusform>`s as Qt's rules give the language:
one for Chinese, Vietnamese, Hungarian, Turkish and Tatar; two for most others,
including English; three for Czech, Slovak, Polish, Romanian, Russian, Ukrainian,
Lithuanian and Latvian; four for Slovenian. `lrelease` is the authority — a form
too few is a compile error in `packaging-lint`. Keep `%n` in every form, and keep
`%1`, `%2` and `%3` with their meaning; their place in the sentence may move.

## Adding a language

Write the header to `translations/harbour-salama-<lang>.ts`:

    <?xml version="1.0" encoding="utf-8"?>
    <!DOCTYPE TS>
    <TS version="2.1" language="<lang>"></TS>

run `make translations` to fill every string, translate them, and commit.
`<lang>` is what `QTranslator` matches against the reader's locale: `de` serves
every German locale, `pt_BR` only Brazil. `tests/tst_translations.cpp` fails a
catalog that adds nothing, names itself wrongly, or is left unfinished.
