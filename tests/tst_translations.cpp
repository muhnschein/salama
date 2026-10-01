// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "Translations.h"

#include <algorithm>

#include <QDir>
#include <QFile>
#include <QLocale>
#include <QMap>
#include <QSet>
#include <QTranslator>
#include <QXmlStreamReader>
#include <QtTest>

using Salama::installTranslations;

namespace {

// The languages Sailfish OS ships in, besides English: one catalog each. Piirit, the sister
// app, is translated into the same set.
QStringList sailfishLanguages()
{
    return QStringLiteral("bg bn cs da de el es et fi fr gu hi hu it kn lt lv ml mr nb nl pa pl "
                          "pt pt_BR ro ru sk sl sv ta te tr tt uk vi zh_CN zh_HK zh_TW")
        .split(QLatin1Char(' '));
}

struct Message
{
    QString context;
    QString source;
    bool unfinished = false;
    QStringList forms; // one, or one per plural form
};

struct Catalog
{
    QString language;
    QList<Message> messages;
};

QString sourceDir()
{
    return QStringLiteral(SALAMA_SOURCE_DIR "/translations");
}

Catalog readCatalog(const QString &path)
{
    Catalog catalog;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return catalog;
    }
    QXmlStreamReader xml(&file);
    QString context;
    Message message;
    while (!xml.atEnd()) {
        xml.readNext();
        if (!xml.isStartElement()) {
            continue;
        }
        const QStringRef name = xml.name();
        if (name == QLatin1String("TS")) {
            catalog.language = xml.attributes().value(QStringLiteral("language")).toString();
        } else if (name == QLatin1String("name")) {
            context = xml.readElementText();
        } else if (name == QLatin1String("message")) {
            message = Message{context, {}, false, {}};
        } else if (name == QLatin1String("source")) {
            message.source = xml.readElementText();
        } else if (name == QLatin1String("translation")) {
            message.unfinished =
                xml.attributes().value(QStringLiteral("type")) == QLatin1String("unfinished");
            // A plural translation holds its forms; any other holds its text.
            QString text;
            while (!(xml.isEndElement() && xml.name() == QLatin1String("translation"))) {
                xml.readNext();
                if (xml.isStartElement() && xml.name() == QLatin1String("numerusform")) {
                    message.forms << xml.readElementText();
                } else if (xml.isCharacters()) {
                    text += xml.text();
                }
            }
            if (message.forms.isEmpty()) {
                message.forms << text;
            }
            catalog.messages << message;
        }
    }
    return catalog;
}

QSet<QString> keys(const Catalog &catalog)
{
    QSet<QString> out;
    for (const Message &message : catalog.messages) {
        out.insert(message.context + QLatin1Char('|') + message.source);
    }
    return out;
}

Message find(const Catalog &catalog, const QString &context, const QString &source)
{
    for (const Message &message : catalog.messages) {
        if (message.context == context && message.source == source) {
            return message;
        }
    }
    return {};
}

} // namespace

class tst_translations : public QObject
{
    Q_OBJECT

private slots:
    void everySailfishLanguageHasACatalog();
    void everyLanguageCatalogIsComplete();
    void sourceCatalogCarriesEnglishPlurals();
    void readersLanguageIsInstalled_data();
    void readersLanguageIsInstalled();
    void pluralsAreCountedByTheLanguagesRule();
    void languageWithoutACatalogGetsEnglish();

    void cleanup();

private:
    static QString qmDir()
    {
        return QStringLiteral(SALAMA_TRANSLATIONS_DIR);
    }
    QTranslator *m_installed = nullptr;
};

void tst_translations::cleanup()
{
    if (m_installed) {
        QCoreApplication::removeTranslator(m_installed);
        delete m_installed;
        m_installed = nullptr;
    }
}

void tst_translations::everySailfishLanguageHasACatalog()
{
    QStringList missing;
    for (const QString &language : sailfishLanguages()) {
        const QString name = QStringLiteral("harbour-salama-%1.ts").arg(language);
        if (!QFile::exists(sourceDir() + QLatin1Char('/') + name)) {
            missing << name;
        }
    }
    QVERIFY2(missing.isEmpty(),
             qPrintable(QStringLiteral("Sailfish OS ships in these languages and Salama has no "
                                       "catalog for them: ") +
                        missing.join(QStringLiteral(", "))));
}

// A string left unfinished is one the reader of that language meets in English, and with
// forty catalogs nobody notices from the diff.
void tst_translations::everyLanguageCatalogIsComplete()
{
    const Catalog source = readCatalog(sourceDir() + QStringLiteral("/harbour-salama.ts"));
    const QSet<QString> wanted = keys(source);
    QVERIFY2(wanted.size() > 100, "the source catalog holds too few strings; did lupdate run?");

    QStringList problems;
    const QStringList files =
        QDir(sourceDir())
            .entryList({QStringLiteral("harbour-salama-*.ts")}, QDir::Files, QDir::Name);
    for (const QString &file : files) {
        const QString prefix = QStringLiteral("harbour-salama-");
        const QString language = file.mid(prefix.size(), file.size() - prefix.size() - 3);
        const Catalog catalog = readCatalog(sourceDir() + QLatin1Char('/') + file);
        // What lupdate counts the plural forms from, and which reader the file is for.
        if (catalog.language != language) {
            problems << QStringLiteral("%1: its language attribute is %2, not %3")
                            .arg(file, catalog.language, language);
        }
        if (keys(catalog) != wanted) {
            problems << QStringLiteral("%1: not the source catalog's strings; run make "
                                       "translations")
                            .arg(file);
        }
        QStringList unfinished;
        for (const Message &message : catalog.messages) {
            const bool empty = std::any_of(message.forms.cbegin(), message.forms.cend(),
                                           [](const QString &form) { return form.isEmpty(); });
            if (message.unfinished || empty) {
                unfinished << message.context + QStringLiteral(": ") + message.source;
            }
        }
        if (!unfinished.isEmpty()) {
            problems << QStringLiteral("%1: %2 untranslated, starting with %3")
                            .arg(file)
                            .arg(unfinished.size())
                            .arg(unfinished.first());
        }
    }
    QVERIFY2(problems.isEmpty(),
             qPrintable(QStringLiteral("these catalogs would show a reader English where their "
                                       "language was promised:\n  ") +
                        problems.join(QStringLiteral("\n  "))));
}

