// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Every language catalog carries every string, translated.
//
// A scan of the .ts files rather than anything Qt: lupdate keeps the catalogs
// in step with the source and ci/packaging-lint.sh proves that, but neither
// says whether a string in one of them has been translated. A string left
// unfinished is one the reader of that language meets in English, and with
// forty catalogs nobody notices from the diff.
//
// harbour-salama.ts is the untranslated source catalog, and
// harbour-salama-en.ts exists only for its plural forms, so those two are
// held to different rules.
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>
#include <QTextStream>
#include <QtTest>

namespace {

const char *const TranslationsDir = SALAMA_SOURCE_DIR "/translations";
const char *const SourceCatalog = "harbour-salama.ts";
const char *const Prefix = "harbour-salama-";
// The set of a catalog's strings is keyed by context and source, as lupdate
// keeps them: the same source text under two contexts is two strings.
const QChar Separator = QChar(0x1f);

struct Catalog
{
    QString language;
    QSet<QString> sources;
    QStringList unfinished;
};

Catalog readCatalog(const QString &path)
{
    Catalog catalog;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return catalog;
    }
    QTextStream in(&file);
    in.setCodec("UTF-8");
    const QRegularExpression pattern(QStringLiteral("language=\"([^\"]*)\""));
    QString context;
    QString source;
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.startsWith(QLatin1String("<TS "))) {
            const auto match = pattern.match(line);
            if (match.hasMatch()) {
                catalog.language = match.captured(1);
            }
        } else if (line.startsWith(QLatin1String("<name>"))) {
            context = line.mid(6, line.size() - 13);
        } else if (line.startsWith(QLatin1String("<source>"))) {
            source = line.mid(8, line.size() - 17);
            catalog.sources.insert(context + Separator + source);
        } else if (line.startsWith(QLatin1String("<translation type=\"unfinished\""))) {
            catalog.unfinished.append(context + QStringLiteral(": ") + source);
        }
    }
    return catalog;
}

// What is wrong with one catalog, if anything, in the words the failure will
// read. English needs only its plurals: everything else falls back to the
// source text, which is English already.
QStringList checkCatalog(const QDir &dir, const QString &name, const Catalog &source)
{
    const Catalog catalog = readCatalog(dir.filePath(name));
    const int prefixLength = QLatin1String(Prefix).size();
    const QString language = name.mid(prefixLength, name.size() - prefixLength - 3);
    QStringList problems;
    if (catalog.language != language) {
        problems.append(QStringLiteral("%1: language is not %2").arg(name, language));
    }
    if (catalog.sources != source.sources) {
        problems.append(QStringLiteral("%1: strings differ").arg(name));
    }
    if (language == QLatin1String("en")) {
        for (const QString &entry : catalog.unfinished) {
            if (entry.contains(QLatin1String("%n"))) {
                problems.append(QStringLiteral("%1: plural untranslated %2").arg(name, entry));
            }
        }
        return problems;
    }
    for (const QString &entry : catalog.unfinished) {
        problems.append(QStringLiteral("%1: untranslated %2").arg(name, entry));
    }
    return problems;
}

} // namespace

class TestTranslations : public QObject
{
    Q_OBJECT

private slots:
    void everyLanguageCatalogIsComplete();
};

void TestTranslations::everyLanguageCatalogIsComplete()
{
    const QDir dir{QLatin1String(TranslationsDir)};
    const Catalog source = readCatalog(dir.filePath(QLatin1String(SourceCatalog)));
    QVERIFY2(source.sources.size() > 100, "the source catalog holds too few strings");

    const QStringList filters{QStringLiteral("harbour-salama-*.ts")};
    QStringList catalogs = dir.entryList(filters, QDir::Files);
    catalogs.sort();
    QVERIFY2(catalogs.size() >= 40, "fewer than forty language catalogs");

    QStringList problems;
    for (const QString &name : catalogs) {
        problems.append(checkCatalog(dir, name, source));
    }
    QVERIFY2(problems.isEmpty(), qPrintable(problems.join(QLatin1String("\n  "))));
}

QTEST_GUILESS_MAIN(TestTranslations)
#include "tst_translations.moc"