// harbour-salama.ts compiles to the catalog a language without its own falls back to, so
// its plural forms are the English ones; everything else falls back to the source text,
// which is English already.
void tst_translations::sourceCatalogCarriesEnglishPlurals()
{
    const Catalog source = readCatalog(sourceDir() + QStringLiteral("/harbour-salama.ts"));
    QCOMPARE(source.language, QStringLiteral("en"));
    int plurals = 0;
    for (const Message &message : source.messages) {
        if (message.forms.size() < 2) {
            continue;
        }
        ++plurals;
        QCOMPARE(message.forms.size(), 2);
        for (const QString &form : message.forms) {
            QVERIFY2(!form.isEmpty() && !form.contains(QStringLiteral("(s)")),
                     qPrintable(message.source));
        }
    }
    QVERIFY(plurals > 0);
}

void tst_translations::readersLanguageIsInstalled_data()
{
    QTest::addColumn<QString>("locale");
    QTest::addColumn<QString>("catalog");

    QTest::newRow("a language's own") << QStringLiteral("fi_FI") << QStringLiteral("fi");
    QTest::newRow("any country of it") << QStringLiteral("de_AT") << QStringLiteral("de");
    // Taiwan's catalog, not Hong Kong's, though both are Traditional Chinese. Brazil's
    // cannot be a row: a QLocale made from "pt_BR" names plain "pt" first, Portuguese
    // being Brazil's by Qt's likely subtags, where the phone's own locale -- read from
    // LANG, as main() gets it -- names "pt-BR" first.
    QTest::newRow("a country's own") << QStringLiteral("zh_TW") << QStringLiteral("zh_TW");
    QTest::newRow("another country's") << QStringLiteral("zh_HK") << QStringLiteral("zh_HK");
    QTest::newRow("Portugal's") << QStringLiteral("pt_PT") << QStringLiteral("pt");
}

void tst_translations::readersLanguageIsInstalled()
{
    QFETCH(QString, locale);
    QFETCH(QString, catalog);

    m_installed = installTranslations(QCoreApplication::instance(), QLocale(locale), qmDir());
    QVERIFY2(m_installed, qPrintable(QStringLiteral("nothing installed from ") + qmDir()));

    // Every string of the catalog, as the reader of that language is shown it.
    const Catalog expected =
        readCatalog(sourceDir() + QStringLiteral("/harbour-salama-%1.ts").arg(catalog));
    QVERIFY(!expected.messages.isEmpty());
    for (const Message &message : expected.messages) {
        if (message.forms.size() > 1 || message.source.contains(QLatin1String("%n"))) {
            continue;
        }
        QCOMPARE(QCoreApplication::translate(message.context.toUtf8().constData(),
                                             message.source.toUtf8().constData()),
                 message.forms.first());
    }
}

void tst_translations::pluralsAreCountedByTheLanguagesRule()
{
    m_installed = installTranslations(QCoreApplication::instance(),
                                      QLocale(QStringLiteral("pl_PL")), qmDir());
    QVERIFY(m_installed);
    const Catalog polish = readCatalog(sourceDir() + QStringLiteral("/harbour-salama-pl.ts"));
    const Message pages =
        find(polish, QStringLiteral("ClearDataDialog"), QStringLiteral("%n page(s)"));
    QCOMPARE(pages.forms.size(), 3);

    // Polish: one; two to four, but not twelve to fourteen; and the rest.
    const QMap<int, int> formOf{{1, 0}, {2, 1}, {4, 1}, {5, 2}, {12, 2}, {22, 1}, {25, 2}};
    for (auto it = formOf.cbegin(); it != formOf.cend(); ++it) {
        QString want = pages.forms.at(it.value());
        want.replace(QStringLiteral("%n"), QString::number(it.key()));
        QCOMPARE(QCoreApplication::translate("ClearDataDialog", "%n page(s)", nullptr, it.key()),
                 want);
    }
}

void tst_translations::languageWithoutACatalogGetsEnglish()
{
    m_installed = installTranslations(QCoreApplication::instance(),
                                      QLocale(QStringLiteral("ja_JP")), qmDir());
    QVERIFY2(m_installed, "the English source catalog was not installed as the fallback");
    QCOMPARE(QCoreApplication::translate("ClearDataDialog", "%n page(s)", nullptr, 1),
             QStringLiteral("1 page"));
    QCOMPARE(QCoreApplication::translate("ClearDataDialog", "%n page(s)", nullptr, 3),
             QStringLiteral("3 pages"));
    QCOMPARE(QCoreApplication::translate("BrowserMenu", "Settings"), QStringLiteral("Settings"));

    // And with no catalogs at all, nothing is installed, and nothing claims to be.
    QVERIFY(!installTranslations(QCoreApplication::instance(), QLocale(QStringLiteral("ja_JP")),
                                 QDir::tempPath() + QStringLiteral("/no-such-salama-catalogs")));
}

QTEST_GUILESS_MAIN(tst_translations)
#include "tst_translations.moc"
