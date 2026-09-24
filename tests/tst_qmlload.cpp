// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Loads the real QML against tests/silica-stubs and drives it through objectNames.
// The stubs imitate no layout: these tests prove structure and wiring, not appearance.
#include "Core.h"
#include "QmlTypes.h"
#include "engine/EngineMessages.h"
#include "tabs/ClosedTabModel.h"
#include "tabs/TabGroupModel.h"

#include <QColor>
#include <QDesktopServices>
#include <QFile>
#include <QFont>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlExpression>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QScopedPointer>
#include <QSet>
#include <QStyleHints>
#include <QTemporaryDir>
#include <QtTest>
#include <algorithm>
#include <utility>

using Salama::BookmarkModel;
using Salama::Core;
using Salama::EngineMessages;
using Salama::Settings;
using Salama::TabModel;

namespace {

const char *const RootQml = SALAMA_SOURCE_DIR "/qml/harbour-salama.qml";

} // namespace

// Takes the addresses Qt.openUrlExternally is handed for one scheme, in place of the
// platform, which a test has no business starting.
class UrlCatcher : public QObject
{
    Q_OBJECT

public:
    explicit UrlCatcher(QString scheme)
        : m_scheme(std::move(scheme))
    {
        QDesktopServices::setUrlHandler(m_scheme, this, "open");
    }
    ~UrlCatcher() override
    {
        QDesktopServices::unsetUrlHandler(m_scheme);
    }
    UrlCatcher(const UrlCatcher &) = delete;
    UrlCatcher &operator=(const UrlCatcher &) = delete;

    QList<QUrl> opened;

public slots:
    void open(const QUrl &url)
    {
        opened.append(url);
    }

private:
    QString m_scheme;
};

class tst_qmlload : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void rootWindowLoads();
    void addressBarNavigates();
    void omnibar();
    void omnibarFollowsItsSources();
    void omnibarChoices();
    void omnibarForANewTab();
    void navigationBarDrivesWebView();
    void addressShowsHostAndSecurity();
    void barDoesNotCoverThePage();
    void editingEndsWithTheKeyboard();
    void thumbnailCapturedOnLoad();
    void faviconResolvedAfterLoad();
    void tabGrid();
    void gridRowsAreDenseGlass();
    void tabGroups();
    void tabSearch();
    void tabsDropOntoGroups();
    void previewGestures();
    void gridGesturesUnderAFinger();
    void carryToGroupUnderAFinger();
    void carryOverTheStripUnderAFinger();
    void tabGroupStripFades();
    void barReachUnderAFinger();
    void recentlyClosedTabs();
    void pagesBeyondTheLimitUnload();
    void restoredTabsLoadLazily();
    void browserMenu();
    void menuSheetUnderAFinger();
    void findInPage();
    void readerView();
    void downloadsPage();
    void historyPage();
    void bookmarksPage();
    void settingsPage();
    void searchSettingsPage();
    void readerSettingsPage();
    void privacySettingsPage();
    void clearDataDialog();
    void coverSettingsPage();
    void cover();
    void coverFieldFollowsTheFront();
    void coverStyleIsConfigurable();
    void quickActions();
    void quickActionLists();
    void quickActionChoice();
    void quickActionBookmarkFollows();
    void thumbnailCapturedOnLeavingTheApp();
    void pagesSleepOutOfSight();
    void mediaControls();
    void muteOnTheGrid();

private:
    bool loadWindow();
    QObject *find(const QString &name) const;
    QList<QObject *> findAll(const QString &name) const;
    QObject *pageStack() const;
    QObject *currentPage() const;
    QObject *currentWebView() const;
    QVariant evaluate(QObject *scope, const QString &expression) const;
    static void click(QObject *object);
    static void enterKey(QObject *field);
    void typeAddress(const QString &text);
    void tapBar(const QString &region);
    void pullUpToTabs();
    void pullDownToBrowser();
    void popPage() const;
    QObject *openMenuItem(const QString &itemName);

    QScopedPointer<QTemporaryDir> m_dir;
    QScopedPointer<Core> m_core;
    QScopedPointer<QQmlEngine> m_engine;
    QScopedPointer<QObject> m_window;
};

// Software rendering, set before anything loads QtQuick: the offscreen platform has no
// OpenGL, and gridGesturesUnderAFinger() needs a real window to press on.
void tst_qmlload::initTestCase()
{
    QQuickWindow::setSceneGraphBackend(QSGRendererInterface::Software);
}

void tst_qmlload::init()
{
    m_dir.reset(new QTemporaryDir);
    m_core.reset(new Core(m_dir->path(), m_dir->path() + QStringLiteral("/salama.conf"),
                          m_dir->path() + QStringLiteral("/Downloads/Salama")));
    QVERIFY(loadWindow());
}

void tst_qmlload::cleanup()
{
    m_window.reset();
    m_engine.reset();
    m_core.reset();
    m_dir.reset();
}

bool tst_qmlload::loadWindow()
{
    Salama::registerQmlTypes(m_core.data());
    m_engine.reset(new QQmlEngine);
    m_engine->addImportPath(QStringLiteral(SALAMA_STUBS_DIR));
    QQmlComponent component(m_engine.data(), QUrl::fromLocalFile(QLatin1String(RootQml)));
    if (component.isError()) {
        qWarning() << component.errorString();
        return false;
    }
    m_window.reset(component.create());
    return !m_window.isNull() && find(QStringLiteral("browserPage")) != nullptr;
}

namespace {

// The stub page stack destroys popped pages with QML's deferred destroy(); settle it
// before searching so stale pages are not found.
void settle()
{
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
}

// Delegates created by Repeater and ListView have no QObject parent, so the search
// walks the visual item tree as well as QObject children.
QList<QObject *> findObjects(QObject *root, const QString &name)
{
    settle();
    QList<QObject *> found;
    QSet<QObject *> seen;
    QList<QObject *> pending{root};
    while (!pending.isEmpty()) {
        QObject *object = pending.takeLast();
        if (object == nullptr || seen.contains(object)) {
            continue;
        }
        seen.insert(object);
        if (object->objectName() == name) {
            found.append(object);
        }
        // Item views batch model changes until the next frame; there is no frame here.
        if (object->inherits("QQuickItemView")) {
            QMetaObject::invokeMethod(object, "forceLayout");
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        }
        QList<QObject *> children = object->children();
        auto *item = qobject_cast<QQuickItem *>(object);
        if (item != nullptr) {
            for (QQuickItem *child : item->childItems()) {
                children.append(child);
            }
        }
        // Reverse so the traversal keeps document order.
        for (int i = children.count() - 1; i >= 0; --i) {
            pending.append(children.at(i));
        }
    }
    return found;
}

// Delegates in the order their rows are laid out: a list view parents them in the
// order they were made, and a row inserted before another is made after it.
QList<QObject *> byRow(QList<QObject *> items)
{
    std::sort(items.begin(), items.end(), [](QObject *one, QObject *other) {
        return one->property("y").toReal() < other->property("y").toReal();
    });
    return items;
}

} // namespace

QObject *tst_qmlload::find(const QString &name) const
{
    const QList<QObject *> found = findObjects(m_window.data(), name);
    return found.isEmpty() ? nullptr : found.first();
}

QList<QObject *> tst_qmlload::findAll(const QString &name) const
{
    return findObjects(m_window.data(), name);
}

QObject *tst_qmlload::pageStack() const
{
    return m_window->property("pageStack").value<QObject *>();
}

QObject *tst_qmlload::currentPage() const
{
    settle();
    return pageStack()->property("currentPage").value<QObject *>();
}

QObject *tst_qmlload::currentWebView() const
{
    return find(QStringLiteral("browserPage"))->property("currentView").value<QObject *>();
}

QVariant tst_qmlload::evaluate(QObject *scope, const QString &expression) const
{
    QQmlExpression script(qmlContext(scope), scope, expression);
    const QVariant result = script.evaluate();
    if (script.hasError()) {
        qWarning() << script.error().toString();
    }
    return result;
}

void tst_qmlload::click(QObject *object)
{
    QMetaObject::invokeMethod(object, "clicked");
}

void tst_qmlload::enterKey(QObject *field)
{
    auto *attached = field->findChild<QObject *>(QStringLiteral("EnterKeyAttached"));
    QVERIFY2(attached != nullptr, "field has no EnterKey handler");
    QMetaObject::invokeMethod(attached, "clicked");
}

// The address is a label until tapped; editing happens in place. The bar's gesture
// handler owns every press, so a tap is raised the way that handler raises it.
void tst_qmlload::typeAddress(const QString &text)
{
    tapBar(QStringLiteral("address"));
    QObject *field = find(QStringLiteral("addressField"));
    QVERIFY(field->property("visible").toBool());
    field->setProperty("text", text);
    enterKey(field);
}

// A tap on the bar, through the one handler that receives them.
void tst_qmlload::tapBar(const QString &region)
{
    evaluate(find(QStringLiteral("navigationBar")), QStringLiteral("activate('%1')").arg(region));
}

// Dragging the navigation bar upwards is what opens the tab grid, and dragging the
// grid past its own top is what closes it again. The drags themselves need a window;
// what the bar and the grid report while one is under way is a distance, and these
// raise the distances of a gesture that goes all the way.
void tst_qmlload::pullUpToTabs()
{
    QObject *bar = find(QStringLiteral("navigationBar"));
    const qreal distance =
        find(QStringLiteral("browserPage"))->property("pullThreshold").toReal() + 1;
    evaluate(bar, QStringLiteral("dragStarted()"));
    evaluate(bar, QStringLiteral("dragMoved(%1)").arg(distance));
    evaluate(bar, QStringLiteral("dragFinished(%1)").arg(distance));
}

void tst_qmlload::pullDownToBrowser()
{
    QObject *grid = find(QStringLiteral("tabsView"));
    const qreal distance =
        find(QStringLiteral("browserPage"))->property("pullThreshold").toReal() + 1;
    evaluate(grid, QStringLiteral("pullStarted()"));
    evaluate(grid, QStringLiteral("pulled(%1)").arg(distance));
    evaluate(grid, QStringLiteral("pullFinished(%1)").arg(distance));
}

void tst_qmlload::popPage() const
{
    QVariant result;
    QMetaObject::invokeMethod(pageStack(), "pop", Q_RETURN_ARG(QVariant, result),
                              Q_ARG(QVariant, QVariant()), Q_ARG(QVariant, QVariant()));
}

// The bar's menu button brings up the sheet of icons; one of them, tapped.
QObject *tst_qmlload::openMenuItem(const QString &itemName)
{
    tapBar(QStringLiteral("menu"));
    QObject *item = find(itemName);
    if (item == nullptr || !find(QStringLiteral("browserMenu"))->property("open").toBool()) {
        return nullptr;
    }
    click(item);
    return currentPage();
}

void tst_qmlload::rootWindowLoads()
{
    QCOMPARE(currentPage()->objectName(), QStringLiteral("browserPage"));
    QCOMPARE(m_core->tabs()->count(), 1);
    QCOMPARE(m_core->tabs()->activeUrl(), Settings::defaultHomePage());

    QObject *webView = find(QStringLiteral("webView"));
    QVERIFY(webView != nullptr);
    QCOMPARE(currentWebView(), webView);
    QCOMPARE(webView->property("url").toUrl().toString(), Settings::defaultHomePage());
    // The engine is told to lay pages out larger than the platform's own default.
    // The engine is told to lay pages out larger than the platform's own default of
    // 1.5 * Theme.pixelRatio.
    QObject *page = find(QStringLiteral("browserPage"));
    const qreal zoom = evaluate(page, QStringLiteral("pageZoom()")).toReal();
    QVERIFY(zoom > 1.5 * evaluate(page, QStringLiteral("Theme.pixelRatio")).toReal() - 0.5);
    // What the engine was given is read through the view: BrowserPage.qml made it, so
    // it carries that file's Sailfish.WebEngine import, which the page's own context
    // does not.
    QCOMPARE(evaluate(webView, QStringLiteral("WebEngineSettings.pixelRatio")).toReal(), zoom);
    QVERIFY(webView->property("downloadsEnabled").toBool());
    // Downloads are saved to the application's own folder, without the engine asking
    // where.
    QCOMPARE(evaluate(webView, QStringLiteral("WebEngineSettings.downloadDir")).toString(),
             m_core->downloads()->directory());
    QVERIFY(evaluate(webView, QStringLiteral("WebEngineSettings.useDownloadDir")).toBool());
    QVERIFY(!webView->property("desktopMode").toBool());

    // The engine is given its tracking protection on start, at the level Settings
    // holds: Standard, until it is changed.
    const QVariantList given =
        evaluate(find(QStringLiteral("viewArea")), QStringLiteral("WebEngineSettings.preferences"))
            .toList();
    const QVariantList standard =
        EngineMessages::trackingProtectionPreferences(Settings::TrackingProtectionStandard);
    QCOMPARE(given.count(), standard.count());
    for (int i = 0; i < given.count(); ++i) {
        QCOMPARE(given.at(i).toMap().value(QStringLiteral("key")),
                 standard.at(i).toMap().value(QStringLiteral("name")));
        QCOMPARE(given.at(i).toMap().value(QStringLiteral("value")),
                 standard.at(i).toMap().value(QStringLiteral("value")));
    }

    // The engine reporting the first url is the first visit.
    QCOMPARE(m_core->history()->count(), 1);
    // The bar carries the host, not the whole url.
    QCOMPARE(find(QStringLiteral("addressLabel"))->property("text").toString(),
             QStringLiteral("qwant.com"));
    QVERIFY(!find(QStringLiteral("addressField"))->property("visible").toBool());
}

void tst_qmlload::addressBarNavigates()
{
    QObject *webView = currentWebView();

    typeAddress(QStringLiteral("example.org"));
    QCOMPARE(webView->property("url").toUrl().toString(), QStringLiteral("https://example.org"));
    QCOMPARE(m_core->tabs()->activeUrl(), QStringLiteral("https://example.org"));
    QCOMPARE(m_core->history()->count(), 2);
    // Editing ends with the field hidden and the label showing the page again.
    QVERIFY(!find(QStringLiteral("addressField"))->property("visible").toBool());
    QCOMPARE(find(QStringLiteral("addressLabel"))->property("text").toString(),
             QStringLiteral("example.org"));
    // Editing gets every character of it back.
    tapBar(QStringLiteral("address"));
    QCOMPARE(find(QStringLiteral("addressField"))->property("text").toString(),
             QStringLiteral("https://example.org"));
    evaluate(find(QStringLiteral("navigationBar")), QStringLiteral("endEditing()"));

    const QUrl searchUrl(QStringLiteral("https://www.qwant.com/?q=sailfish%20os"));
    typeAddress(QStringLiteral("sailfish os"));
    QCOMPARE(webView->property("url").toUrl(), searchUrl);

    typeAddress(QString());
    QCOMPARE(webView->property("url").toUrl(), searchUrl);
    QCOMPARE(m_core->tabs()->count(), 1);
}

namespace {

QString textIn(QObject *item, const QString &name)
{
    return findObjects(item, name).first()->property("text").toString();
}

bool shownIn(QObject *item, const QString &name)
{
    return findObjects(item, name).first()->property("visible").toBool();
}

QList<QObject *> omnibarRows(QObject *root)
{
    return byRow(findObjects(root, QStringLiteral("omnibarResult")));
}

// The headings over the omnibar's sections, top to bottom. A list view keeps a few it
// no longer shows, hidden, to use again.
QStringList omnibarHeadings(QObject *root)
{
    QStringList texts;
    for (QObject *heading : byRow(findObjects(root, QStringLiteral("omnibarSection")))) {
        if (heading->property("visible").toBool()) {
            texts.append(heading->property("text").toString());
        }
    }
    return texts;
}

// Typed into the address bar, which is opened for it first if it is not, and the
// debounce run out: what is typed is looked for at once.
void typeIntoBar(QObject *root, const QString &text)
{
    QObject *bar = findObjects(root, QStringLiteral("navigationBar")).first();
    if (!bar->property("editing").toBool()) {
        QQmlExpression(qmlContext(bar), bar, QStringLiteral("activate('address')")).evaluate();
    }
    findObjects(root, QStringLiteral("addressField")).first()->setProperty("text", text);
    QMetaObject::invokeMethod(findObjects(root, QStringLiteral("omnibarDebounce")).first(),
                              "triggered");
}

void observeDownload(Core *core, const QVariantMap &message)
{
    core->downloads()->observe(core->downloads()->topic(), message);
}

// What the omnibar's tests look for: something of every kind with "forest" in it. The
// tab in front has it, and is the one never listed; one tab is beside it in its group
// and one in another group; a bookmark, a page of the history -- the only one: the
// tabs' own visits are cleared -- and a download on its way.
struct Forest
{
    int front = 0;
    int near = 0;
    int work = 0;
    int away = 0;
};

Forest plantForest(Core *core)
{
    TabModel *tabs = core->tabs();
    Forest forest;
    forest.front = tabs->activeTabId();
    tabs->updateTitle(forest.front, QStringLiteral("Forest front"));
    forest.near = tabs->newTab(QStringLiteral("https://forest.example/near"));
    tabs->updateTitle(forest.near, QStringLiteral("Forest near"));
    forest.work = tabs->addGroup(QStringLiteral("Work"));
    forest.away = tabs->newTab(QStringLiteral("https://forest.example/work"));
    tabs->updateTitle(forest.away, QStringLiteral("Forest work"));
    tabs->activateTabById(forest.front);
    core->history()->clear();
    core->bookmarks()->add(QStringLiteral("https://forest.example/wiki"),
                           QStringLiteral("Forest wiki"));
    core->history()->visit(QStringLiteral("https://forest.example/blog"),
                           QStringLiteral("Forest blog"));
    observeDownload(core, {{QStringLiteral("msg"), QStringLiteral("dl-start")},
                           {QStringLiteral("id"), 1},
                           {QStringLiteral("displayName"), QStringLiteral("forest-map.pdf")},
                           {QStringLiteral("sourceUrl"),
                            QStringLiteral("https://files.example/forest-map.pdf")},
                           {QStringLiteral("targetPath"), QStringLiteral("/tmp/forest-map.pdf")},
                           {QStringLiteral("mimeType"), QStringLiteral("application/pdf")},
                           {QStringLiteral("size"), 2048}});
    observeDownload(core, {{QStringLiteral("msg"), QStringLiteral("dl-progress")},
                           {QStringLiteral("id"), 1},
                           {QStringLiteral("percent"), 40}});
    return forest;
}

} // namespace

// The address bar is an omnibar (docs/DECISIONS/0027-omnibar.md): typed into, it brings
// up a pane above itself with what the words find among the tabs of every group, the
// bookmarks, the history and the downloads, and below them the rows that go to the
// address or search for the words. What the model finds, and in what order, is
// tst_omnibarmodel's; this is the pane and the bar under it.
void tst_qmlload::omnibar()
{
    const Forest forest = plantForest(m_core.data());
    QObject *root = m_window.data();
    QObject *bar = find(QStringLiteral("navigationBar"));
    QObject *field = find(QStringLiteral("addressField"));
    QObject *pane = find(QStringLiteral("omnibarView"));
    QObject *gesture = find(QStringLiteral("navigationBarGesture"));
    QObject *searchAction = find(QStringLiteral("omnibarSearchAction"));
    const QString home = m_core->tabs()->activeUrl();
    const int keepFocus = evaluate(bar, QStringLiteral("FocusBehavior.KeepFocus")).toInt();
    const int clearFocus = evaluate(bar, QStringLiteral("FocusBehavior.ClearItemFocus")).toInt();
    QVERIFY(keepFocus != clearFocus);

    // Declared after both bars, so nothing of theirs is drawn over it.
    auto *paneItem = qobject_cast<QQuickItem *>(pane);
    const QList<QQuickItem *> layer = paneItem->parentItem()->childItems();
    QVERIFY(layer.indexOf(paneItem) > layer.indexOf(qobject_cast<QQuickItem *>(bar)));
    QVERIFY(layer.indexOf(paneItem) >
            layer.indexOf(qobject_cast<QQuickItem *>(find(QStringLiteral("findBar")))));

    // The field opens with the page's address, which is nothing to look for: no pane,
    // and the field and the reach as they always were.
    tapBar(QStringLiteral("address"));
    QCOMPARE(bar->property("typedText").toString(), home);
    QVERIFY(!bar->property("edited").toBool());
    QVERIFY(!bar->property("paneUp").toBool());
    QVERIFY(!pane->property("visible").toBool());
    QCOMPARE(field->property("focusOutBehavior").toInt(), clearFocus);
    QVERIFY(gesture->property("reach").toReal() > 0);

    // Typed into, the pane is up: the field keeps its focus through presses on it, and
    // the reach above the bar is the pane's. The rows that go and search follow the
    // text at once -- words are nothing to go to -- and what is found waits for the
    // debounce.
    field->setProperty("text", QStringLiteral("forest"));
    QVERIFY(bar->property("paneUp").toBool());
    QVERIFY(pane->property("visible").toBool());
    QCOMPARE(field->property("focusOutBehavior").toInt(), keepFocus);
    QCOMPARE(gesture->property("reach").toReal(), qreal(0));
    QCOMPARE(gesture->property("height").toReal(), bar->property("height").toReal());
    QVERIFY(searchAction->property("visible").toBool());
    const QString engine =
        m_core->settings()->searchEngineNames().at(m_core->settings()->searchEngineIndex());
    QCOMPARE(textIn(searchAction, QStringLiteral("omnibarActionTitle")),
             QStringLiteral("Search %1 for “forest”").arg(engine));
    QVERIFY(!find(QStringLiteral("omnibarGoAction"))->property("visible").toBool());
    QCOMPARE(m_core->omnibar()->query(), QString());
    QVERIFY(omnibarRows(root).isEmpty());
    QObject *debounce = find(QStringLiteral("omnibarDebounce"));
    QCOMPARE(debounce->property("interval").toInt(), 150);
    QMetaObject::invokeMethod(debounce, "triggered");
    QCOMPARE(m_core->omnibar()->query(), QStringLiteral("forest"));

    // A section of each, in this order, each saying how many; the tab in front is not
    // among them. A tab in another group says which, one in the group in front only
    // where it is; a download, where it came from and how far along it is.
    QCOMPARE(omnibarHeadings(root),
             QStringList({QStringLiteral("Tabs (2)"), QStringLiteral("Bookmarks (1)"),
                          QStringLiteral("History (1)"), QStringLiteral("Downloads (1)")}));
    const QList<QObject *> rows = omnibarRows(root);
    QCOMPARE(rows.count(), 5);
    const QStringList titles{QStringLiteral("Forest work"), QStringLiteral("Forest near"),
                             QStringLiteral("Forest wiki"), QStringLiteral("Forest blog"),
                             QStringLiteral("forest-map.pdf")};
    const QStringList details{QStringLiteral("Work · forest.example"),
                              QStringLiteral("forest.example"),
                              QStringLiteral("https://forest.example/wiki"),
                              QStringLiteral("https://forest.example/blog"),
                              QStringLiteral("files.example · Downloading, 40%")};
    // Without a site's own icon, a glyph of the platform's; and a date for the kinds
    // that have one.
    const QStringList glyphs{QStringLiteral("icon-m-tabs"), QStringLiteral("icon-m-tabs"),
                             QStringLiteral("icon-m-favorite"), QStringLiteral("icon-m-history"),
                             QStringLiteral("icon-m-downloads")};
    for (int i = 0; i < rows.count(); ++i) {
        QVERIFY(evaluate(rows.at(i), QStringLiteral("model.tabId")).toInt() != forest.front);
        QCOMPARE(textIn(rows.at(i), QStringLiteral("omnibarResultTitle")), titles.at(i));
        QCOMPARE(textIn(rows.at(i), QStringLiteral("omnibarResultDetail")), details.at(i));
        QObject *glyph = findObjects(rows.at(i), QStringLiteral("omnibarResultGlyph")).first();
        QVERIFY(glyph->property("visible").toBool());
        QCOMPARE(glyph->property("source").toUrl(),
                 QUrl(QStringLiteral("image://theme/") + glyphs.at(i)));
        QCOMPARE(shownIn(rows.at(i), QStringLiteral("omnibarResultDate")), i >= 3);
        QCOMPARE(shownIn(rows.at(i), QStringLiteral("omnibarResultProgress")), i == 4);
    }
    QVERIFY(!textIn(rows.at(3), QStringLiteral("omnibarResultDate")).isEmpty());

    // The list is as tall as what it holds and hangs from the rows that go and search,
    // which sit on the bar; it never makes an item current, which would take the focus.
    auto *results = qobject_cast<QQuickItem *>(find(QStringLiteral("omnibarResults")));
    auto *actions = qobject_cast<QQuickItem *>(find(QStringLiteral("omnibarActions")));
    QCOMPARE(results->property("currentIndex").toInt(), -1);
    QCOMPARE(results->y() + results->height(), actions->y());
    QCOMPARE(actions->y() + actions->height(), paneItem->height());
    QVERIFY(results->height() < actions->y());

    // Neither the keyboard closing nor the field's focus going ends the edit while the
    // pane is up: the list is scrolled with the keyboard put away. The field lets its
    // focus go with the keyboard, so that a tap on it brings the keyboard back.
    QVERIFY(field->property("focus").toBool());
    evaluate(bar, QStringLiteral("keyboardVisibilityChanged(false)"));
    QVERIFY(!field->property("focus").toBool());
    evaluate(bar, QStringLiteral("focusChanged(false)"));
    QMetaObject::invokeMethod(results, "dragStarted");
    QVERIFY(bar->property("editing").toBool());
    QVERIFY(pane->property("visible").toBool());

    // Typed back to the address it opened with, the pane goes, and the model is asked
    // for nothing; the bar is as it was, and the keyboard closing ends the edit again.
    field->setProperty("text", home);
    QVERIFY(bar->property("editing").toBool());
    QVERIFY(!pane->property("visible").toBool());
    QCOMPARE(m_core->omnibar()->query(), QString());
    QCOMPARE(m_core->omnibar()->count(), 0);
    QCOMPARE(field->property("focusOutBehavior").toInt(), clearFocus);
    QVERIFY(gesture->property("reach").toReal() > 0);
    evaluate(bar, QStringLiteral("keyboardVisibilityChanged(false)"));
    QVERIFY(!bar->property("editing").toBool());
}

// What the omnibar lists follows its sources while it is up: a row comes and goes as
// they change, a download's progress and a tab's icon change a row in place rather
// than making it again under the finger, and a section that found more than it shows
// says so.
void tst_qmlload::omnibarFollowsItsSources()
{
    const Forest forest = plantForest(m_core.data());
    QObject *root = m_window.data();
    typeIntoBar(root, QStringLiteral("forest"));
    QCOMPARE(omnibarRows(root).count(), 5);

    m_core->bookmarks()->add(QStringLiteral("https://forest.example/camp"),
                             QStringLiteral("Forest camp"));
    QTRY_COMPARE(omnibarHeadings(root).value(1), QStringLiteral("Bookmarks (2)"));
    const QPointer<QObject> map = omnibarRows(root).last();
    observeDownload(m_core.data(), {{QStringLiteral("msg"), QStringLiteral("dl-progress")},
                                    {QStringLiteral("id"), 1},
                                    {QStringLiteral("percent"), 60}});
    QTRY_COMPARE(textIn(omnibarRows(root).last(), QStringLiteral("omnibarResultDetail")),
                 QStringLiteral("files.example · Downloading, 60%"));
    QVERIFY(!map.isNull());
    QCOMPARE(omnibarRows(root).last(), map.data());

    // A site's own icon once it has one that loads, and the glyph again when it fails.
    TabModel *tabs = m_core->tabs();
    const QPointer<QObject> near = omnibarRows(root).at(1);
    tabs->updateFavicon(forest.near, QUrl::fromLocalFile(QStringLiteral(SALAMA_SOURCE_DIR
                                                                        "/art/harbour-salama.png"))
                                         .toString());
    QTRY_VERIFY(shownIn(omnibarRows(root).at(1), QStringLiteral("omnibarResultFavicon")));
    QVERIFY(!shownIn(omnibarRows(root).at(1), QStringLiteral("omnibarResultGlyph")));
    tabs->updateFavicon(
        forest.near, QUrl::fromLocalFile(m_dir->path() + QStringLiteral("/none.png")).toString());
    QTRY_VERIFY(shownIn(omnibarRows(root).at(1), QStringLiteral("omnibarResultGlyph")));
    QVERIFY(!shownIn(omnibarRows(root).at(1), QStringLiteral("omnibarResultFavicon")));
    QVERIFY(!near.isNull());
    QCOMPARE(omnibarRows(root).at(1), near.data());
    tabs->updateFavicon(forest.near, QString());

    for (int i = 0; i < 11; ++i) {
        m_core->history()->visit(QStringLiteral("https://forest.example/post/%1").arg(i),
                                 QStringLiteral("Forest post %1").arg(i));
    }
    QTRY_COMPARE(omnibarHeadings(root).value(2), QStringLiteral("History (10 of 12)"));
    evaluate(find(QStringLiteral("navigationBar")), QStringLiteral("endEditing()"));
    QCOMPARE(m_core->omnibar()->query(), QString());
    QCOMPARE(m_core->omnibar()->count(), 0);
}

// Each thing the omnibar lists does its own thing when chosen, and so does each row
// above the bar; every one of them ends the edit.
void tst_qmlload::omnibarChoices()
{
    const Forest forest = plantForest(m_core.data());
    QObject *root = m_window.data();
    TabModel *tabs = m_core->tabs();
    QObject *bar = find(QStringLiteral("navigationBar"));
    QObject *webView = currentWebView();
    const int open = tabs->count();

    // A tab comes to the front, and its group with it.
    typeIntoBar(root, QStringLiteral("forest work"));
    QCOMPARE(omnibarRows(root).count(), 1);
    click(omnibarRows(root).first());
    QVERIFY(!bar->property("editing").toBool());
    QVERIFY(!find(QStringLiteral("omnibarView"))->property("visible").toBool());
    QCOMPARE(tabs->activeTabId(), forest.away);
    QCOMPARE(tabs->currentGroupId(), forest.work);
    QVERIFY(tabs->activateTabById(forest.front));
    QCOMPARE(currentWebView(), webView);

    // A bookmark and a page of the history open in the tab in front.
    typeIntoBar(root, QStringLiteral("forest wiki"));
    QCOMPARE(omnibarHeadings(root), QStringList{QStringLiteral("Bookmarks (1)")});
    click(omnibarRows(root).first());
    QVERIFY(!bar->property("editing").toBool());
    QCOMPARE(webView->property("url").toUrl(), QUrl(QStringLiteral("https://forest.example/wiki")));
    typeIntoBar(root, QStringLiteral("forest blog"));
    QCOMPARE(omnibarHeadings(root), QStringList{QStringLiteral("History (1)")});
    click(omnibarRows(root).first());
    QCOMPARE(webView->property("url").toUrl(), QUrl(QStringLiteral("https://forest.example/blog")));
    QCOMPARE(tabs->count(), open);
    QCOMPARE(tabs->activeTabId(), forest.front);

    // A download on its way is shown in the list of downloads; one that has arrived
    // opens its file.
    typeIntoBar(root, QStringLiteral("forest map"));
    click(omnibarRows(root).first());
    QVERIFY(!bar->property("editing").toBool());
    QCOMPARE(currentPage()->objectName(), QStringLiteral("downloadsPage"));
    popPage();
    observeDownload(m_core.data(),
                    {{QStringLiteral("msg"), QStringLiteral("dl-done")},
                     {QStringLiteral("id"), 1},
                     {QStringLiteral("targetPath"), QStringLiteral("/tmp/forest-map.pdf")}});
    const UrlCatcher files(QStringLiteral("file"));
    typeIntoBar(root, QStringLiteral("forest map"));
    QObject *arrived = omnibarRows(root).first();
    QCOMPARE(textIn(arrived, QStringLiteral("omnibarResultDetail")),
             QStringLiteral("files.example"));
    QVERIFY(!shownIn(arrived, QStringLiteral("omnibarResultProgress")));
    click(arrived);
    QCOMPARE(files.opened, QList<QUrl>{QUrl::fromLocalFile(QStringLiteral("/tmp/forest-map.pdf"))});
    QCOMPARE(currentPage()->objectName(), QStringLiteral("browserPage"));

    // An address can be gone to, with the address it makes under it; and it can be
    // searched for all the same.
    QObject *goAction = find(QStringLiteral("omnibarGoAction"));
    typeIntoBar(root, QStringLiteral("forest.example/path"));
    QVERIFY(goAction->property("visible").toBool());
    QCOMPARE(textIn(goAction, QStringLiteral("omnibarActionTitle")),
             QStringLiteral("Go to forest.example/path"));
    QCOMPARE(textIn(goAction, QStringLiteral("omnibarActionSubtitle")),
             QStringLiteral("https://forest.example/path"));
    click(goAction);
    QVERIFY(!bar->property("editing").toBool());
    QCOMPARE(webView->property("url").toUrl(), QUrl(QStringLiteral("https://forest.example/path")));
    typeIntoBar(root, QStringLiteral("forest.example"));
    QVERIFY(find(QStringLiteral("omnibarGoAction"))->property("visible").toBool());
    click(find(QStringLiteral("omnibarSearchAction")));
    QCOMPARE(webView->property("url").toUrl(),
             QUrl(m_core->settings()->searchUrl(QStringLiteral("forest.example"))));
    QCOMPARE(tabs->count(), open);
}

// Opened for a new tab -- the cover's search -- the field is empty and the pane is up at
// once over the whole page, listing the bookmarks, as sailfish-browser's new-tab overlay
// lists its favourites; no tab is made until something is chosen, and what is chosen
// opens in one.
void tst_qmlload::omnibarForANewTab()
{
    const Forest forest = plantForest(m_core.data());
    QObject *root = m_window.data();
    TabModel *tabs = m_core->tabs();
    QObject *page = find(QStringLiteral("browserPage"));
    QObject *bar = find(QStringLiteral("navigationBar"));
    QObject *field = find(QStringLiteral("addressField"));
    QObject *pane = find(QStringLiteral("omnibarView"));
    auto *paneItem = qobject_cast<QQuickItem *>(pane);
    const int open = tabs->count();

    evaluate(page, QStringLiteral("openOmnibar(true)"));
    QVERIFY(bar->property("editing").toBool());
    QVERIFY(bar->property("forNewTab").toBool());
    QCOMPARE(field->property("text").toString(), QString());
    QCOMPARE(field->property("placeholderText").toString(),
             QStringLiteral("Search or enter address"));
    QVERIFY(pane->property("visible").toBool());
    QVERIFY(m_core->omnibar()->bookmarksWhenEmpty());
    QCOMPARE(omnibarHeadings(root), QStringList{QStringLiteral("Bookmarks (1)")});
    QCOMPARE(omnibarRows(root).count(), 1);
    QVERIFY(!find(QStringLiteral("omnibarGoAction"))->property("visible").toBool());
    QVERIFY(!find(QStringLiteral("omnibarSearchAction"))->property("visible").toBool());
    QCOMPARE(paneItem->y(), find(QStringLiteral("viewArea"))->property("y").toReal());
    QCOMPARE(paneItem->y() + paneItem->height(), bar->property("y").toReal());
    QCOMPARE(find(QStringLiteral("navigationBarGesture"))->property("reach").toReal(), qreal(0));

    // A tap on the bare glass, in a window where a press goes to whatever is drawn at
    // the point, puts it away having made nothing.
    {
        auto *window = qobject_cast<QQuickItem *>(root);
        QQuickWindow host;
        host.resize(int(window->width()), int(window->height()));
        window->setParentItem(host.contentItem());
        host.show();
        const bool exposed = QTest::qWaitForWindowExposed(&host);
        const QPointF bare =
            paneItem->mapToScene(QPointF(paneItem->width() / 2, paneItem->height() / 4));
        QTest::mouseClick(&host, Qt::LeftButton, Qt::NoModifier, bare.toPoint());
        window->setParentItem(nullptr);
        QVERIFY(exposed);
    }
    QVERIFY(!bar->property("editing").toBool());
    QVERIFY(!pane->property("visible").toBool());
    QVERIFY(!m_core->omnibar()->bookmarksWhenEmpty());
    QCOMPARE(tabs->count(), open);
    QCOMPARE(tabs->activeTabId(), forest.front);

    // What is chosen opens in a tab of its own, and the page behind it stays as it was;
    // Enter does the same. A tab found is only brought to the front.
    const QString behind = tabs->activeUrl();
    evaluate(page, QStringLiteral("openOmnibar(true)"));
    click(omnibarRows(root).first());
    QCOMPARE(tabs->count(), open + 1);
    QCOMPARE(tabs->activeUrl(), QStringLiteral("https://forest.example/wiki"));
    tabs->activateTabById(forest.front);
    QCOMPARE(tabs->activeUrl(), behind);
    evaluate(page, QStringLiteral("openOmnibar(true)"));
    field->setProperty("text", QStringLiteral("example.org"));
    enterKey(field);
    QCOMPARE(tabs->count(), open + 2);
    QCOMPARE(tabs->activeUrl(), QStringLiteral("https://example.org"));
    evaluate(page, QStringLiteral("openOmnibar(true)"));
    typeIntoBar(root, QStringLiteral("forest work"));
    click(omnibarRows(root).first());
    QCOMPARE(tabs->count(), open + 2);
    QCOMPARE(tabs->activeTabId(), forest.away);

    // The menu sheet, the find bar and the grid are put away for it; the menu opened,
    // or the grid pulled up, ends it.
    QObject *menu = find(QStringLiteral("browserMenu"));
    QObject *findBar = find(QStringLiteral("findBar"));
    tapBar(QStringLiteral("menu"));
    QVERIFY(menu->property("open").toBool());
    evaluate(page, QStringLiteral("openOmnibar(true)"));
    QVERIFY(!menu->property("open").toBool());
    QVERIFY(pane->property("visible").toBool());
    tapBar(QStringLiteral("menu"));
    QVERIFY(!bar->property("editing").toBool());
    click(find(QStringLiteral("findMenuButton")));
    QVERIFY(findBar->property("active").toBool());
    evaluate(page, QStringLiteral("openOmnibar(true)"));
    QVERIFY(!findBar->property("active").toBool());
    QVERIFY(pane->property("visible").toBool());
    pullUpToTabs();
    QVERIFY(page->property("tabsOpen").toBool());
    QVERIFY(!bar->property("editing").toBool());
    QVERIFY(!pane->property("visible").toBool());
    evaluate(page, QStringLiteral("openOmnibar(true)"));
    QVERIFY(!page->property("tabsOpen").toBool());
    QVERIFY(pane->property("visible").toBool());
}

void tst_qmlload::navigationBarDrivesWebView()
{
    QObject *webView = currentWebView();
    QObject *bar = find(QStringLiteral("navigationBar"));

    // Every control on the bar is reached by the region a press lands in, and the
    // regions tile it: each boundary is asserted against the item itself, because a
    // region no press can land in is exactly how the first gesture handler failed.
    const qreal barWidth = bar->property("width").toReal();
    QVERIFY(barWidth > 0);
    QCOMPARE(evaluate(bar, QStringLiteral("regionAt(0)")).toString(), QStringLiteral("back"));
    QCOMPARE(evaluate(bar, QStringLiteral("regionAt(width / 2)")).toString(),
             QStringLiteral("address"));
    QCOMPARE(evaluate(bar, QStringLiteral("regionAt(width - 1)")).toString(),
             QStringLiteral("menu"));
    const qreal reloadX = find(QStringLiteral("reloadButton"))->property("x").toReal();
    QVERIFY(reloadX > 0);
    QCOMPARE(evaluate(bar, QStringLiteral("regionAt(%1)").arg(reloadX + 1)).toString(),
             QStringLiteral("reload"));

    // Back is ignored until there is somewhere to go back to.
    tapBar(QStringLiteral("back"));
    QVERIFY(webView->property("calls").toStringList().isEmpty());
    webView->setProperty("canGoBack", true);
    tapBar(QStringLiteral("back"));
    tapBar(QStringLiteral("reload"));
    QCOMPARE(webView->property("calls").toStringList(),
             QStringList({QStringLiteral("goBack"), QStringLiteral("reload")}));

    // The address is centred on the screen rather than in the room left between the
    // controls, and it never reaches either of them.
    QObject *addressLabel = find(QStringLiteral("addressLabel"));
    const qreal centred = bar->property("centredWidth").toReal();
    QVERIFY(centred > 0);
    QVERIFY(centred <= barWidth - 2 * (barWidth - reloadX));
    QVERIFY(addressLabel->property("width").toReal() <= centred);

    QObject *progress = find(QStringLiteral("loadProgress"));
    QVERIFY(!progress->property("visible").toBool());
    webView->setProperty("loading", true);
    webView->setProperty("loadProgress", 50);
    QVERIFY(progress->property("visible").toBool());
    // The same region stops a load that is running.
    tapBar(QStringLiteral("reload"));
    QCOMPARE(webView->property("calls").toStringList().last(), QStringLiteral("stop"));
    webView->setProperty("loading", false);

    // While the address is being edited the bar belongs to the field: back and
    // reload are not drawn, the room they had is the field's, and the text inside it
    // is inset by a padding rather than by a page margin. The field was half the bar
    // wide with a page margin at each end of it, twice over.
    QObject *field = find(QStringLiteral("addressField"));
    const qreal pageMargin = evaluate(bar, QStringLiteral("Theme.horizontalPageMargin")).toReal();
    const qreal menuX = find(QStringLiteral("menuButton"))->property("x").toReal();
    tapBar(QStringLiteral("address"));
    QVERIFY(field->property("visible").toBool());
    QVERIFY(!find(QStringLiteral("backButton"))->property("visible").toBool());
    QVERIFY(!find(QStringLiteral("reloadButton"))->property("visible").toBool());
    QCOMPARE(evaluate(bar, QStringLiteral("regionAt(0)")).toString(), QStringLiteral("address"));
    QCOMPARE(evaluate(bar, QStringLiteral("regionAt(%1)").arg(reloadX + 1)).toString(),
             QStringLiteral("address"));
    const qreal fieldLeft = field->property("x").toReal();
    const qreal fieldRight = fieldLeft + field->property("width").toReal();
    QVERIFY(fieldLeft < pageMargin);
    QVERIFY(fieldRight <= menuX);
    QVERIFY(menuX - fieldRight < pageMargin);
    QVERIFY(field->property("textLeftMargin").toReal() < pageMargin);
    QVERIFY(field->property("textRightMargin").toReal() < pageMargin);
    // The field is drawn at the size the host is, and both are larger than the small
    // text Silica puts in a label.
    const int hostSize = addressLabel->property("font").value<QFont>().pixelSize();
    QCOMPARE(field->property("font").value<QFont>().pixelSize(), hostSize);
    QVERIFY(hostSize > evaluate(bar, QStringLiteral("Theme.fontSizeSmall")).toInt());
    evaluate(bar, QStringLiteral("endEditing()"));
    QVERIFY(find(QStringLiteral("backButton"))->property("visible").toBool());

    // The bar reports how far it has been dragged and the page decides. The deck
    // follows the finger while it moves, and a drag that stops short springs back:
    // a gesture that shows nothing until it fires cannot be told apart, on device,
    // from the system's own edge swipe having taken the touch.
    QObject *page = find(QStringLiteral("browserPage"));
    const qreal threshold = page->property("pullThreshold").toReal();
    QVERIFY(threshold > 0);
    evaluate(bar, QStringLiteral("dragStarted()"));
    evaluate(bar, QStringLiteral("dragMoved(%1)").arg(threshold / 2));
    QCOMPARE(page->property("tabsOffset").toReal(), threshold / 2);
    evaluate(bar, QStringLiteral("dragFinished(%1)").arg(threshold / 2));
    QVERIFY(!page->property("tabsOpen").toBool());

    // The handler must cover the bar: the first one sat behind the controls, which
    // tile it, so no press ever reached it and the gesture could not be made. It also
    // reaches above the bar, to give the drag somewhere to start that the system's own
    // bottom-edge swipe has not already taken.
    QObject *gesture = find(QStringLiteral("navigationBarGesture"));
    QVERIFY(gesture->property("enabled").toBool());
    QCOMPARE(gesture->property("width").toReal(), barWidth);
    const qreal reach = gesture->property("reach").toReal();
    QVERIFY(reach > 0);
    QCOMPARE(gesture->property("height").toReal(), bar->property("height").toReal() + reach);

    pullUpToTabs();
    QVERIFY(page->property("tabsOpen").toBool());
    // The grid is not a page: nothing was pushed, and there is nothing to come back
    // from. It is the same page, further down.
    QCOMPARE(currentPage()->objectName(), QStringLiteral("browserPage"));
}

// The address is shown short, and a connection the engine is unhappy with is drawn
// on it. The warning is only for pages that claimed to be secure in the first place.
void tst_qmlload::addressShowsHostAndSecurity()
{
    QObject *bar = find(QStringLiteral("navigationBar"));
    QObject *warning = find(QStringLiteral("securityWarning"));
    QObject *webView = currentWebView();
    QVERIFY(!warning->property("visible").toBool());
    QVERIFY(!bar->property("tlsBroken").toBool());

    auto *security = webView->property("security").value<QObject *>();
    QVERIFY(security != nullptr);
    security->setProperty("allGood", false);
    QVERIFY(bar->property("tlsBroken").toBool());
    QVERIFY(warning->property("visible").toBool());

    // Not while the address is being edited: the field then shows the whole url,
    // which says more than any icon can.
    tapBar(QStringLiteral("address"));
    QVERIFY(!warning->property("visible").toBool());
    evaluate(bar, QStringLiteral("endEditing()"));
    QVERIFY(warning->property("visible").toBool());

    // No verdict for this page -- the engine has not judged it -- says nothing either,
    // which is the same pair sailfish-browser reads.
    security->setProperty("validState", false);
    QVERIFY(!bar->property("tlsBroken").toBool());
    security->setProperty("validState", true);
    QVERIFY(bar->property("tlsBroken").toBool());

    // An engine build that hands out no security object at all says nothing.
    webView->setProperty("security", QVariant::fromValue<QObject *>(nullptr));
    QVERIFY(!bar->property("tlsBroken").toBool());

    // A page served over plain http is not broken TLS, it is no TLS.
    m_core->tabs()->newTab(QStringLiteral("http://plain.example/"));
    QObject *plainView = currentWebView();
    QVERIFY(plainView != webView);
    plainView->property("security").value<QObject *>()->setProperty("allGood", false);
    QCOMPARE(find(QStringLiteral("addressLabel"))->property("text").toString(),
             QStringLiteral("plain.example"));
    QVERIFY(!bar->property("tlsBroken").toBool());
    QVERIFY(!warning->property("visible").toBool());
}

// The bar lies over the page, so the engine's own chrome gesture takes it off the
// bottom while a page is scrolled down and brings it back on the way up. Without it
// the foot of a page stays under the bar: RawWebView::setFooterMargin only reaches
// the engine while the virtual keyboard is up.
void tst_qmlload::barDoesNotCoverThePage()
{
    QObject *page = find(QStringLiteral("browserPage"));
    QObject *bar = find(QStringLiteral("navigationBar"));
    QObject *webView = currentWebView();
    const qreal fullBar = bar->property("height").toReal();
    const qreal pageHeight = page->property("height").toReal();
    const qreal inset = page->property("cutoutInset").toReal();
    QVERIFY(fullBar > 0);
    QVERIFY(inset > 0);

    // The engine's view sits between the cutout and the bar, so neither the first
    // line of a page nor its last is behind anything. The bar used to lie over the
    // page and take itself off the screen on the engine's chrome gesture; on device
    // the last rows of a page were still out of reach often enough to be a defect.
    QObject *viewArea = find(QStringLiteral("viewArea"));
    QCOMPARE(viewArea->property("y").toReal(), inset);
    QCOMPARE(viewArea->property("height").toReal(), pageHeight - fullBar - inset);
    QCOMPARE(webView->property("height").toReal(), pageHeight - fullBar - inset);
    QCOMPARE(find(QStringLiteral("cutoutBand"))->property("height").toReal(), inset);
    QCOMPARE(bar->property("y").toReal(), pageHeight - fullBar);
    QVERIFY(webView->property("chromeGestureEnabled").toBool());
    // The threshold is a constant, not the bar's own height: the bar changes height
    // in answer to the gesture, and a threshold that moved with it would chase it.
    QCOMPARE(webView->property("chromeGestureThreshold").toReal(),
             evaluate(page, QStringLiteral("Theme.itemSizeLarge")).toReal());
    // With the view already clear of the cutout, a page has nothing left to avoid.
    QCOMPARE(webView->property("safeAreaTop").toReal(), qreal(0));

    // That gesture now slims the bar rather than removing it. The bar animates
    // between its two heights, so what is asserted is where it settles -- and the
    // view is sized for the slimmer height from the first frame, so that it is
    // resized once rather than on every frame of the animation.
    const qreal slimBar = bar->property("slimHeight").toReal();
    QVERIFY(slimBar < fullBar);
    QVERIFY(slimBar > fullBar * 0.6);
    webView->setProperty("chrome", false);
    QVERIFY(page->property("barCompact").toBool());
    QVERIFY(bar->property("compact").toBool());
    QTRY_COMPARE(bar->property("height").toReal(), slimBar);
    QCOMPARE(bar->property("y").toReal(), pageHeight - slimBar);
    // The page ends above the slim bar as it does above the whole one, and the whole
    // slim bar takes presses: nothing under it is the page's.
    QCOMPARE(viewArea->property("height").toReal(), pageHeight - slimBar - inset);
    QObject *slimGesture = find(QStringLiteral("navigationBarGesture"));
    const qreal strip = slimGesture->property("strip").toReal();
    QCOMPARE(strip, slimBar);
    QCOMPARE(slimGesture->property("height").toReal(),
             strip + slimGesture->property("reach").toReal());

    // Nothing is left on the slim bar but the address, drawn smaller, and every
    // press on it belongs to the address. Its background stays opaque: faded, the
    // host was unreadable over a light page.
    QCOMPARE(bar->property("expansion").toReal(), qreal(0));
    QObject *background = find(QStringLiteral("navigationBarBackground"));
    QCOMPARE(background->property("color").value<QColor>().alpha(), 255);
    QVERIFY(!find(QStringLiteral("menuButton"))->property("visible").toBool());
    QVERIFY(!find(QStringLiteral("backButton"))->property("visible").toBool());
    QVERIFY(!find(QStringLiteral("reloadButton"))->property("visible").toBool());
    QCOMPARE(evaluate(bar, QStringLiteral("regionAt(width - 1)")).toString(),
             QStringLiteral("address"));
    QObject *addressLabel = find(QStringLiteral("addressLabel"));
    const int slimSize = addressLabel->property("font").value<QFont>().pixelSize();

    // A tap on the slim bar brings the whole bar back rather than the field, the way a
    // page scrolled back up does; the next tap edits.
    tapBar(QStringLiteral("address"));
    QVERIFY(webView->property("chrome").toBool());
    QVERIFY(!bar->property("compact").toBool());
    QVERIFY(!bar->property("editing").toBool());
    tapBar(QStringLiteral("address"));
    QVERIFY(bar->property("editing").toBool());
    // Editing keeps the bar whole, however the page is scrolled meanwhile, and so does
    // a new page.
    webView->setProperty("chrome", false);
    QVERIFY(!bar->property("compact").toBool());
    evaluate(bar, QStringLiteral("endEditing()"));
    QVERIFY(bar->property("compact").toBool());
    webView->setProperty("loading", true);
    QVERIFY(webView->property("chrome").toBool());
    QVERIFY(!bar->property("compact").toBool());
    webView->setProperty("loading", false);
    QTRY_COMPARE(bar->property("height").toReal(), fullBar);
    QCOMPARE(viewArea->property("height").toReal(), pageHeight - fullBar - inset);
    QVERIFY(addressLabel->property("font").value<QFont>().pixelSize() > slimSize);
    QCOMPARE(background->property("color").value<QColor>().alpha(), 255);
    QVERIFY(find(QStringLiteral("menuButton"))->property("visible").toBool());

    // The handle is drawn on the line between the bar and the page, which is where
    // the finger aims, and it lights up while a drag is under way.
    QObject *handle = find(QStringLiteral("barDragHandle"));
    QVERIFY(handle != nullptr);
    QVERIFY(handle->property("width").toReal() > 0);
    QVERIFY(handle->property("y").toReal() < 0);
    QVERIFY(!handle->property("active").toBool());
    QObject *gesture = find(QStringLiteral("navigationBarGesture"));
    gesture->setProperty("dragging", true);
    QVERIFY(handle->property("active").toBool());
    gesture->setProperty("dragging", false);
    QVERIFY(!handle->property("active").toBool());
}

void tst_qmlload::editingEndsWithTheKeyboard()
{
    QObject *bar = find(QStringLiteral("navigationBar"));
    QObject *field = find(QStringLiteral("addressField"));

    tapBar(QStringLiteral("address"));
    QVERIFY(bar->property("editing").toBool());
    evaluate(bar, QStringLiteral("focusChanged(false)"));
    QVERIFY(!bar->property("editing").toBool());
    QVERIFY(!field->property("visible").toBool());

    tapBar(QStringLiteral("address"));
    QVERIFY(bar->property("editing").toBool());
    evaluate(bar, QStringLiteral("keyboardVisibilityChanged(false)"));
    QVERIFY(!bar->property("editing").toBool());

    // The keyboard opening is not the end of anything.
    tapBar(QStringLiteral("address"));
    evaluate(bar, QStringLiteral("keyboardVisibilityChanged(true)"));
    QVERIFY(bar->property("editing").toBool());

    // The bar stays live while a field is up: the menu answers and it can still be
    // dragged. The address region is the one that does not, because the field has it.
    // The menu is the end of editing too: its sheet comes up from under the bar, where
    // the keyboard would sit over it.
    QVERIFY(find(QStringLiteral("navigationBarGesture"))->property("enabled").toBool());
    tapBar(QStringLiteral("menu"));
    QObject *menu = find(QStringLiteral("browserMenu"));
    QVERIFY(menu->property("open").toBool());
    QVERIFY(!bar->property("editing").toBool());
    QVERIFY(!field->property("visible").toBool());
    evaluate(menu, QStringLiteral("hide()"));
}

void tst_qmlload::thumbnailCapturedOnLoad()
{
    QObject *webView = currentWebView();
    const int tabId = m_core->tabs()->activeTabId();

    webView->setProperty("loading", true);
    webView->setProperty("loading", false);
    const QString captured = webView->property("lastGrabPath").toString();
    QVERIFY(!captured.isEmpty());
    // Grabbed at half size: the read back and the encode land in the middle of a
    // gesture, and the grid never draws the picture wider than half the screen.
    QCOMPARE(webView->property("lastGrabSize").toSize().width(),
             int(webView->property("width").toReal() / 2));
    QCOMPARE(m_core->tabs()->data(m_core->tabs()->index(0, 0), TabModel::ThumbnailRole).toString(),
             captured);

    // A failed save leaves the previous preview in place.
    webView->setProperty("grabSaveFails", true);
    webView->setProperty("loading", true);
    webView->setProperty("loading", false);
    QCOMPARE(m_core->tabs()->data(m_core->tabs()->index(0, 0), TabModel::ThumbnailRole).toString(),
             captured);

    Q_UNUSED(tabId)
}

void tst_qmlload::faviconResolvedAfterLoad()
{
    QObject *webView = currentWebView();
    webView->setProperty("scriptResult", QStringLiteral("/icon.png"));
    webView->setProperty("loading", true);
    webView->setProperty("loading", false);
    QVERIFY(webView->property("scripts").toStringList().contains(
        m_core->engineMessages()->faviconScript()));
    QCOMPARE(m_core->tabs()->activeFavicon(), QStringLiteral("https://www.qwant.com/icon.png"));

    // The page is asked for its theme colour in the same breath, and the strip beside
    // the cutout is painted with what it says. The engine keeps that colour to itself,
    // so there is nothing to read it from but the page.
    webView->setProperty("scriptResult", QStringLiteral("#123456"));
    webView->setProperty("loading", true);
    webView->setProperty("loading", false);
    QVERIFY(webView->property("scripts").toStringList().contains(
        m_core->engineMessages()->themeColorScript()));
    QCOMPARE(webView->property("pageThemeColor").toString(), QStringLiteral("#123456"));
    QCOMPARE(find(QStringLiteral("cutoutBand"))->property("color").value<QColor>(),
             QColor(QStringLiteral("#123456")));

    webView->setProperty("scriptFails", true);
    webView->setProperty("loading", true);
    webView->setProperty("loading", false);
    QCOMPARE(m_core->tabs()->activeFavicon(), QStringLiteral("https://www.qwant.com/favicon.ico"));
    // A page that says nothing leaves the strip in the application's own colour.
    QVERIFY(webView->property("pageThemeColor").toString().isEmpty());
    QVERIFY(find(QStringLiteral("cutoutBand"))->property("color").value<QColor>() !=
            QColor(QStringLiteral("#123456")));
}

void tst_qmlload::tabGrid()
{
    m_core->tabs()->newTab(QStringLiteral("https://two.example/"));
    QCOMPARE(findAll(QStringLiteral("webView")).count(), 2);
    QCOMPARE(currentWebView()->property("url").toUrl().toString(),
             QStringLiteral("https://two.example/"));

    QObject *page = find(QStringLiteral("browserPage"));
    QObject *grid = find(QStringLiteral("tabsView"));
    QVERIFY(!grid->property("visible").toBool());

    pullUpToTabs();
    QVERIFY(page->property("tabsOpen").toBool());
    QVERIFY(grid->property("visible").toBool());
    // The grid carries two rows of its own, drawn over the cells: along the head the
    // search for a tab, along the foot the way to a new tab, the groups and the way to
    // edit them. The head is also what keeps the top row of cells clear of the screen's
    // own cutout. One group so far, unnamed, so named by its count.
    auto *headRow = qobject_cast<QQuickItem *>(find(QStringLiteral("gridHeadRow")));
    auto *footRow = qobject_cast<QQuickItem *>(find(QStringLiteral("gridFootRow")));
    QVERIFY(headRow != nullptr);
    QVERIFY(footRow != nullptr);
    const auto item = [this](const char *name) {
        return qobject_cast<QQuickItem *>(find(QLatin1String(name)));
    };
    QVERIFY(headRow->isAncestorOf(item("tabSearchField")));
    QVERIFY(footRow->isAncestorOf(item("newTabButton")));
    QVERIFY(footRow->isAncestorOf(item("tabGroupStrip")));
    QVERIFY(footRow->isAncestorOf(item("editGroupsButton")));
    QVERIFY(find(QStringLiteral("searchTabsButton")) == nullptr);
    // The head along the top of the grid, and the foot along its bottom.
    auto *gridView = qobject_cast<QQuickItem *>(grid);
    QCOMPARE(headRow->mapToItem(gridView, QPointF(0, 0)).y(), qreal(0));
    QCOMPARE(footRow->mapToItem(gridView, QPointF(0, footRow->height())).y(), gridView->height());
    // The search field across the head from its left edge, where its words start; new
    // tab in the foot's left corner and edit in its right, and the names between them in
    // the middle of the screen.
    const auto sceneX = [](QQuickItem *of, qreal x) { return of->mapToScene(QPointF(x, 0)).x(); };
    const qreal screenWidth = headRow->width();
    QCOMPARE(sceneX(item("tabSearchField"), 0), qreal(0));
    QCOMPARE(item("tabSearchField")->width(), screenWidth);
    QCOMPARE(item("tabSearchField")->property("placeholderText").toString(),
             QStringLiteral("Search tabs"));
    QVERIFY(sceneX(item("newTabButton"), 0) < screenWidth / 2);
    QVERIFY(sceneX(item("editGroupsButton"), 0) > screenWidth / 2);
    QQuickItem *names = item("tabGroupList");
    QCOMPARE(sceneX(names, names->width() / 2), screenWidth / 2);
    QVERIFY(sceneX(names, 0) >= sceneX(item("newTabButton"), item("newTabButton")->width()));
    QVERIFY(sceneX(names, names->width()) <= sceneX(item("editGroupsButton"), 0));
    QList<QObject *> groupLabels = findAll(QStringLiteral("tabGroupLabel"));
    QCOMPARE(groupLabels.count(), 1);
    QCOMPARE(groupLabels.first()->property("text").toString(), QStringLiteral("2 tab(s)"));
    // The row is sized to sit with the search field: the names in medium type, and the
    // corners' icons a step below Silica's medium size, each at the page margin inside
    // a button a padding wider either side and the row's height.
    QCOMPARE(groupLabels.first()->property("font").value<QFont>().pixelSize(),
             evaluate(grid, QStringLiteral("Theme.fontSizeMedium")).toInt());
    const qreal smallPlus = evaluate(grid, QStringLiteral("Theme.iconSizeSmallPlus")).toReal();
    QVERIFY(smallPlus < evaluate(grid, QStringLiteral("Theme.iconSizeMedium")).toReal());
    const qreal margin = evaluate(grid, QStringLiteral("Theme.horizontalPageMargin")).toReal();
    for (QQuickItem *corner : {item("newTabButton"), item("editGroupsButton")}) {
        auto *icon = corner->property("icon").value<QObject *>();
        QVERIFY(icon != nullptr);
        QCOMPARE(icon->property("sourceSize").toSizeF(), QSizeF(smallPlus, smallPlus));
        QCOMPARE(corner->height(), footRow->height());
        QVERIFY(corner->width() > smallPlus);
    }
    QCOMPARE(sceneX(item("newTabButton"), (item("newTabButton")->width() - smallPlus) / 2), margin);
    QCOMPARE(sceneX(item("editGroupsButton"), (item("editGroupsButton")->width() + smallPlus) / 2),
             screenWidth - margin);
    // The current group is the one underlined.
    QList<QObject *> underlines = findAll(QStringLiteral("tabGroupUnderline"));
    QCOMPARE(underlines.count(), 1);
    QVERIFY(underlines.first()->property("visible").toBool());

    // Both the head row's controls and the first row of cells clear the display's own
    // cutout: the head sat under the notch, and so did the close button in the corner
    // of the first cell.
    const qreal cutout = grid->property("cutoutHeight").toReal();
    QVERIFY(cutout > 0);
    QVERIFY(headRow->height() > cutout);
    QVERIFY(item("gridHeadControls")->y() >= cutout);
    // What says the edge is a pulley: a line in the highlight colour across the very
    // top of the screen, cutout or no cutout.
    QObject *indicator = find(QStringLiteral("gridPullIndicator"));
    QVERIFY(indicator != nullptr);
    QCOMPARE(indicator->property("y").toReal(), 0.0);
    // The highlight background rather than the highlight itself: the stub theme's.
    QCOMPARE(indicator->property("color").value<QColor>(), QColor(QStringLiteral("#aaccff")));
    QCOMPARE(indicator->property("width").toReal(), headRow->width());
    QVERIFY(indicator->property("height").toReal() > 0);
    QVERIFY(find(QStringLiteral("gridDragHandle")) == nullptr);
    // Both rows are panes of Silica's glass: the tint, and over it the ambience's own
    // pattern, tiled and drawn at the tenth the glass draws it at -- under whatever the
    // row carries, and loaded, which the stub's theme lets every image be.
    const QString pattern = evaluate(grid, QStringLiteral("Theme._patternImage")).toString();
    QVERIFY(!pattern.isEmpty());
    for (QQuickItem *row : {headRow, footRow}) {
        QQuickItem *glass = row->childItems().value(0);
        QVERIFY(glass != nullptr);
        QVERIFY(glass->objectName().endsWith(QLatin1String("Glass")));
        QCOMPARE(glass->property("source").toUrl().toString(), pattern);
        QVERIFY(evaluate(glass, QStringLiteral("fillMode === Image.Tile")).toBool());
        QCOMPARE(glass->opacity(), 0.1);
        QCOMPARE(glass->width(), row->width());
        QCOMPARE(glass->height(), row->height());
        QTRY_VERIFY(evaluate(glass, QStringLiteral("status === Image.Ready")).toBool());
    }
    QCOMPARE(item("gridHeadGlass")->parentItem(), headRow);
    QCOMPARE(item("gridFootGlass")->parentItem(), footRow);
    // Room in the scrolled content for each row, so no cell is stranded under one.
    auto *headerItem = find(QStringLiteral("tabGrid"))->property("headerItem").value<QObject *>();
    QVERIFY(headerItem != nullptr);
    QCOMPARE(headerItem->property("height").toReal(), headRow->height());
    auto *footerItem = find(QStringLiteral("tabGrid"))->property("footerItem").value<QObject *>();
    QVERIFY(footerItem != nullptr);
    QCOMPARE(footerItem->property("height").toReal(), footRow->height());

    QList<QObject *> previews = findAll(QStringLiteral("tabPreview"));
    QCOMPARE(previews.count(), 2);

    // The preview box is rounded, and the wash drawn round the active one is square,
    // as Silica's own is. Clipping is rectangular whatever the shape of the item doing
    // it, so the picture is cut to the box's corners by a mask.
    QObject *shot = findObjects(previews.at(1), QStringLiteral("tabPreviewShot")).first();
    QVERIFY(shot->property("radius").toReal() > 0);
    QObject *wash = findObjects(previews.at(1), QStringLiteral("tabPreviewHighlight")).first();
    QVERIFY(wash->property("visible").toBool());
    QCOMPARE(wash->property("radius").toReal(), qreal(0));
    // The picture sits in from the cell's edges by a little more than a medium padding,
    // and two cells stand twice that apart.
    const qreal inset = previews.at(1)->property("inset").toReal();
    QVERIFY(inset > evaluate(grid, QStringLiteral("Theme.paddingMedium")).toReal());
    QCOMPARE(shot->property("x").toReal(), inset);
    QCOMPARE(shot->property("width").toReal(),
             previews.at(1)->property("width").toReal() - 2 * inset);
    // The active cell's box is marked as well, by its border.
    auto *border = shot->property("border").value<QObject *>();
    QVERIFY(border != nullptr);
    QVERIFY(border->property("width").toReal() > 0);
    auto *shotLayer = shot->property("layer").value<QObject *>();
    QVERIFY(shotLayer != nullptr);
    QVERIFY(shotLayer->property("enabled").toBool());

    QCOMPARE(findObjects(previews.at(1), QStringLiteral("tabTitle"))
                 .first()
                 ->property("text")
                 .toString(),
             QStringLiteral("https://two.example/"));
    QVERIFY(previews.at(1)->property("highlighted").toBool());
    // Opening the grid captured the tab being left, so that cell has a preview while
    // the one never displayed still shows its placeholder.
    QVERIFY(!m_core->tabs()
                 ->data(m_core->tabs()->index(1, 0), TabModel::ThumbnailRole)
                 .toString()
                 .isEmpty());
    QVERIFY(!findObjects(previews.at(1), QStringLiteral("tabPreviewPlaceholder"))
                 .first()
                 ->property("visible")
                 .toBool());
    QVERIFY(findObjects(previews.at(0), QStringLiteral("tabPreviewPlaceholder"))
                .first()
                ->property("visible")
                .toBool());

    // Tapping a preview is one way back, and it brings its tab with it.
    QMetaObject::invokeMethod(previews.at(0), "tapped");
    QCOMPARE(m_core->tabs()->activeTabIndex(), 0);
    QVERIFY(!page->property("tabsOpen").toBool());
    QCOMPARE(currentWebView()->property("url").toUrl().toString(), Settings::defaultHomePage());

    // Leaving for the grid refreshes the preview of the tab being left.
    QObject *homeView = currentWebView();
    QCOMPARE(homeView->property("grabCount").toInt(), 0);
    pullUpToTabs();
    QCOMPARE(homeView->property("grabCount").toInt(), 1);

    previews = findAll(QStringLiteral("tabPreview"));
    click(findObjects(previews.at(1), QStringLiteral("closeTabButton")).first());
    QCOMPARE(m_core->tabs()->count(), 1);
    QCOMPARE(findAll(QStringLiteral("tabPreview")).count(), 1);

    // The other way back: dragging the grid down past its own top.
    QVERIFY(page->property("tabsOpen").toBool());
    pullDownToBrowser();
    QVERIFY(!page->property("tabsOpen").toBool());

    // Half a pull moves the deck half way and settles back on the grid, the same way
    // half a drag on the bar settles back on the page.
    pullUpToTabs();
    const qreal threshold = page->property("pullThreshold").toReal();
    evaluate(grid, QStringLiteral("pullStarted()"));
    evaluate(grid, QStringLiteral("pulled(%1)").arg(threshold / 2));
    QCOMPARE(page->property("tabsOffset").toReal(),
             page->property("height").toReal() - threshold / 2);
    evaluate(grid, QStringLiteral("pullFinished(%1)").arg(threshold / 2));
    QVERIFY(page->property("tabsOpen").toBool());

    // That pull is the view's own overscroll: dragged past its top it reports the
    // distance and moves up by the same amount, which cancels the shift the flickable
    // would draw and leaves its content under the finger.
    QObject *view = find(QStringLiteral("tabGrid"));
    const qreal originY = view->property("originY").toReal();
    view->setProperty("contentY", originY - threshold);
    QCOMPARE(view->property("overscroll").toReal(), threshold);
    QCOMPARE(view->property("y").toReal(), -threshold);
    view->setProperty("contentY", originY);
    QCOMPARE(view->property("overscroll").toReal(), qreal(0));
    QCOMPARE(view->property("y").toReal(), qreal(0));

    // A cell carried across the grid reorders the tabs. The view of the tab that
    // moved is carried with it rather than built again: a Repeater that recreated its
    // delegates on a move would reload the page behind the preview.
    m_core->tabs()->newTab(QStringLiteral("https://three.example/"));
    pullUpToTabs();
    QObject *carried = currentWebView();
    const int carriedId = m_core->tabs()->activeTabId();
    QCOMPARE(m_core->tabs()->activeTabIndex(), 1);
    previews = findAll(QStringLiteral("tabPreview"));
    QCOMPARE(previews.count(), 2);
    // A cell that has been carried must not also open on release: releasing one used
    // to drop the grid and jump to that tab.
    previews.at(1)->setProperty("carried", true);
    evaluate(previews.at(1), QStringLiteral("releaseTap()"));
    QVERIFY(find(QStringLiteral("browserPage"))->property("tabsOpen").toBool());
    previews.at(1)->setProperty("carried", false);

    evaluate(previews.at(1), QStringLiteral("moveRequested(1, 0)"));
    QCOMPARE(m_core->tabs()->activeTabId(), carriedId);
    QCOMPARE(m_core->tabs()->activeTabIndex(), 0);
    QCOMPARE(currentWebView(), carried);
    QCOMPARE(currentWebView()->property("url").toUrl().toString(),
             QStringLiteral("https://three.example/"));
    m_core->tabs()->closeTab(1);

    // That control opens a tab and hands the page back with it.
    click(find(QStringLiteral("newTabButton")));
    QCOMPARE(m_core->tabs()->count(), 2);
    QVERIFY(!page->property("tabsOpen").toBool());
    QCOMPARE(m_core->tabs()->activeUrl(), Settings::defaultHomePage());
}

void tst_qmlload::tabGroups()
{
    TabModel *tabs = m_core->tabs();
    const int home = tabs->defaultGroupId();
    const int first = tabs->activeTabId();
    QObject *page = find(QStringLiteral("browserPage"));
    pullUpToTabs();

    // The strip holds every group: the default one alone so far.
    QObject *strip = find(QStringLiteral("tabGroupStrip"));
    QVERIFY(strip != nullptr);
    QCOMPARE(findAll(QStringLiteral("tabGroupItem")).count(), 1);
    QCOMPARE(tabs->currentGroupIndex(), 0);

    // The edit corner leads to the list of groups, where a new one is made from the
    // row under the last group; it is the current group from then on, and the grid
    // under the page shows it empty.
    click(find(QStringLiteral("editGroupsButton")));
    QCOMPARE(currentPage()->objectName(), QStringLiteral("tabGroupsPage"));
    QCOMPARE(findAll(QStringLiteral("tabGroupDelegate")).count(), 1);
    QVERIFY(find(QStringLiteral("tabGroupDelegate"))->property("enabled").toBool());
    QVERIFY(find(QStringLiteral("newGroupMenu")) == nullptr);
    click(find(QStringLiteral("newGroupButton")));
    QObject *dialog = currentPage();
    QCOMPARE(dialog->objectName(), QStringLiteral("tabGroupDialog"));
    QCOMPARE(dialog->property("groupId").toInt(), 0);
    find(QStringLiteral("groupNameField"))->setProperty("text", QStringLiteral("Work"));
    QMetaObject::invokeMethod(dialog, "accept");
    popPage();
    QCOMPARE(tabs->groups().count(), 2);
    const int work = tabs->groups().at(1).id;
    QCOMPARE(tabs->groups().at(1).name, QStringLiteral("Work"));
    QCOMPARE(tabs->currentGroupId(), work);
    QList<QObject *> delegates = byRow(findAll(QStringLiteral("tabGroupDelegate")));
    QCOMPARE(delegates.count(), 2);
    // The default group is named by its count, and offers neither rename nor delete.
    QCOMPARE(findObjects(delegates.at(0), QStringLiteral("tabGroupName"))
                 .first()
                 ->property("text")
                 .toString(),
             QStringLiteral("1 tab(s)"));
    QCOMPARE(findObjects(delegates.at(1), QStringLiteral("tabGroupName"))
                 .first()
                 ->property("text")
                 .toString(),
             QStringLiteral("Work"));
    QVERIFY(!findObjects(delegates.at(0), QStringLiteral("renameGroupMenu"))
                 .first()
                 ->property("enabled")
                 .toBool());
    QVERIFY(!findObjects(delegates.at(0), QStringLiteral("deleteGroupMenu"))
                 .first()
                 ->property("enabled")
                 .toBool());
    QVERIFY(findObjects(delegates.at(1), QStringLiteral("renameGroupMenu"))
                .first()
                ->property("enabled")
                .toBool());

    // Tapping a group there makes it current and returns to the grid, still open.
    click(delegates.at(1));
    QCOMPARE(currentPage()->objectName(), QStringLiteral("browserPage"));
    QVERIFY(page->property("tabsOpen").toBool());
    QCOMPARE(findAll(QStringLiteral("tabPreview")).count(), 0);
    QCOMPARE(findAll(QStringLiteral("tabGroupItem")).count(), 2);
    QCOMPARE(tabs->currentGroupIndex(), 1);
    QVERIFY(findAll(QStringLiteral("tabGroupUnderline")).at(1)->property("visible").toBool());
    QCOMPARE(tabs->activeTabId(), first);

    // A new tab opens in the current group.
    click(find(QStringLiteral("newTabButton")));
    const int second = tabs->activeTabId();
    QVERIFY(second != first);
    QCOMPARE(tabs->tabCountInGroup(work), 1);
    QVERIFY(!page->property("tabsOpen").toBool());
    pullUpToTabs();
    QCOMPARE(findAll(QStringLiteral("tabPreview")).count(), 1);
    QCOMPARE(findAll(QStringLiteral("webView")).count(), 2);

    // A tap on a group in the strip makes it current, and its last tab comes to the
    // front.
    evaluate(strip, QStringLiteral("select(0)"));
    QCOMPARE(tabs->currentGroupId(), home);
    QCOMPARE(tabs->activeTabId(), first);
    QCOMPARE(findAll(QStringLiteral("tabPreview")).count(), 1);
    QCOMPARE(findAll(QStringLiteral("webView")).count(), 2);

    // Into a group made for it: made from the list of groups, which makes it current
    // and empty. How a tab is carried there from the grid is tabsDropOntoGroups().
    click(find(QStringLiteral("editGroupsButton")));
    click(find(QStringLiteral("newGroupButton")));
    dialog = currentPage();
    QCOMPARE(dialog->objectName(), QStringLiteral("tabGroupDialog"));
    find(QStringLiteral("groupNameField"))->setProperty("text", QStringLiteral("Mail"));
    QMetaObject::invokeMethod(dialog, "accept");
    popPage();
    popPage();
    QCOMPARE(tabs->groups().count(), 3);
    QVERIFY(tabs->moveTabToGroup(second, tabs->groups().at(2).id));
    QCOMPARE(tabs->currentGroupId(), tabs->groups().at(2).id);
    QCOMPARE(tabs->tabCountInGroup(work), 0);
    QCOMPARE(tabs->tabCountInGroup(home), 1);

    // Rename and delete are in the group's own menu; the default group has neither.
    pullUpToTabs();
    click(find(QStringLiteral("editGroupsButton")));
    delegates = byRow(findAll(QStringLiteral("tabGroupDelegate")));
    QCOMPARE(delegates.count(), 3);
    click(findObjects(delegates.at(1), QStringLiteral("renameGroupMenu")).first());
    dialog = currentPage();
    QCOMPARE(dialog->property("groupId").toInt(), work);
    QCOMPARE(dialog->property("name").toString(), QStringLiteral("Work"));
    find(QStringLiteral("groupNameField"))->setProperty("text", QStringLiteral("Play"));
    QMetaObject::invokeMethod(dialog, "accept");
    popPage();
    QCOMPARE(tabs->groups().at(1).name, QStringLiteral("Play"));

    delegates = byRow(findAll(QStringLiteral("tabGroupDelegate")));
    QObject *deleteMenu = findObjects(delegates.at(2), QStringLiteral("deleteGroupMenu")).first();
    QVERIFY(deleteMenu->property("enabled").toBool());
    click(deleteMenu);
    QCOMPARE(tabs->groups().count(), 2);
    QCOMPARE(tabs->count(), 1);
    QCOMPARE(tabs->activeTabId(), first);
    click(findObjects(byRow(findAll(QStringLiteral("tabGroupDelegate"))).at(1),
                      QStringLiteral("deleteGroupMenu"))
              .first());
    QCOMPARE(tabs->groups().count(), 1);
    QVERIFY(!findObjects(byRow(findAll(QStringLiteral("tabGroupDelegate"))).at(0),
                         QStringLiteral("deleteGroupMenu"))
                 .first()
                 ->property("enabled")
                 .toBool());
    QCOMPARE(tabs->currentGroupIndex(), 0);
    popPage();
}

// What is typed in the field along the grid's head lists the tabs that hold it, group by
// group, in place of the cells; a tap on one brings it to the front and puts the grid
// away. Nothing is pushed for it.
void tst_qmlload::tabSearch()
{
    TabModel *tabs = m_core->tabs();
    const int first = tabs->activeTabId();
    const int work = tabs->addGroup(QStringLiteral("Work"));
    tabs->groupModel()->activate(1);
    const int second = tabs->newTab(QStringLiteral("https://two.example/"));
    QCOMPARE(tabs->tabCountInGroup(work), 1);
    tabs->groupModel()->activate(0);
    QCOMPARE(tabs->activeTabId(), first);
    QObject *page = find(QStringLiteral("browserPage"));
    QObject *strip = find(QStringLiteral("tabGroupStrip"));
    pullUpToTabs();

    tabs->updateTitle(first, QStringLiteral("Office hours"));
    tabs->updateTitle(second, QStringLiteral("Office mail"));
    QObject *view = find(QStringLiteral("tabsView"));
    QObject *field = find(QStringLiteral("tabSearchField"));
    QObject *found = find(QStringLiteral("tabSearchList"));
    auto *cells = find(QStringLiteral("tabGrid"))->property("contentItem").value<QQuickItem *>();
    const auto cellsShown = [cells]() { return cells->isVisible(); };
    QVERIFY(!found->property("visible").toBool());
    QVERIFY(cellsShown());
    // The term follows the field a beat after typing stops, not on each keystroke, and
    // the cells stay until it does.
    field->setProperty("text", QStringLiteral("office"));
    QVERIFY(m_core->tabSearch()->searchTerm().isEmpty());
    QVERIFY(!found->property("visible").toBool());
    QObject *debounce = find(QStringLiteral("searchDebounce"));
    QVERIFY(debounce->property("running").toBool());
    QVERIFY(debounce->property("interval").toInt() >= 200);
    QMetaObject::invokeMethod(debounce, "triggered");
    QCOMPARE(m_core->tabSearch()->searchTerm(), QStringLiteral("office"));
    QVERIFY(found->property("visible").toBool());
    QVERIFY(!cellsShown());
    QCOMPARE(currentPage()->objectName(), QStringLiteral("browserPage"));
    QList<QObject *> results = byRow(findAll(QStringLiteral("tabSearchDelegate")));
    QCOMPARE(results.count(), 2);
    QObject *heading = findObjects(results.at(1), QStringLiteral("tabSearchGroupHeader")).first();
    QVERIFY(heading->property("visible").toBool());
    QCOMPARE(heading->property("text").toString(), QStringLiteral("Work"));
    QCOMPARE(findObjects(results.at(0), QStringLiteral("tabSearchGroupHeader"))
                 .first()
                 ->property("text")
                 .toString(),
             QStringLiteral("1 tab(s)"));
    // Emptied, the field gives the cells back at once.
    field->setProperty("text", QString());
    QVERIFY(!found->property("visible").toBool());
    QVERIFY(cellsShown());
    field->setProperty("text", QStringLiteral("mail"));
    QMetaObject::invokeMethod(debounce, "triggered");
    // Enter puts the keyboard away and leaves what was found.
    field->setProperty("focus", true);
    enterKey(field);
    QVERIFY(!field->property("focus").toBool());
    QVERIFY(found->property("visible").toBool());
    results = findAll(QStringLiteral("tabSearchDelegate"));
    QCOMPARE(results.count(), 1);
    QCOMPARE(findObjects(results.at(0), QStringLiteral("tabRowTitle"))
                 .first()
                 ->property("text")
                 .toString(),
             QStringLiteral("Office mail"));
    click(findObjects(results.at(0), QStringLiteral("tabSearchItem")).first());
    QCOMPARE(currentPage()->objectName(), QStringLiteral("browserPage"));
    QVERIFY(!page->property("tabsOpen").toBool());
    QCOMPARE(tabs->activeTabId(), second);
    QCOMPARE(tabs->currentGroupId(), work);
    QCOMPARE(evaluate(strip, QStringLiteral("currentButton.current")).toBool(), true);
    QCOMPARE(evaluate(strip, QStringLiteral("currentButton")).value<QObject *>(),
             findAll(QStringLiteral("tabGroupItem")).at(1));
    // The search does not outlive the grid: the next one starts empty, with the cells.
    QVERIFY(m_core->tabSearch()->searchTerm().isEmpty());
    QVERIFY(field->property("text").toString().isEmpty());
    QVERIFY(!debounce->property("running").toBool());
    QVERIFY(cellsShown());
    // Nor does one left by pulling the page back over the grid.
    pullUpToTabs();
    field->setProperty("text", QStringLiteral("office"));
    QMetaObject::invokeMethod(debounce, "triggered");
    QVERIFY(found->property("visible").toBool());
    pullDownToBrowser();
    QTRY_VERIFY(!view->property("visible").toBool());
    QVERIFY(field->property("text").toString().isEmpty());
    QVERIFY(m_core->tabSearch()->searchTerm().isEmpty());
    QVERIFY(!found->property("visible").toBool());
}

// A tab carried down onto a group in the strip moves into that group, the way a tap
// on a name chooses it: through the strip's own functions. The finger that carries it
// is carryToGroupUnderAFinger().
void tst_qmlload::tabsDropOntoGroups()
{
    TabModel *tabs = m_core->tabs();
    const int home = tabs->defaultGroupId();
    const int first = tabs->activeTabId();
    const int second = tabs->newTab(QStringLiteral("https://two.example/"));
    const int work = tabs->addGroup(QStringLiteral("Work"));
    tabs->groupModel()->activate(0);
    QCOMPARE(tabs->activeTabId(), second);
    QObject *page = find(QStringLiteral("browserPage"));
    QObject *strip = find(QStringLiteral("tabGroupStrip"));
    pullUpToTabs();

    // The tab in front takes the grid with it: the tab in front is always in the group
    // the grid shows. The view behind the tab is the one it had.
    QCOMPARE(strip->property("dropIndex").toInt(), -1);
    strip->setProperty("dropIndex", 1);
    QVERIFY(evaluate(strip, QStringLiteral("dropTab(%1)").arg(second)).toBool());
    QCOMPARE(strip->property("dropIndex").toInt(), -1);
    // A moment later: the carried cell's own release is still running when the finger
    // lifts, and the move takes the cell out of the grid.
    QCOMPARE(tabs->tabCountInGroup(home), 2);
    QTRY_COMPARE(tabs->tabCountInGroup(work), 1);
    QCOMPARE(tabs->currentGroupId(), work);
    QCOMPARE(tabs->activeTabId(), second);
    QCOMPARE(findAll(QStringLiteral("webView")).count(), 2);
    QVERIFY(page->property("tabsOpen").toBool());

    // Dropped with nothing lit, nothing moves, then or a moment later.
    QVERIFY(!evaluate(strip, QStringLiteral("dropTab(%1)").arg(second)).toBool());
    QCoreApplication::processEvents();
    QCOMPARE(tabs->tabCountInGroup(work), 1);
    // A finger nowhere near the strip lights nothing, and the carry is the grid's;
    // however the carry ends, nothing stays lit.
    strip->setProperty("dropIndex", 0);
    QVERIFY(!evaluate(strip, QStringLiteral("carryOver(null, -1, -1)")).toBool());
    QCOMPARE(strip->property("dropIndex").toInt(), -1);
    strip->setProperty("dropIndex", 0);
    evaluate(strip, QStringLiteral("endCarry()"));
    QCOMPARE(strip->property("dropIndex").toInt(), -1);

    // A tab that is not the one in front leaves the grid where it is, a cell fewer.
    evaluate(strip, QStringLiteral("select(0)"));
    QCOMPARE(tabs->activeTabId(), first);
    const int third = tabs->newTab(QStringLiteral("https://three.example/"));
    tabs->activateTabById(first);
    QCOMPARE(findAll(QStringLiteral("tabPreview")).count(), 2);
    strip->setProperty("dropIndex", 1);
    QVERIFY(evaluate(strip, QStringLiteral("dropTab(%1)").arg(third)).toBool());
    QTRY_COMPARE(tabs->tabCountInGroup(work), 2);
    QCOMPARE(tabs->currentGroupId(), home);
    QCOMPARE(tabs->activeTabId(), first);
    QCOMPARE(findAll(QStringLiteral("tabPreview")).count(), 1);
}

void tst_qmlload::previewGestures()
{
    TabModel *tabs = m_core->tabs();
    tabs->newTab(QStringLiteral("https://two.example/"));
    QObject *page = find(QStringLiteral("browserPage"));
    pullUpToTabs();
    QList<QObject *> previews = findAll(QStringLiteral("tabPreview"));
    QCOMPARE(previews.count(), 2);
    QObject *cell = previews.at(0);

    // A second of holding still picks a cell up to be carried; a cell picked up does
    // not open when the finger lifts.
    QObject *timer = findObjects(cell, QStringLiteral("holdTimer")).first();
    QCOMPARE(timer->property("interval").toInt(), 1000);
    QCOMPARE(cell->property("holdInterval").toInt(), 1000);
    // A thumb drifts while it holds: some movement still counts as holding. The grid
    // may still take the drag while a hold is forming -- a flickable refused a touch
    // once never takes it back, and the grid could then be neither scrolled nor
    // pulled from a cell -- and may not once the cell is up (gridGesturesUnderAFinger).
    QObject *gesture = findObjects(cell, QStringLiteral("tabPreviewGesture")).first();
    QVERIFY(cell->property("holdTolerance").toReal() > 0);
    QVERIFY(!cell->property("held").toBool());
    cell->setProperty("holding", true);
    QVERIFY(!gesture->property("preventStealing").toBool());
    evaluate(cell, QStringLiteral("letGo()"));
    QVERIFY(!cell->property("holding").toBool());
    evaluate(cell, QStringLiteral("pickUp()"));
    QVERIFY(cell->property("held").toBool());
    QVERIFY(cell->property("carried").toBool());
    QVERIFY(!cell->property("holding").toBool());
    QVERIFY(!timer->property("running").toBool());
    QVERIFY(gesture->property("preventStealing").toBool());
    evaluate(cell, QStringLiteral("releaseTap()"));
    QVERIFY(page->property("tabsOpen").toBool());
    evaluate(cell, QStringLiteral("drop()"));
    QVERIFY(!cell->property("held").toBool());

    // A slide to the left that stops short springs back and closes nothing.
    const qreal width = cell->property("width").toReal();
    QCOMPARE(cell->property("closeDistance").toReal(), width / 3);
    evaluate(cell, QStringLiteral("swipeTo(-10)"));
    QVERIFY(cell->property("swiping").toBool());
    QVERIFY(cell->property("carried").toBool());
    evaluate(cell, QStringLiteral("releaseSwipe()"));
    QVERIFY(!cell->property("swiping").toBool());
    QCOMPARE(tabs->count(), 2);
    // Only leftwards: to the right the cell stays put.
    evaluate(cell, QStringLiteral("swipeTo(50)"));
    evaluate(cell, QStringLiteral("releaseSwipe()"));
    QCOMPARE(tabs->count(), 2);

    // Far enough, and letting go closes the tab.
    evaluate(cell, QStringLiteral("swipeTo(%1)").arg(-width / 2));
    evaluate(cell, QStringLiteral("releaseSwipe()"));
    QCOMPARE(tabs->count(), 1);
    QCOMPARE(findAll(QStringLiteral("tabPreview")).count(), 1);
    QVERIFY(page->property("tabsOpen").toBool());

    // The close button is its own mark, a disc faint enough not to be the first thing
    // seen on each cell -- it is opaque only under a finger, which
    // gridGesturesUnderAFinger() puts on it; there is no second disc under it.
    QObject *mark = find(QStringLiteral("closeTabMark"));
    QVERIFY(mark != nullptr);
    QCOMPARE(mark->property("opacity").toReal(),
             evaluate(mark, QStringLiteral("Theme.opacityHigh")).toReal());
    QCOMPARE(mark->property("radius").toReal(), mark->property("width").toReal() / 2);
    QVERIFY(find(QStringLiteral("closeTabDisc")) == nullptr);
}

namespace {

QPoint centreOf(QObject *object)
{
    auto *item = qobject_cast<QQuickItem *>(object);
    return item->mapToScene(QPointF(item->width() / 2, item->height() / 2)).toPoint();
}

// The application in a real window, for as long as this lives: for the gestures whose
// outcome Qt's own event delivery decides.
class FingerWindow
{
public:
    explicit FingerWindow(QObject *root)
        : m_root(qobject_cast<QQuickItem *>(root))
    {
        m_window.resize(int(m_root->width()), int(m_root->height()));
        m_root->setParentItem(m_window.contentItem());
        m_window.show();
    }
    ~FingerWindow()
    {
        m_root->setParentItem(nullptr);
    }
    FingerWindow(const FingerWindow &) = delete;
    FingerWindow &operator=(const FingerWindow &) = delete;

    QQuickWindow *window()
    {
        return &m_window;
    }

private:
    QQuickItem *m_root;
    QQuickWindow m_window;
};

// The script errors QML reports while one of these lives, which a test that drives
// the QML by hand would otherwise only see scroll past: a handler that throws goes on
// as if nothing had happened.
QStringList *scriptErrors = nullptr;
QtMessageHandler previousHandler = nullptr;

void recordScriptError(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    if (scriptErrors != nullptr && (message.contains(QLatin1String("TypeError")) ||
                                    message.contains(QLatin1String("ReferenceError")) ||
                                    message.contains(QLatin1String("invalid context")))) {
        scriptErrors->append(message);
    }
    previousHandler(type, context, message);
}

class ScriptErrors
{
public:
    ScriptErrors()
    {
        scriptErrors = &m_errors;
        previousHandler = qInstallMessageHandler(recordScriptError);
    }
    ~ScriptErrors()
    {
        qInstallMessageHandler(previousHandler);
        scriptErrors = nullptr;
    }
    ScriptErrors(const ScriptErrors &) = delete;
    ScriptErrors &operator=(const ScriptErrors &) = delete;

    QString all() const
    {
        return m_errors.join(QLatin1Char('\n'));
    }

private:
    QStringList m_errors;
};

// A finger put down at one point, moved to another in even steps and lifted there.
void drag(QWindow *window, const QPoint &from, const QPoint &to)
{
    const int steps = 24;
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, from);
    for (int step = 1; step <= steps; ++step) {
        QTest::mouseMove(window, from + (to - from) * step / steps);
    }
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, to);
}

} // namespace

// The grid's gestures under a real finger, in a real window. Whether a drag begun on a
// cell ever reaches the grid is decided inside Qt's own delivery -- a cell that keeps
// the touch from the moment it is pressed leaves the flickable nothing to take for the
// rest of it -- so raising the signals the gestures end in, as the rest of this file
// does, cannot see that go wrong. It went wrong once: the grid could only be pulled
// back from the gaps between its cells.
void tst_qmlload::gridGesturesUnderAFinger()
{
    TabModel *tabs = m_core->tabs();
    const int second = tabs->newTab(QStringLiteral("https://two.example/"));
    QObject *page = find(QStringLiteral("browserPage"));
    auto *root = qobject_cast<QQuickItem *>(m_window.data());
    FingerWindow host(root);
    QQuickWindow &window = *host.window();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    const auto openGrid = [&]() {
        pullUpToTabs();
        QTRY_COMPARE(page->property("tabsOffset").toReal(), page->property("fullHeight").toReal());
    };
    const auto cells = [&]() { return byRow(findAll(QStringLiteral("tabPreview"))); };
    const QPoint down(0, 3 * page->property("pullThreshold").toInt());

    // Dragged down from a cell, the grid hands the page back, as it does from the gaps
    // between the cells and from the row over them at its head.
    openGrid();
    drag(&window, centreOf(cells().first()), centreOf(cells().first()) + down);
    QVERIFY(!page->property("tabsOpen").toBool());
    openGrid();
    drag(&window, centreOf(find(QStringLiteral("gridHeadControls"))),
         centreOf(find(QStringLiteral("gridHeadControls"))) + down);
    QVERIFY(!page->property("tabsOpen").toBool());
    openGrid();
    const QPoint gap(int(root->width()) / 2, int(root->height()) * 3 / 4);
    drag(&window, gap, gap + down);
    QVERIFY(!page->property("tabsOpen").toBool());

    // A tap on a cell opens its tab.
    openGrid();
    QCOMPARE(tabs->activeTabId(), second);
    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, centreOf(cells().first()));
    QVERIFY(!page->property("tabsOpen").toBool());
    QVERIFY(tabs->activeTabId() != second);

    // The close button's disc is faint until a finger is on it; one taken off it
    // before it lifts closes nothing.
    openGrid();
    QObject *mark = findObjects(cells().first(), QStringLiteral("closeTabMark")).first();
    QVERIFY(mark->property("opacity").toReal() < 1.0);
    const QPoint onMark = centreOf(mark);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, onMark);
    QCOMPARE(mark->property("opacity").toReal(), 1.0);
    const QPoint offMark = onMark - QPoint(3 * mark->property("width").toInt(), 0);
    QTest::mouseMove(&window, offMark);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, offMark);
    QVERIFY(mark->property("opacity").toReal() < 1.0);
    QCOMPARE(tabs->count(), 2);
    QVERIFY(page->property("tabsOpen").toBool());

    // Held still for the hold interval -- or all but still: a thumb drifts, and a
    // drift short of a drag is still a hold -- a cell comes up and is carried to
    // another place in the grid, and letting go of it opens nothing.
    openGrid();
    QObject *first = cells().first();
    const QPoint grab = centreOf(first);
    const int firstId = tabs->data(tabs->index(0, 0), TabModel::TabIdRole).toInt();
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, grab);
    QTest::mouseMove(&window, grab + QPoint(first->property("holdTolerance").toInt() / 2, 2));
    QTRY_VERIFY(first->property("held").toBool());
    const QPoint target = centreOf(cells().last());
    QTest::mouseMove(&window, (grab + target) / 2);
    QTest::mouseMove(&window, target);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, target);
    QCOMPARE(tabs->data(tabs->index(1, 0), TabModel::TabIdRole).toInt(), firstId);
    QVERIFY(page->property("tabsOpen").toBool());

    // Slid to the left, a cell closes its tab -- slanting as a thumb does, too: the
    // grid would take a slide that drifted down by its drag distance before it had
    // gone across by the hold's tolerance, and scroll or pull instead.
    const int across = cells().first()->property("width").toInt() / 2;
    const QPoint slant = centreOf(cells().first());
    drag(&window, slant, slant + QPoint(-across, across * 3 / 5));
    QCOMPARE(tabs->count(), 1);
    QVERIFY(page->property("tabsOpen").toBool());
    tabs->newTab(QStringLiteral("https://two.example/"));
    openGrid();
    const QPoint slide = centreOf(cells().first());
    drag(&window, slide, slide - QPoint(across, 0));
    QCOMPARE(tabs->count(), 1);
    QVERIFY(page->property("tabsOpen").toBool());

    // With more tabs than the screen holds, a drag begun on a cell scrolls the grid,
    // and a pull back down on one scrolls it back rather than dropping the grid.
    for (int i = 0; i < 9; ++i) {
        tabs->newTab(QStringLiteral("https://more.example/%1").arg(i));
    }
    openGrid();
    QObject *grid = find(QStringLiteral("tabGrid"));
    const qreal top = grid->property("contentY").toReal();
    const QPoint middle = centreOf(cells().at(4));
    drag(&window, middle, middle - down);
    QTRY_VERIFY(!grid->property("moving").toBool());
    const qreal scrolled = grid->property("contentY").toReal();
    QVERIFY(scrolled > top);
    const QPoint lower = centreOf(cells().at(4)) - down / 3;
    drag(&window, lower, lower + down / 3);
    QTRY_VERIFY(!grid->property("moving").toBool());
    QVERIFY(grid->property("contentY").toReal() < scrolled);
    QVERIFY(page->property("tabsOpen").toBool());
}

// A cell held until it comes up and carried down onto a name in the strip of groups
// moves its tab into that group, under a real finger: the strip lights the name the
// finger is over, and while it is over the strip the cells hidden under it are not
// traded with.
void tst_qmlload::carryToGroupUnderAFinger()
{
    TabModel *tabs = m_core->tabs();
    const int home = tabs->defaultGroupId();
    const int first = tabs->activeTabId();
    const int second = tabs->newTab(QStringLiteral("https://two.example/"));
    const int work = tabs->addGroup(QStringLiteral("Work"));
    tabs->groupModel()->activate(0);
    QCOMPARE(tabs->currentGroupId(), home);
    QCOMPARE(tabs->activeTabId(), second);
    QObject *page = find(QStringLiteral("browserPage"));
    QObject *strip = find(QStringLiteral("tabGroupStrip"));
    auto *root = qobject_cast<QQuickItem *>(m_window.data());
    FingerWindow host(root);
    QQuickWindow &window = *host.window();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    pullUpToTabs();
    QTRY_COMPARE(page->property("tabsOffset").toReal(), page->property("fullHeight").toReal());
    // A drop that moves the tab at once takes the carried cell out of the grid while
    // its own release is still being handled, and the rest of that handler throws.
    const ScriptErrors errors;

    const auto cellOf = [this](int tabId) -> QObject * {
        for (QObject *cell : findAll(QStringLiteral("tabPreview"))) {
            if (evaluate(cell, QStringLiteral("model.tabId")).toInt() == tabId) {
                return cell;
            }
        }
        return nullptr;
    };
    const auto labelOf = [this](const QString &name) -> QObject * {
        for (QObject *label : findAll(QStringLiteral("tabGroupLabel"))) {
            if (label->property("text").toString() == name) {
                return label;
            }
        }
        return nullptr;
    };
    const auto highlights = [this]() {
        int lit = 0;
        for (QObject *highlight : findAll(QStringLiteral("tabGroupDropHighlight"))) {
            lit += highlight->property("visible").toBool() ? 1 : 0;
        }
        return lit;
    };
    // Put down on a cell and held there, drifting a little; the caller waits for it
    // to come up.
    const auto press = [&window](QObject *cell) {
        const QPoint grab = centreOf(cell);
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, grab);
        QTest::mouseMove(&window, grab + QPoint(2, 2));
        return grab;
    };
    const auto carry = [&window](const QPoint &from, const QPoint &to) {
        const int steps = 12;
        for (int step = 1; step <= steps; ++step) {
            QTest::mouseMove(&window, from + (to - from) * step / steps);
        }
    };
    const QList<int> order{first, second};
    const auto tabOrder = [tabs]() {
        QList<int> ids;
        for (int row = 0; row < tabs->count(); ++row) {
            ids.append(tabs->data(tabs->index(row, 0), TabModel::TabIdRole).toInt());
        }
        return ids;
    };

    // Carried over the name of another group, the name is lit; over the strip's own
    // current group nothing is, and no cell trades places. (Between two names is the
    // nearer one's: the names' buttons tile the row.)
    QObject *workLabel = labelOf(QStringLiteral("Work"));
    QVERIFY(workLabel != nullptr);
    QObject *cell = cellOf(first);
    QPoint at = press(cell);
    QTRY_VERIFY(cell->property("held").toBool());
    const QPoint homeName = centreOf(labelOf(QStringLiteral("2 tab(s)")));
    carry(at, homeName);
    at = homeName;
    QCOMPARE(strip->property("dropIndex").toInt(), -1);
    QCOMPARE(highlights(), 0);
    const QPoint workName = centreOf(workLabel);
    carry(at, workName);
    QCOMPARE(strip->property("dropIndex").toInt(), 1);
    QCOMPARE(highlights(), 1);
    // The cells' own wash, square as theirs is.
    QCOMPARE(findAll(QStringLiteral("tabGroupDropHighlight")).at(1)->property("radius").toReal(),
             qreal(0));
    QCOMPARE(workLabel->property("color").value<QColor>(), QColor(QStringLiteral("#aaccff")));
    QCOMPARE(tabOrder(), order);

    // Let go there, and the tab is in that group. It was not the tab in front, so the
    // grid stays on its own group, one cell the fewer, and the grid stays up.
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, workName);
    QTRY_COMPARE(tabs->tabCountInGroup(work), 1);
    QCOMPARE(tabs->tabCountInGroup(home), 1);
    QCOMPARE(tabs->currentGroupId(), home);
    QCOMPARE(tabs->activeTabId(), second);
    QVERIFY(page->property("tabsOpen").toBool());
    QCOMPARE(strip->property("dropIndex").toInt(), -1);
    QTRY_COMPARE(findAll(QStringLiteral("tabPreview")).count(), 1);
    QCOMPARE(highlights(), 0);

    // Carried onto the strip and back off it, nothing stays lit, and let go among the
    // cells nothing moves group.
    cell = cellOf(second);
    at = press(cell);
    QTRY_VERIFY(cell->property("held").toBool());
    carry(at, centreOf(labelOf(QStringLiteral("Work"))));
    QCOMPARE(highlights(), 1);
    carry(centreOf(labelOf(QStringLiteral("Work"))), at);
    QCOMPARE(strip->property("dropIndex").toInt(), -1);
    QCOMPARE(highlights(), 0);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, at);
    QCOMPARE(tabs->tabCountInGroup(home), 1);
    QVERIFY(page->property("tabsOpen").toBool());

    // The tab in front carried to another group takes the grid with it: the tab in
    // front is always in the group the grid shows.
    cell = cellOf(second);
    at = press(cell);
    QTRY_VERIFY(cell->property("held").toBool());
    carry(at, centreOf(labelOf(QStringLiteral("Work"))));
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier,
                        centreOf(labelOf(QStringLiteral("Work"))));
    QTRY_COMPARE(tabs->tabCountInGroup(work), 2);
    QCOMPARE(tabs->currentGroupId(), work);
    QCOMPARE(tabs->activeTabId(), second);
    QTRY_COMPARE(findAll(QStringLiteral("tabPreview")).count(), 2);
    QVERIFY(page->property("tabsOpen").toBool());
    QVERIFY2(errors.all().isEmpty(), qPrintable(errors.all()));
}

// Over the strip the finger is choosing a group, not a place, and the carried cell
// trades with none of the cells that lie under the strip; a carry the grid takes away
// leaves nothing lit; and a name scrolled out of sight is not a place to drop a tab.
void tst_qmlload::carryOverTheStripUnderAFinger()
{
    TabModel *tabs = m_core->tabs();
    const int home = tabs->defaultGroupId();
    for (int i = 0; i < 9; ++i) {
        tabs->newTab(QStringLiteral("https://more.example/%1").arg(i));
    }
    QObject *page = find(QStringLiteral("browserPage"));
    auto *strip = qobject_cast<QQuickItem *>(find(QStringLiteral("tabGroupStrip")));
    auto *grid = qobject_cast<QQuickItem *>(find(QStringLiteral("tabGrid")));
    auto *root = qobject_cast<QQuickItem *>(m_window.data());
    FingerWindow host(root);
    QQuickWindow &window = *host.window();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    pullUpToTabs();
    QTRY_COMPARE(page->property("tabsOffset").toReal(), page->property("fullHeight").toReal());
    const ScriptErrors errors;

    const auto tabOrder = [tabs]() {
        QList<int> ids;
        for (int row = 0; row < tabs->count(); ++row) {
            ids.append(tabs->data(tabs->index(row, 0), TabModel::TabIdRole).toInt());
        }
        return ids;
    };
    const auto cellAt = [this](int index) -> QObject * {
        for (QObject *cell : findAll(QStringLiteral("tabPreview"))) {
            if (evaluate(cell, QStringLiteral("index")).toInt() == index) {
                return cell;
            }
        }
        return nullptr;
    };
    const auto cellUnder = [this, grid](const QPoint &point) {
        const QPointF inGrid = grid->mapFromScene(QPointF(point));
        return evaluate(grid, QStringLiteral("indexAt(contentX + %1, contentY + %2)")
                                  .arg(inGrid.x())
                                  .arg(inGrid.y()))
            .toInt();
    };
    const auto carry = [&window](const QPoint &from, const QPoint &to) {
        const int steps = 12;
        for (int step = 1; step <= steps; ++step) {
            QTest::mouseMove(&window, from + (to - from) * step / steps);
        }
    };
    const auto lift = [&window](QObject *cell) {
        const QPoint grab = centreOf(cell);
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, grab);
        QTest::mouseMove(&window, grab + QPoint(2, 2));
        return grab;
    };
    const int stripY = int(strip->mapToScene(QPointF(0, strip->height() / 2)).y());
    const int width = int(root->width());
    const QPoint left(width / 4, stripY);
    const QPoint right(width * 3 / 4, stripY);
    // With ten tabs, cells lie under both ends of the strip.
    QVERIFY(cellUnder(left) >= 0);
    QVERIFY(cellUnder(right) >= 0);

    // Carried straight down its column onto the strip -- trading places with the cells
    // it crosses on the way, as a carry does -- and then along the strip over the cells
    // under it, which it does not trade with.
    QObject *cell = cellAt(0);
    QPoint at = lift(cell);
    QTRY_VERIFY(cell->property("held").toBool());
    carry(at, QPoint(at.x(), stripY));
    const QList<int> onTheStrip = tabOrder();
    carry(QPoint(at.x(), stripY), right);
    QCOMPARE(tabOrder(), onTheStrip);
    QCOMPARE(strip->property("dropIndex").toInt(), -1);
    // The corners are the strip's too: over the edit corner -- past the names, over a
    // cell other than the carried one -- no cell is traded with, and nothing is lit.
    const QPoint corner = centreOf(find(QStringLiteral("editGroupsButton")));
    QCOMPARE(corner.y(), stripY);
    QVERIFY(cellUnder(corner) >= 0);
    QVERIFY(cellUnder(corner) != evaluate(cell, QStringLiteral("index")).toInt());
    carry(right, corner);
    QCOMPARE(tabOrder(), onTheStrip);
    QCOMPARE(strip->property("dropIndex").toInt(), -1);
    // Dropped there, with nothing lit, the tab stays in its group, the corner's button
    // is not pressed and the grid stays up.
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, corner);
    QCoreApplication::processEvents();
    QCOMPARE(tabs->tabCountInGroup(home), 10);
    QCOMPARE(currentPage()->objectName(), QStringLiteral("browserPage"));
    QVERIFY(page->property("tabsOpen").toBool());

    // A carry the grid takes away mid-way leaves nothing lit, and nothing moves.
    const int work = tabs->addGroup(QStringLiteral("Work"));
    tabs->groupModel()->activate(0);
    QTRY_COMPARE(findAll(QStringLiteral("tabGroupLabel")).count(), 2);
    QObject *workLabel = findAll(QStringLiteral("tabGroupLabel")).at(1);
    QTRY_VERIFY(centreOf(workLabel).x() > width / 2);
    cell = cellAt(0);
    at = lift(cell);
    QTRY_VERIFY(cell->property("held").toBool());
    carry(at, centreOf(workLabel));
    QCOMPARE(strip->property("dropIndex").toInt(), 1);
    window.mouseGrabberItem()->ungrabMouse();
    QVERIFY(!cell->property("held").toBool());
    QCOMPARE(strip->property("dropIndex").toInt(), -1);
    QVERIFY(!findAll(QStringLiteral("tabGroupDropHighlight")).at(1)->property("visible").toBool());
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, centreOf(workLabel));
    QCoreApplication::processEvents();
    QCOMPARE(tabs->tabCountInGroup(work), 0);

    // With more names than the strip shows, a point of the strip past the end of the
    // names -- where the ones scrolled out of sight lie, clipped -- lights nothing.
    for (int i = 0; i < 8; ++i) {
        tabs->addGroup(QStringLiteral("Group number %1").arg(i));
    }
    tabs->groupModel()->activate(0);
    auto *names = qobject_cast<QQuickItem *>(find(QStringLiteral("tabGroupList")));
    QTRY_VERIFY(names->property("interactive").toBool());
    QTRY_COMPARE(names->property("contentX").toReal(), qreal(0));
    const QPointF namesEnd = names->mapToScene(QPointF(names->width(), names->height() / 2));
    const QPoint pastTheNames(int(namesEnd.x()) + 10, int(namesEnd.y()));
    QVERIFY(pastTheNames.x() < width);
    cell = cellAt(0);
    at = lift(cell);
    QTRY_VERIFY(cell->property("held").toBool());
    carry(at, centreOf(workLabel));
    QCOMPARE(strip->property("dropIndex").toInt(), 1);
    carry(centreOf(workLabel), pastTheNames);
    QCOMPARE(strip->property("dropIndex").toInt(), -1);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, pastTheNames);
    QCoreApplication::processEvents();
    QCOMPARE(tabs->tabCountInGroup(home), 10);
    QVERIFY2(errors.all().isEmpty(), qPrintable(errors.all()));
}

// A strip with more names than it shows fades out at an end with names past it, and
// only there, and not by much; a row of names that fits has no fade at all.
void tst_qmlload::tabGroupStripFades()
{
    TabModel *tabs = m_core->tabs();
    // In a window: the names are laid out in a row, and a row lays out when drawn.
    FingerWindow host(m_window.data());
    QVERIFY(QTest::qWaitForWindowExposed(host.window()));
    pullUpToTabs();
    QObject *left = find(QStringLiteral("tabGroupLeftFade"));
    QObject *right = find(QStringLiteral("tabGroupRightFade"));
    auto *names = qobject_cast<QQuickItem *>(find(QStringLiteral("tabGroupList")));
    QVERIFY(left != nullptr);
    QVERIFY(right != nullptr);
    QVERIFY(!left->property("enabled").toBool());
    QVERIFY(!right->property("enabled").toBool());

    for (int i = 0; i < 8; ++i) {
        tabs->addGroup(QStringLiteral("Group number %1").arg(i));
    }
    // At the first name only the far end fades, drawn from the names themselves: the
    // ramp that is opaque on the left.
    tabs->groupModel()->activate(0);
    QTRY_VERIFY(names->property("interactive").toBool());
    QTRY_COMPARE(names->property("contentX").toReal(), qreal(0));
    QVERIFY(!left->property("enabled").toBool());
    QVERIFY(right->property("enabled").toBool());
    QCOMPARE(right->property("sourceItem").value<QObject *>(), names);
    QCOMPARE(right->property("direction").toInt(), 0);
    // Over a twentieth of the screen at most, fading to nothing at the very edge.
    const qreal slope = right->property("slope").toReal();
    const qreal screenWidth = evaluate(names, QStringLiteral("Screen.width")).toReal();
    QVERIFY(names->width() / slope <= screenWidth / 20);
    QCOMPARE(right->property("offset").toReal(), 1 - 1 / slope);

    // Among the names, both ends, the second ramp drawn from the first.
    tabs->groupModel()->activate(4);
    QTRY_VERIFY(left->property("enabled").toBool());
    QVERIFY(right->property("enabled").toBool());
    QCOMPARE(right->property("sourceItem").value<QObject *>(), left);
    QCOMPARE(left->property("sourceItem").value<QObject *>(), names);
    QCOMPARE(left->property("direction").toInt(), 1);

    // At the last name, only the near end.
    tabs->groupModel()->activate(8);
    QTRY_VERIFY(!right->property("enabled").toBool());
    QVERIFY(left->property("enabled").toBool());
    QCOMPARE(names->property("contentX").toReal(),
             names->property("contentWidth").toReal() - names->width());
}

// The reach above the navigation bar lies over the foot of the page, where a player
// keeps its seek bar and its buttons. It keeps the one thing it is there for -- a drag
// upwards, which opens the grid -- and hands the page everything else: a tap, a drag
// sideways or down. A press held there is kept, since the handle sits in the reach.
void tst_qmlload::barReachUnderAFinger()
{
    FingerWindow host(m_window.data());
    QQuickWindow &window = *host.window();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QObject *page = find(QStringLiteral("browserPage"));
    auto *view = qobject_cast<QQuickItem *>(currentWebView());
    auto *gesture = qobject_cast<QQuickItem *>(find(QStringLiteral("navigationBarGesture")));
    const qreal reach = gesture->property("reach").toReal();
    const QPointF gestureTop = gesture->mapToScene(QPointF(0, 0));
    const int inReach = int(gestureTop.y() + reach / 2);
    const int onBar = int(gestureTop.y() + reach + gesture->property("strip").toReal() / 2);
    const auto touches = [view]() { return view->property("touches").toList(); };
    const auto touch = [&](int i) { return touches().at(i).toMap(); };
    const int shake = evaluate(page, QStringLiteral("Theme.startDragDistance")).toInt();

    // A tap goes down and up on the page where the finger was, in the view's own
    // coordinates, and the view has the focus a real touch would have given it.
    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, QPoint(200, inReach));
    QCOMPARE(touches().count(), 2);
    QCOMPARE(touch(0).value(QStringLiteral("phase")).toString(), QStringLiteral("begin"));
    QCOMPARE(touch(1).value(QStringLiteral("phase")).toString(), QStringLiteral("end"));
    const QPointF local = view->mapFromScene(QPointF(200, inReach));
    QCOMPARE(touch(0).value(QStringLiteral("x")).toReal(), local.x());
    QCOMPARE(touch(0).value(QStringLiteral("y")).toReal(), local.y());
    QVERIFY(local.y() > view->height() - reach && local.y() < view->height());
    QVERIFY(view->hasActiveFocus());
    QVERIFY(!page->property("tabsOpen").toBool());

    // A drag along a seek bar goes to the page from where it started, move by move.
    drag(&window, QPoint(200, inReach), QPoint(700, inReach));
    QCOMPARE(touch(2).value(QStringLiteral("phase")).toString(), QStringLiteral("begin"));
    QCOMPARE(touch(2).value(QStringLiteral("x")).toReal(), local.x());
    QVERIFY(touches().count() > 5);
    QCOMPARE(touches().last().toMap().value(QStringLiteral("phase")).toString(),
             QStringLiteral("end"));
    QCOMPARE(touches().last().toMap().value(QStringLiteral("x")).toReal(),
             view->mapFromScene(QPointF(700, inReach)).x());
    QVERIFY(!page->property("tabsOpen").toBool());

    // So does one downwards, and a shake smaller than a drag is still a tap.
    int before = touches().count();
    drag(&window, QPoint(300, int(gestureTop.y()) + 1), QPoint(300, inReach + shake));
    QVERIFY(touches().count() > before + 2);
    before = touches().count();
    drag(&window, QPoint(300, inReach), QPoint(300 + shake / 2, inReach));
    QCOMPARE(touches().count(), before + 2);

    // A press held, and a tap on the bar itself, are not the page's.
    before = touches().count();
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(300, inReach));
    QTRY_VERIFY(gesture->property("heldDown").toBool());
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(300, inReach));
    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier,
                      QPoint(int(gesture->width()) / 2, onBar));
    QCOMPARE(touches().count(), before);
    QVERIFY(find(QStringLiteral("navigationBar"))->property("editing").toBool());
    evaluate(find(QStringLiteral("navigationBar")), QStringLiteral("endEditing()"));
    // On the bar itself a slow tap is a tap: the hold is the reach's alone.
    const QPoint address(int(gesture->width()) / 2, onBar);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, address);
    QTest::qWait(QGuiApplication::styleHints()->mousePressAndHoldInterval() + 200);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, address);
    QVERIFY(find(QStringLiteral("navigationBar"))->property("editing").toBool());
    evaluate(find(QStringLiteral("navigationBar")), QStringLiteral("endEditing()"));
    QCOMPARE(touches().count(), before);

    // A drag upwards from the reach is still the one that opens the grid.
    drag(&window, QPoint(540, inReach),
         QPoint(540, inReach - 3 * page->property("pullThreshold").toInt()));
    QVERIFY(page->property("tabsOpen").toBool());
    QCOMPARE(touches().count(), before);
}

void tst_qmlload::recentlyClosedTabs()
{
    TabModel *tabs = m_core->tabs();
    const int second = tabs->newTab(QStringLiteral("https://two.example/"));
    tabs->updateTitle(second, QStringLiteral("Two"));
    tabs->closeTabById(second);
    QCOMPARE(tabs->closedTabs()->count(), 1);
    pullUpToTabs();

    // Holding the new-tab button, in the corner of the grid's foot, brings the panel up
    // from under it.
    QObject *panel = find(QStringLiteral("recentlyClosedPanel"));
    QVERIFY(panel != nullptr);
    QVERIFY(!panel->property("open").toBool());
    QVERIFY(panel->property("modal").toBool());
    QMetaObject::invokeMethod(find(QStringLiteral("newTabButton")), "pressAndHold");
    QVERIFY(panel->property("open").toBool());
    QList<QObject *> rows = findAll(QStringLiteral("closedTabDelegate"));
    QCOMPARE(rows.count(), 1);
    QCOMPARE(findObjects(rows.first(), QStringLiteral("tabRowTitle"))
                 .first()
                 ->property("text")
                 .toString(),
             QStringLiteral("Two"));
    QCOMPARE(findObjects(rows.first(), QStringLiteral("tabRowSubtitle"))
                 .first()
                 ->property("text")
                 .toString(),
             QStringLiteral("https://two.example/"));

    // A tap opens the tab again with what it had, puts the panel away and hands
    // the page back.
    click(rows.first());
    QCOMPARE(tabs->count(), 2);
    QCOMPARE(tabs->activeUrl(), QStringLiteral("https://two.example/"));
    QCOMPARE(tabs->activeTitle(), QStringLiteral("Two"));
    QVERIFY(!panel->property("open").toBool());
    QVERIFY(!find(QStringLiteral("browserPage"))->property("tabsOpen").toBool());
    QCOMPARE(tabs->closedTabs()->count(), 0);
    QCOMPARE(findAll(QStringLiteral("closedTabDelegate")).count(), 0);
}

void tst_qmlload::pagesBeyondTheLimitUnload()
{
    TabModel *tabs = m_core->tabs();
    // Five by default, and the main page of Settings offers the choice.
    QCOMPARE(tabs->liveTabLimit(), 5);
    QCOMPARE(openMenuItem(QStringLiteral("settingsMenuButton"))->objectName(),
             QStringLiteral("settingsPage"));
    QObject *combo = find(QStringLiteral("liveTabLimitCombo"));
    QCOMPARE(combo->property("currentIndex").toInt(), 1);
    combo->setProperty("currentIndex", 0);
    QCOMPARE(m_core->settings()->liveTabLimit(), 3);
    QCOMPARE(tabs->liveTabLimit(), 3);

    // Ten minutes in the background, and the engine is asked to trim its heap with
    // the words sailfish-browser uses. Read through something of BrowserPage.qml's
    // own, whose scope has the engine singleton the browsing page's inline component
    // has not.
    QObject *page = find(QStringLiteral("browserPage"));
    QObject *pageScope = find(QStringLiteral("viewArea"));
    QCOMPARE(find(QStringLiteral("trimTimer"))->property("interval").toInt(), 600000);
    QCOMPARE(evaluate(pageScope, QStringLiteral("WebEngine.notifications.length")).toInt(), 0);
    evaluate(page, QStringLiteral("trimMemory()"));
    QCOMPARE(evaluate(pageScope, QStringLiteral("WebEngine.notifications.length")).toInt(), 1);
    QCOMPARE(evaluate(pageScope, QStringLiteral("WebEngine.notifications[0].topic")).toString(),
             QStringLiteral("memory-pressure"));
    QCOMPARE(evaluate(pageScope, QStringLiteral("WebEngine.notifications[0].value")).toString(),
             QStringLiteral("heap-minimize"));
    popPage();

    const int first = tabs->activeTabId();
    tabs->newTab(QStringLiteral("https://two.example/"));
    tabs->newTab(QStringLiteral("https://three.example/"));
    QCOMPARE(findAll(QStringLiteral("webView")).count(), 3);
    // A fourth, and the one read least recently gives its view up.
    tabs->newTab(QStringLiteral("https://four.example/"));
    QCOMPARE(findAll(QStringLiteral("webView")).count(), 3);
    QList<QObject *> loaders = findAll(QStringLiteral("webViewLoader"));
    QCOMPARE(loaders.count(), 4);
    QVERIFY(!loaders.at(0)->property("active").toBool());
    QVERIFY(loaders.at(3)->property("active").toBool());

    // Back in front it is loaded again, from the page it was on, and the next least
    // recent goes instead.
    tabs->activateTabById(first);
    QCOMPARE(findAll(QStringLiteral("webView")).count(), 3);
    QVERIFY(loaders.at(0)->property("active").toBool());
    QVERIFY(!loaders.at(1)->property("active").toBool());
    QCOMPARE(currentWebView()->property("url").toUrl().toString(), Settings::defaultHomePage());
}

void tst_qmlload::restoredTabsLoadLazily()
{
    m_core->tabs()->newTab(QStringLiteral("https://two.example/"));
    m_core->tabs()->newTab(QStringLiteral("https://three.example/"));
    m_core->tabs()->activateTab(1);

    m_window.reset();
    m_engine.reset();
    m_core.reset(new Core(m_dir->path(), m_dir->path() + QStringLiteral("/salama.conf"),
                          m_dir->path() + QStringLiteral("/Downloads/Salama")));
    QVERIFY(loadWindow());

    QCOMPARE(m_core->tabs()->count(), 3);
    QCOMPARE(findAll(QStringLiteral("webViewLoader")).count(), 3);
    QCOMPARE(findAll(QStringLiteral("webView")).count(), 1);
    QCOMPARE(currentWebView()->property("url").toUrl().toString(),
             QStringLiteral("https://two.example/"));
    // The three pages were visited in the first session; restoring adds no visits.
    QCOMPARE(m_core->history()->count(), 3);

    m_core->tabs()->activateTab(2);
    QCOMPARE(findAll(QStringLiteral("webView")).count(), 2);
    QCOMPARE(currentWebView()->property("url").toUrl().toString(),
             QStringLiteral("https://three.example/"));
    QCOMPARE(m_core->history()->count(), 3);
}

// The bar's menu button brings up a sheet of icons from under the bar, in two rows:
// the page in front, the browser. Nothing is pushed for it, and each icon puts it
// away as it does what it says. A new tab is not on it: that is the grid's plus.
void tst_qmlload::browserMenu()
{
    TabModel *tabs = m_core->tabs();
    QObject *menu = find(QStringLiteral("browserMenu"));
    QVERIFY(menu != nullptr);
    QVERIFY(!menu->property("open").toBool());
    QVERIFY(menu->property("modal").toBool());
    QCOMPARE(menu->property("dock").toInt(), 2); // Dock.Bottom
    tapBar(QStringLiteral("menu"));
    QVERIFY(menu->property("open").toBool());
    QCOMPARE(currentPage()->objectName(), QStringLiteral("browserPage"));
    const QStringList entries{
        QStringLiteral("findMenuButton"),      QStringLiteral("bookmarkMenuButton"),
        QStringLiteral("shareMenuButton"),     QStringLiteral("desktopMenuButton"),
        QStringLiteral("bookmarksMenuButton"), QStringLiteral("historyMenuButton"),
        QStringLiteral("downloadsMenuButton"), QStringLiteral("settingsMenuButton"),
    };
    for (const QString &entry : entries) {
        QObject *button = find(entry);
        QVERIFY2(button != nullptr, qPrintable(entry));
        QVERIFY2(button->property("enabled").toBool(), qPrintable(entry));
        QVERIFY2(!button->property("iconSource").toString().isEmpty(), qPrintable(entry));
        QVERIFY2(!button->property("text").toString().isEmpty(), qPrintable(entry));
    }
    // In the two rows asked for: the page in front, the browser. The row of the tabs,
    // which held New tab alone, is gone with it.
    QVERIFY(find(QStringLiteral("newTabMenuButton")) == nullptr);
    QVERIFY(find(QStringLiteral("menuTabsRow")) == nullptr);
    const QHash<QString, QString> rows{
        {QStringLiteral("findMenuButton"), QStringLiteral("menuPageRow")},
        {QStringLiteral("bookmarkMenuButton"), QStringLiteral("menuPageRow")},
        {QStringLiteral("shareMenuButton"), QStringLiteral("menuPageRow")},
        {QStringLiteral("desktopMenuButton"), QStringLiteral("menuPageRow")},
        {QStringLiteral("readerMenuButton"), QStringLiteral("menuPageRow")},
        {QStringLiteral("bookmarksMenuButton"), QStringLiteral("menuBrowserRow")},
        {QStringLiteral("historyMenuButton"), QStringLiteral("menuBrowserRow")},
        {QStringLiteral("downloadsMenuButton"), QStringLiteral("menuBrowserRow")},
        {QStringLiteral("settingsMenuButton"), QStringLiteral("menuBrowserRow")},
    };
    for (auto it = rows.cbegin(); it != rows.cend(); ++it) {
        QCOMPARE(qobject_cast<QQuickItem *>(find(it.key()))->parentItem()->objectName(),
                 it.value());
    }
    // The reader view is offered only for a page that reads as an article, which a
    // search engine's front page does not (readerView()).
    QObject *reader = find(QStringLiteral("readerMenuButton"));
    QVERIFY(reader != nullptr);
    QVERIFY(!reader->property("enabled").toBool());
    QVERIFY(!reader->property("iconSource").toString().isEmpty());
    QCOMPARE(reader->property("text").toString(), QStringLiteral("Reader view"));
    // Searching the page and its desktop version need the page's view, and wait for it;
    // bookmarking and sharing need only its address.
    // Through something of BrowserPage.qml's own, whose scope names the page and the menu.
    QObject *pageScope = find(QStringLiteral("viewArea"));
    evaluate(pageScope, QStringLiteral("browserMenu.view = null"));
    QVERIFY(!find(QStringLiteral("findMenuButton"))->property("enabled").toBool());
    QVERIFY(!find(QStringLiteral("desktopMenuButton"))->property("enabled").toBool());
    QVERIFY(!reader->property("enabled").toBool());
    QVERIFY(find(QStringLiteral("bookmarkMenuButton"))->property("enabled").toBool());
    QVERIFY(find(QStringLiteral("shareMenuButton"))->property("enabled").toBool());
    evaluate(pageScope, QStringLiteral("browserMenu.view = Qt.binding(function () {"
                                       " return browserPage.currentView })"));
    QCOMPARE(menu->property("view").value<QObject *>(), currentWebView());
    // What the old page of the menu also had is not here: the grid is the bar's drag,
    // and a tab changes group by being carried onto one.
    QVERIFY(find(QStringLiteral("tabsItem")) == nullptr);
    QVERIFY(find(QStringLiteral("moveToGroupItem")) == nullptr);

    // A new tab is the plus at the grid's foot, which still opens the home page in one.
    QMetaObject::invokeMethod(menu, "hide");
    pullUpToTabs();
    click(find(QStringLiteral("newTabButton")));
    QCOMPARE(tabs->count(), 2);
    QCOMPARE(tabs->activeUrl(), Settings::defaultHomePage());
    QVERIFY(!find(QStringLiteral("browserPage"))->property("tabsOpen").toBool());
    QVERIFY(!menu->property("open").toBool());

    // Bookmarking is a switch, which says which way it goes.
    tapBar(QStringLiteral("menu"));
    QObject *bookmark = find(QStringLiteral("bookmarkMenuButton"));
    QVERIFY(!bookmark->property("checked").toBool());
    QCOMPARE(bookmark->property("text").toString(), QStringLiteral("Bookmark"));
    click(bookmark);
    QCOMPARE(m_core->bookmarks()->count(), 1);
    QVERIFY(m_core->bookmarks()->activeUrlBookmarked());
    QVERIFY(!menu->property("open").toBool());
    tapBar(QStringLiteral("menu"));
    QVERIFY(bookmark->property("checked").toBool());
    QCOMPARE(bookmark->property("text").toString(), QStringLiteral("Remove bookmark"));
    click(bookmark);
    QCOMPARE(m_core->bookmarks()->count(), 0);

    // Share sends the address.
    tapBar(QStringLiteral("menu"));
    QObject *share = find(QStringLiteral("shareAction"));
    click(find(QStringLiteral("shareMenuButton")));
    QCOMPARE(share->property("triggerCount").toInt(), 1);
    QCOMPARE(share->property("mimeType").toString(), QStringLiteral("text/x-url"));
    const QVariantMap resource = share->property("resources").toList().first().toMap();
    QCOMPARE(resource.value(QStringLiteral("status")).toString(), tabs->activeUrl());
    QVERIFY(!menu->property("open").toBool());

    // The desktop version is the page in front's alone, and the switch says which the
    // page in front is in.
    QObject *front = currentWebView();
    QObject *desktop = find(QStringLiteral("desktopMenuButton"));
    QVERIFY(!front->property("desktopMode").toBool());
    tapBar(QStringLiteral("menu"));
    QVERIFY(!desktop->property("checked").toBool());
    click(desktop);
    QVERIFY(front->property("desktopMode").toBool());
    QVERIFY(!menu->property("open").toBool());
    QVERIFY(desktop->property("checked").toBool());
    const int newest = tabs->activeTabId();
    tabs->activateTab(0);
    QVERIFY(currentWebView() != front);
    QVERIFY(!currentWebView()->property("desktopMode").toBool());
    QVERIFY(!desktop->property("checked").toBool());
    tabs->activateTabById(newest);
    QVERIFY(desktop->property("checked").toBool());
    // And back.
    tapBar(QStringLiteral("menu"));
    click(desktop);
    QVERIFY(!front->property("desktopMode").toBool());

    // The browser's own pages go over the browsing page, which stays under them.
    const QList<QPair<QString, QString>> pages{
        {QStringLiteral("bookmarksMenuButton"), QStringLiteral("bookmarksPage")},
        {QStringLiteral("historyMenuButton"), QStringLiteral("historyPage")},
        {QStringLiteral("downloadsMenuButton"), QStringLiteral("downloadsPage")},
        {QStringLiteral("settingsMenuButton"), QStringLiteral("settingsPage")},
    };
    for (const auto &entry : pages) {
        QObject *opened = openMenuItem(entry.first);
        QVERIFY2(opened != nullptr, qPrintable(entry.first));
        QCOMPARE(opened->objectName(), entry.second);
        QVERIFY(!menu->property("open").toBool());
        QCOMPARE(pageStack()->property("depth").toInt(), 2);
        popPage();
        QCOMPARE(currentPage()->objectName(), QStringLiteral("browserPage"));
    }

    // The sheet is about the page in front, and another page in front puts it away:
    // the cover's new tab, say.
    tapBar(QStringLiteral("menu"));
    tabs->newTab(QStringLiteral("https://three.example/"));
    QVERIFY(!menu->property("open").toBool());
}

// The sheet of icons goes back down under a finger that pulls it, begun on an icon as
// much as anywhere, and keeps the icon under the finger: let go past a short distance
// it goes away, short of it it comes back up. The stub icons take no presses, so what
// this proves is the sheet's own pull; Silica's buttons giving a drag up to it is the
// device's to show (docs/TESTING.md).
void tst_qmlload::menuSheetUnderAFinger()
{
    auto *root = qobject_cast<QQuickItem *>(m_window.data());
    FingerWindow host(root);
    QQuickWindow &window = *host.window();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto *menu = qobject_cast<QQuickItem *>(find(QStringLiteral("browserMenu")));
    auto *page = qobject_cast<QQuickItem *>(find(QStringLiteral("browserPage")));
    tapBar(QStringLiteral("menu"));
    QVERIFY(menu->property("open").toBool());
    const qreal openY = page->height() - menu->height();
    QCOMPARE(menu->y(), openY);
    const qreal closeDistance = menu->property("closeDistance").toReal();
    QVERIFY(closeDistance > 0);
    QVERIFY(closeDistance <= menu->height() / 3);
    auto *icon = qobject_cast<QQuickItem *>(find(QStringLiteral("historyMenuButton")));
    const QPoint grab = centreOf(icon);
    const qreal iconY = icon->mapToScene(QPointF(0, 0)).y();
    const int slack = QGuiApplication::styleHints()->startDragDistance() + 1;
    const auto pullTo = [&window, grab](int distance) {
        const int steps = 12;
        for (int step = 1; step <= steps; ++step) {
            QTest::mouseMove(&window, grab + QPoint(0, distance * step / steps));
        }
    };

    // Short of the distance: the sheet goes down with the finger, as far as the finger
    // less the way a drag takes to start -- not half of it, which is what the flickable
    // itself draws -- and the icon with it. It comes back up when the finger lifts.
    const int shortPull = int(closeDistance / 2);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, grab);
    pullTo(shortPull);
    const qreal travelled = menu->y() - openY;
    QVERIFY(travelled <= shortPull);
    QVERIFY2(travelled >= shortPull - 2 * slack, qPrintable(QString::number(travelled)));
    QVERIFY(qAbs(icon->mapToScene(QPointF(0, 0)).y() - iconY - travelled) < 1);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, grab + QPoint(0, shortPull));
    QTRY_COMPARE(menu->y(), openY);
    QVERIFY(menu->property("open").toBool());
    QCOMPARE(icon->mapToScene(QPointF(0, 0)).y(), iconY);

    // Past it, the sheet is put away.
    const int longPull = int(closeDistance * 2);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, grab);
    pullTo(longPull);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, grab + QPoint(0, longPull));
    QVERIFY(!menu->property("open").toBool());
    QCOMPARE(currentPage()->objectName(), QStringLiteral("browserPage"));

    // Open again, it sits where it sat, whole.
    tapBar(QStringLiteral("menu"));
    QTRY_COMPARE(menu->y(), openY);
    QTRY_COMPARE(find(QStringLiteral("menuSheet"))->property("y").toReal(), qreal(0));
}

// Search on page: a field over the navigation bar, whose search and steps are the
// engine's own messages to the page, and whose answers come back on the name the
// page was told to listen for.
void tst_qmlload::findInPage()
{
    QObject *view = currentWebView();
    QObject *bar = find(QStringLiteral("findBar"));
    QObject *navigation = find(QStringLiteral("navigationBar"));
    const auto lastMessage = [](QObject *of) {
        return of->property("messages").toList().last().toMap();
    };
    const auto request = [&lastMessage](QObject *of) {
        return lastMessage(of).value(QStringLiteral("data")).toMap();
    };

    // Every page listens for the answer from the start.
    QVERIFY(
        view->property("messageListeners").toStringList().contains(QStringLiteral("embed:find")));
    QVERIFY(!bar->property("visible").toBool());

    QObject *menu = find(QStringLiteral("browserMenu"));
    tapBar(QStringLiteral("menu"));
    click(find(QStringLiteral("findMenuButton")));
    QVERIFY(!menu->property("open").toBool());
    QVERIFY(bar->property("active").toBool());
    QVERIFY(bar->property("visible").toBool());
    // Over the navigation bar, which stays whole under it however the page scrolls.
    QCOMPARE(bar->property("y").toReal(), navigation->property("y").toReal());
    QCOMPARE(bar->property("height").toReal(), navigation->property("height").toReal());
    view->setProperty("chrome", false);
    QVERIFY(!navigation->property("compact").toBool());
    view->setProperty("chrome", true);
    QVERIFY(find(QStringLiteral("findPreviousButton"))->property("enabled").toBool() == false);

    // Enter searches from the top of the page.
    QObject *field = find(QStringLiteral("findField"));
    field->setProperty("text", QStringLiteral("salama"));
    enterKey(field);
    QCOMPARE(lastMessage(view).value(QStringLiteral("name")).toString(),
             QStringLiteral("embedui:find"));
    QCOMPARE(request(view).value(QStringLiteral("text")).toString(), QStringLiteral("salama"));
    QVERIFY(!request(view).value(QStringLiteral("again")).toBool());
    QVERIFY(!request(view).value(QStringLiteral("backwards")).toBool());

    // The arrows step on from there, either way.
    click(find(QStringLiteral("findNextButton")));
    QVERIFY(request(view).value(QStringLiteral("again")).toBool());
    QVERIFY(!request(view).value(QStringLiteral("backwards")).toBool());
    click(find(QStringLiteral("findPreviousButton")));
    QVERIFY(request(view).value(QStringLiteral("again")).toBool());
    QVERIFY(request(view).value(QStringLiteral("backwards")).toBool());
    QCOMPARE(request(view).value(QStringLiteral("text")).toString(), QStringLiteral("salama"));

    // The page answers; the field says when there is nothing to find. Another
    // message is not an answer.
    const auto answer = [view](const QString &name, int result) {
        const QVariant data = QVariantMap{{QStringLiteral("r"), result}};
        QMetaObject::invokeMethod(view, "recvAsyncMessage", Q_ARG(QString, name),
                                  Q_ARG(QVariant, data));
    };
    QVERIFY(!field->property("errorHighlight").toBool());
    answer(QStringLiteral("embed:find"), 1);
    QVERIFY(!bar->property("found").toBool());
    QVERIFY(field->property("errorHighlight").toBool());
    answer(QStringLiteral("embed:other"), 0);
    QVERIFY(!bar->property("found").toBool());
    answer(QStringLiteral("embed:find"), 2);
    QVERIFY(bar->property("found").toBool());

    // Closed, the page is told the search is over, which takes its highlight away.
    click(find(QStringLiteral("findCloseButton")));
    QVERIFY(!bar->property("active").toBool());
    QCOMPARE(request(view).value(QStringLiteral("text")).toString(), QString());
    const int sent = view->property("messages").toList().count();

    // Another page in front ends the search, on the page that was searched.
    tapBar(QStringLiteral("menu"));
    click(find(QStringLiteral("findMenuButton")));
    QVERIFY(bar->property("active").toBool());
    m_core->tabs()->newTab(QStringLiteral("https://two.example/"));
    QVERIFY(currentWebView() != view);
    QVERIFY(!bar->property("active").toBool());
    QCOMPARE(view->property("messages").toList().count(), sent + 1);
    QCOMPARE(request(view).value(QStringLiteral("text")).toString(), QString());
}

// The list of downloads is the browser's own, fed by what the engine says of them on
// the topic the browsing page subscribes to.
// The reader view, as Firefox has it (docs/DECISIONS/0024-reader-view.md): offered for a
// page Readability says reads as an article, opened as a page of its own in the view's
// history, and left with back -- the tab, the bar and the history keeping the article's
// own address throughout. The stub view answers each script as the page would.
void tst_qmlload::readerView()
{
    TabModel *tabs = m_core->tabs();
    const Salama::Reader *engine = m_core->reader();
    QObject *webView = currentWebView();
    auto *reader = webView->property("reader").value<QObject *>();
    QVERIFY(reader != nullptr);
    QObject *button = find(QStringLiteral("readerMenuButton"));
    QObject *menu = find(QStringLiteral("browserMenu"));
    const QString story = QStringLiteral("https://example.com/2026/a-story");
    const QString article = QString::fromUtf8(
        QJsonDocument(QJsonObject{
                          {QStringLiteral("title"), QStringLiteral("A story")},
                          {QStringLiteral("byline"), QStringLiteral("A. Writer")},
                          {QStringLiteral("lang"), QStringLiteral("en")},
                          {QStringLiteral("content"), QStringLiteral("<p>Once upon a time.</p>")},
                          {QStringLiteral("length"), 2000},
                      })
            .toJson(QJsonDocument::Compact));
    // What the page answers Readability's quick look, and its parse: a JavaScript
    // expression each, the article as a string literal.
    const auto answer = [&](const QString &readerable, const QString &parsed) {
        evaluate(webView, QStringLiteral("answer = function (script) {"
                                         " if (script === Reader.readerableScript) { return %1 }"
                                         " if (script === Reader.articleScript) { return %2 }"
                                         " return '' }")
                              .arg(readerable, parsed));
    };
    const QString articleLiteral =
        QString::fromUtf8(QJsonDocument(QJsonArray{article}).toJson(QJsonDocument::Compact)) +
        QStringLiteral("[0]");
    const auto load = [webView]() {
        webView->setProperty("loading", true);
        webView->setProperty("loading", false);
    };
    const auto runs = [webView](const QString &script) {
        return webView->property("scripts").toStringList().count(script);
    };
    const auto calls = [webView](const QString &call) {
        return webView->property("calls").toStringList().count(call);
    };

    // A site's front page is not looked at at all.
    answer(QStringLiteral("true"), articleLiteral);
    load();
    QCOMPARE(runs(engine->readerableScript()), 0);
    QVERIFY(!reader->property("readerable").toBool());

    // An article is, once it has loaded, and offered if Readability says so.
    answer(QStringLiteral("false"), articleLiteral);
    webView->setProperty("url", QUrl(story));
    load();
    QVERIFY(runs(engine->readerableScript()) > 0);
    QVERIFY(!reader->property("readerable").toBool());
    QVERIFY(!button->property("enabled").toBool());
    answer(QStringLiteral("true"), articleLiteral);
    load();
    QVERIFY(reader->property("readerable").toBool());
    QVERIFY(button->property("enabled").toBool());
    QVERIFY(!button->property("checked").toBool());
    QCOMPARE(tabs->activeUrl(), story);
    const int visits = m_core->history()->count();

    // Opened: Readability over the page, and the reader view loaded in its place.
    tapBar(QStringLiteral("menu"));
    click(button);
    QVERIFY(!menu->property("open").toBool());
    QCOMPARE(runs(engine->articleScript()), 1);
    QCOMPARE(calls(QStringLiteral("loadHtml")), 1);
    const QString html = webView->property("lastHtml").toString();
    QVERIFY(html.contains(QLatin1String("<h1 class=\"reader-title\">A story</h1>")));
    QVERIFY(html.contains(QLatin1String("<p>Once upon a time.</p>")));
    const QUrl readerUrl = webView->property("url").toUrl();
    QCOMPARE(readerUrl.scheme(), QStringLiteral("data"));
    QVERIFY(reader->property("active").toBool());
    QCOMPARE(reader->property("source").toString(), story);
    QVERIFY(button->property("enabled").toBool());
    QVERIFY(button->property("checked").toBool());
    // The tab, and so the bar and the history, keep the article's address.
    QCOMPARE(tabs->activeUrl(), story);
    QCOMPARE(m_core->history()->count(), visits);
    // The engine calls the reader view's document insecure, which it is not: it came
    // over no connection. The bar does not warn of a broken https connection for it.
    QObject *bar = find(QStringLiteral("navigationBar"));
    evaluate(webView, QStringLiteral("security.allGood = false"));
    QVERIFY(!bar->property("tlsBroken").toBool());
    // Loaded, it is asked for its icon, which is the article site's; it is not asked
    // whether it reads as an article.
    const int checks = runs(engine->readerableScript());
    load();
    QCOMPARE(runs(engine->readerableScript()), checks);
    QCOMPARE(tabs->activeFavicon(), QStringLiteral("https://example.com/favicon.ico"));

    // The settings restyle it where it is, in the ambience's colours until others are
    // chosen: the stub's ambience is a dark one.
    QCOMPARE(runs(engine->styleScript(true)), 0);
    m_core->settings()->setReaderColors(Settings::ReaderSepia);
    QCOMPARE(runs(engine->styleScript(true)), 1);
    QVERIFY(engine->styleScript(true).contains(QLatin1String("'sepia sans-serif'")));
    QCOMPARE(calls(QStringLiteral("loadHtml")), 1);

    // Closed: back, as Firefox goes back to the page it was opened from.
    webView->setProperty("canGoBack", true);
    tapBar(QStringLiteral("menu"));
    click(button);
    QCOMPARE(calls(QStringLiteral("goBack")), 1);
    webView->setProperty("url", QUrl(story));
    QVERIFY(!reader->property("active").toBool());
    QVERIFY(!button->property("checked").toBool());
    // The page's own connection is the page's own verdict again.
    QVERIFY(bar->property("tlsBroken").toBool());
    evaluate(webView, QStringLiteral("security.allGood = true"));
    QVERIFY(!bar->property("tlsBroken").toBool());
    // A page gone back to from its bfcache is not loaded again, and is looked at anyway.
    QVERIFY(reader->property("readerable").toBool());
    QCOMPARE(tabs->activeUrl(), story);
    // Nothing is restyled that is not a reader view.
    m_core->settings()->setReaderColors(Settings::ReaderDark);
    QCOMPARE(runs(engine->styleScript(true)), 0);

    // A reader view come back to through the history is one too, whatever the tab was
    // on in between: forward from a page the article linked to, say.
    webView->setProperty("url", QUrl(QStringLiteral("https://example.com/linked")));
    QCOMPARE(tabs->activeUrl(), QStringLiteral("https://example.com/linked"));
    webView->setProperty("url", readerUrl);
    QVERIFY(reader->property("active").toBool());
    QCOMPARE(tabs->activeUrl(), story);
    // With nothing before it, closing loads the article's page.
    webView->setProperty("canGoBack", false);
    evaluate(reader, QStringLiteral("toggle()"));
    QCOMPARE(webView->property("url").toUrl(), QUrl(story));
    QVERIFY(!reader->property("active").toBool());

    // A page Readability finds no article in stops being offered, and nothing loads.
    answer(QStringLiteral("true"), QStringLiteral("''"));
    load();
    QVERIFY(button->property("enabled").toBool());
    tapBar(QStringLiteral("menu"));
    click(button);
    QCOMPARE(calls(QStringLiteral("loadHtml")), 1);
    QVERIFY(!reader->property("readerable").toBool());
    QVERIFY(!button->property("enabled").toBool());
    // Nor one where the script fails.
    answer(QStringLiteral("true"), articleLiteral);
    load();
    QVERIFY(reader->property("readerable").toBool());
    webView->setProperty("scriptFails", true);
    evaluate(reader, QStringLiteral("open()"));
    QVERIFY(!reader->property("busy").toBool());
    QVERIFY(!reader->property("readerable").toBool());
    QCOMPARE(calls(QStringLiteral("loadHtml")), 1);
}

void tst_qmlload::downloadsPage()
{
    Salama::DownloadModel *downloads = m_core->downloads();
    // Something of BrowserPage.qml's own, whose scope has the engine.
    QObject *scope = find(QStringLiteral("viewArea"));
    QVERIFY(evaluate(scope, QStringLiteral("WebEngine.observers"))
                .toStringList()
                .contains(downloads->topic()));
    evaluate(scope, QStringLiteral("WebEngine.recvObserve('embed:download', {msg: 'dl-start',"
                                   " id: 1, displayName: 'report.pdf',"
                                   " sourceUrl: 'https://files.example/report.pdf',"
                                   " targetPath: '/tmp/report.pdf', mimeType: 'application/pdf',"
                                   " size: 2048})"));
    evaluate(scope, QStringLiteral("WebEngine.recvObserve('embed:download',"
                                   " {msg: 'dl-progress', id: 1, percent: 40})"));
    QCOMPARE(downloads->count(), 1);

    QObject *page = openMenuItem(QStringLiteral("downloadsMenuButton"));
    QCOMPARE(page->objectName(), QStringLiteral("downloadsPage"));
    QList<QObject *> rows = findAll(QStringLiteral("downloadDelegate"));
    QCOMPARE(rows.count(), 1);
    const auto text = [](QObject *row, const char *name) {
        return findObjects(row, QLatin1String(name)).first()->property("text").toString();
    };
    const auto shows = [](QObject *row, const char *name) {
        return findObjects(row, QLatin1String(name)).first()->property("visible").toBool();
    };
    QCOMPARE(text(rows.first(), "downloadName"), QStringLiteral("report.pdf"));
    QCOMPARE(text(rows.first(), "downloadStatus"), QStringLiteral("Downloading, 40%"));
    QVERIFY(shows(rows.first(), "downloadProgress"));

    // Not yet there, a tap opens nothing.
    const UrlCatcher files(QStringLiteral("file"));
    click(rows.first());
    QVERIFY(files.opened.isEmpty());

    // Arrived, it says where it came from, and a tap opens the file.
    evaluate(scope, QStringLiteral("WebEngine.recvObserve('embed:download',"
                                   " {msg: 'dl-done', id: 1, targetPath: '/tmp/report.pdf'})"));
    QCOMPARE(text(rows.first(), "downloadStatus"), QStringLiteral("files.example"));
    QVERIFY(!shows(rows.first(), "downloadProgress"));
    click(rows.first());
    QCOMPARE(files.opened, QList<QUrl>{QUrl::fromLocalFile(QStringLiteral("/tmp/report.pdf"))});
    QCOMPARE(currentPage(), page);

    // One that failed says so, newest first; a tap on it opens nothing.
    evaluate(scope,
             QStringLiteral("WebEngine.recvObserve('embed:download', {msg: 'dl-start',"
                            " id: 2, displayName: 'big.iso', sourceUrl: 'https://x.example/',"
                            " targetPath: '/tmp/big.iso', mimeType: '', size: 0})"));
    evaluate(scope, QStringLiteral("WebEngine.recvObserve('embed:download',"
                                   " {msg: 'dl-fail', id: 2})"));
    rows = byRow(findAll(QStringLiteral("downloadDelegate")));
    QCOMPARE(rows.count(), 2);
    QCOMPARE(text(rows.first(), "downloadName"), QStringLiteral("big.iso"));
    QCOMPARE(text(rows.first(), "downloadStatus"), QStringLiteral("Failed"));
    click(rows.first());
    QCOMPARE(files.opened.count(), 1);
    QCOMPARE(currentPage(), page);

    // Forgotten one at a time from its menu, or all at once from the pulley; the files
    // are not the list's to delete.
    click(findObjects(rows.first(), QStringLiteral("removeDownloadMenu")).first());
    QCOMPARE(downloads->count(), 1);
    QObject *clear = find(QStringLiteral("clearDownloadsMenu"));
    QVERIFY(clear->property("enabled").toBool());
    click(clear);
    QCOMPARE(downloads->count(), 0);
    QCOMPARE(findAll(QStringLiteral("downloadDelegate")).count(), 0);
    QVERIFY(!clear->property("enabled").toBool());
}

void tst_qmlload::historyPage()
{
    m_core->history()->visit(QStringLiteral("https://one.example/"), QStringLiteral("One"));
    m_core->history()->visit(QStringLiteral("https://two.example/"), QStringLiteral("Two"));

    openMenuItem(QStringLiteral("historyMenuButton"));
    QCOMPARE(findAll(QStringLiteral("historyDelegate")).count(), 3);
    QObject *search = find(QStringLiteral("historySearch"));
    search->setProperty("text", QStringLiteral("two"));
    QCOMPARE(m_core->history()->count(), 1);
    QCOMPARE(findAll(QStringLiteral("historyDelegate")).count(), 1);
    search->setProperty("text", QString());
    QCOMPARE(findAll(QStringLiteral("historyDelegate")).count(), 3);

    QList<QObject *> delegates = findAll(QStringLiteral("historyDelegate"));
    QCOMPARE(delegates.at(1)
                 ->findChild<QObject *>(QStringLiteral("historyTitle"))
                 ->property("text")
                 .toString(),
             QStringLiteral("One"));
    click(delegates.at(1));
    QCOMPARE(currentPage()->objectName(), QStringLiteral("browserPage"));
    QCOMPARE(currentWebView()->property("url").toUrl().toString(),
             QStringLiteral("https://one.example/"));
    QCOMPARE(m_core->tabs()->count(), 1);

    openMenuItem(QStringLiteral("historyMenuButton"));
    delegates = findAll(QStringLiteral("historyDelegate"));
    click(findObjects(delegates.at(0), QStringLiteral("openInNewTabMenu")).first());
    QCOMPARE(m_core->tabs()->count(), 2);
    QCOMPARE(currentPage()->objectName(), QStringLiteral("browserPage"));

    openMenuItem(QStringLiteral("historyMenuButton"));
    const int before = m_core->history()->count();
    delegates = findAll(QStringLiteral("historyDelegate"));
    click(findObjects(delegates.at(0), QStringLiteral("removeHistoryMenu")).first());
    QCOMPARE(m_core->history()->count(), before - 1);

    click(find(QStringLiteral("clearHistoryMenu")));
    QCOMPARE(m_core->history()->count(), 0);
    QCOMPARE(findAll(QStringLiteral("historyDelegate")).count(), 0);
}

void tst_qmlload::bookmarksPage()
{
    m_core->bookmarks()->add(QStringLiteral("https://b1.example/"), QStringLiteral("B1"));
    m_core->bookmarks()->add(QStringLiteral("https://b2.example/"), QStringLiteral("B2"));

    openMenuItem(QStringLiteral("bookmarksMenuButton"));
    QList<QObject *> delegates = findAll(QStringLiteral("bookmarkDelegate"));
    QCOMPARE(delegates.count(), 2);
    click(delegates.at(0));
    QCOMPARE(currentPage()->objectName(), QStringLiteral("browserPage"));
    QCOMPARE(currentWebView()->property("url").toUrl().toString(),
             QStringLiteral("https://b1.example/"));

    openMenuItem(QStringLiteral("bookmarksMenuButton"));
    delegates = findAll(QStringLiteral("bookmarkDelegate"));
    click(findObjects(delegates.at(1), QStringLiteral("editBookmarkMenu")).first());
    QObject *dialog = currentPage();
    QCOMPARE(dialog->objectName(), QStringLiteral("bookmarkEditDialog"));
    QCOMPARE(dialog->property("bookmarkIndex").toInt(), 1);
    QCOMPARE(dialog->property("url").toString(), QStringLiteral("https://b2.example/"));
    find(QStringLiteral("bookmarkUrlField"))
        ->setProperty("text", QStringLiteral("https://b2.example/x"));
    find(QStringLiteral("bookmarkTitleField"))->setProperty("text", QStringLiteral("B2x"));
    QMetaObject::invokeMethod(dialog, "accept");
    QCOMPARE(m_core->bookmarks()
                 ->data(m_core->bookmarks()->index(1, 0), BookmarkModel::UrlRole)
                 .toString(),
             QStringLiteral("https://b2.example/x"));
    QCOMPARE(m_core->bookmarks()
                 ->data(m_core->bookmarks()->index(1, 0), BookmarkModel::TitleRole)
                 .toString(),
             QStringLiteral("B2x"));
    popPage();

    delegates = findAll(QStringLiteral("bookmarkDelegate"));
    click(findObjects(delegates.at(0), QStringLiteral("removeBookmarkMenu")).first());
    QCOMPARE(m_core->bookmarks()->count(), 1);
    QCOMPARE(delegates.at(0)->property("remorseCount").toInt(), 1);

    QObject *addMenu = find(QStringLiteral("addBookmarkMenu"));
    QVERIFY(addMenu->property("enabled").toBool());
    click(addMenu);
    QCOMPARE(m_core->bookmarks()->count(), 2);
    QVERIFY(m_core->bookmarks()->contains(QStringLiteral("https://b1.example/")));
    QVERIFY(!addMenu->property("enabled").toBool());

    delegates = findAll(QStringLiteral("bookmarkDelegate"));
    click(findObjects(delegates.at(0), QStringLiteral("openInNewTabMenu")).first());
    QCOMPARE(m_core->tabs()->count(), 2);
    QCOMPARE(currentPage()->objectName(), QStringLiteral("browserPage"));
}

namespace {

// What the column an item of a settings page sits in holds, in order: each item's
// objectName, and each section header as "#" and its text. Declaration order rather
// than laid-out y: a Column places its items as it is polished, which a window that
// draws nothing never is.
QStringList columnOf(QObject *item)
{
    QStringList held;
    for (QQuickItem *child : qobject_cast<QQuickItem *>(item)->parentItem()->childItems()) {
        if (QString::fromLatin1(child->metaObject()->className())
                .startsWith(QLatin1String("SectionHeader"))) {
            held.append(QLatin1Char('#') + child->property("text").toString());
        } else if (!child->objectName().isEmpty()) {
            held.append(child->objectName());
        }
    }
    return held;
}

} // namespace

// Settings is a main page of what is set in a line, and a way each to a page of its own
// for search, the reader view, the cover and privacy, under the headings General,
// Appearance and Privacy (docs/DECISIONS/0028-settings-pages.md). Each way in is a
// theme icon, a name and a line saying how its subject is set, and pushes its page
// over the main one.
void tst_qmlload::settingsPage()
{
    QObject *page = openMenuItem(QStringLiteral("settingsMenuButton"));
    QCOMPARE(page->objectName(), QStringLiteral("settingsPage"));

    // In the order asked for: Search first, ahead of what stayed on the main page.
    const QStringList expected{
        QStringLiteral("#General"),
        QStringLiteral("searchSettingsEntry"),
        QStringLiteral("homePageField"),
        QStringLiteral("desktopModeSwitch"),
        QStringLiteral("cutoutGuardSwitch"),
        QStringLiteral("liveTabLimitCombo"),
        QStringLiteral("#Appearance"),
        QStringLiteral("readerSettingsEntry"),
        QStringLiteral("coverSettingsEntry"),
        QStringLiteral("#Privacy"),
        QStringLiteral("privacySettingsEntry"),
    };
    QCOMPARE(columnOf(find(QStringLiteral("searchSettingsEntry"))), expected);

    // Each way in, with the theme icon a Jolla application gives the same subject --
    // the cover's is the tab count's, not the display's, which sailfish-browser has
    // for what is the screen cutout here.
    struct Entry
    {
        QString name;
        QString icon;
        QString page;
    };
    const QList<Entry> entries{
        {QStringLiteral("searchSettingsEntry"), QStringLiteral("icon-m-search"),
         QStringLiteral("searchSettingsPage")},
        {QStringLiteral("readerSettingsEntry"), QStringLiteral("icon-m-file-formatted"),
         QStringLiteral("readerSettingsPage")},
        {QStringLiteral("coverSettingsEntry"), QStringLiteral("icon-m-tabs"),
         QStringLiteral("coverSettingsPage")},
        {QStringLiteral("privacySettingsEntry"), QStringLiteral("icon-m-device-lock"),
         QStringLiteral("privacySettingsPage")},
    };
    const qreal itemSize = evaluate(page, QStringLiteral("Theme.itemSizeMedium")).toReal();
    const qreal iconSize = evaluate(page, QStringLiteral("Theme.iconSizeMedium")).toReal();
    for (const Entry &entry : entries) {
        QObject *item = find(entry.name);
        QVERIFY2(item != nullptr, qPrintable(entry.name));
        QCOMPARE(item->property("iconSource").toString(),
                 QStringLiteral("image://theme/") + entry.icon);
        QVERIFY2(!item->property("text").toString().isEmpty(), qPrintable(entry.name));
        QVERIFY2(!item->property("summary").toString().isEmpty(), qPrintable(entry.name));
        QCOMPARE(item->property("height").toReal(), itemSize);
        QObject *icon = findObjects(item, QStringLiteral("settingsEntryIcon")).first();
        QCOMPARE(icon->property("width").toReal(), iconSize);
        QCOMPARE(findObjects(item, QStringLiteral("settingsEntryName")).first()->property("text"),
                 item->property("text"));
        QObject *summary = findObjects(item, QStringLiteral("settingsEntrySummary")).first();
        QCOMPARE(summary->property("text"), item->property("summary"));
        QVERIFY(summary->property("visible").toBool());
        click(item);
        QCOMPARE(currentPage()->objectName(), entry.page);
        QCOMPARE(pageStack()->property("depth").toInt(), 3);
        popPage();
        QCOMPARE(currentPage(), page);
    }

    // Lit while it is pressed, as Silica's rows are.
    QObject *search = find(QStringLiteral("searchSettingsEntry"));
    QObject *searchName = findObjects(search, QStringLiteral("settingsEntryName")).first();
    QCOMPARE(searchName->property("color"), evaluate(page, QStringLiteral("Theme.primaryColor")));
    search->setProperty("down", true);
    QVERIFY(findObjects(search, QStringLiteral("settingsEntryIcon"))
                .first()
                ->property("highlighted")
                .toBool());
    QCOMPARE(searchName->property("color"), evaluate(page, QStringLiteral("Theme.highlightColor")));
    search->setProperty("down", false);

    // What stayed on the main page is set here as it was.
    QObject *home = find(QStringLiteral("homePageField"));
    QCOMPARE(home->property("text").toString(), Settings::defaultHomePage());
    home->setProperty("text", QStringLiteral("sailfishos.org"));
    enterKey(home);
    QCOMPARE(m_core->settings()->homePage(), QStringLiteral("https://sailfishos.org"));

    find(QStringLiteral("desktopModeSwitch"))->setProperty("checked", true);
    QVERIFY(m_core->settings()->desktopMode());
    QVERIFY(currentWebView()->property("desktopMode").toBool());

    // The cutout guard is on until it is turned off here, and the page answers.
    QObject *cutoutSwitch = find(QStringLiteral("cutoutGuardSwitch"));
    QVERIFY(cutoutSwitch->property("checked").toBool());
    cutoutSwitch->setProperty("checked", false);
    QVERIFY(!m_core->settings()->cutoutGuard());
    QCOMPARE(find(QStringLiteral("browserPage"))->property("cutoutInset").toReal(), qreal(0));
    QCOMPARE(find(QStringLiteral("tabsView"))->property("cutoutHeight").toReal(), qreal(0));
    cutoutSwitch->setProperty("checked", true);
    QVERIFY(find(QStringLiteral("browserPage"))->property("cutoutInset").toReal() > 0);
}

// Search: the engine the address bar searches with, and the sources its suggestions
// are drawn from, each a switch that is on until it is turned off. The way in names
// the engine.
void tst_qmlload::searchSettingsPage()
{
    Settings *settings = m_core->settings();
    openMenuItem(QStringLiteral("settingsMenuButton"));
    QObject *entry = find(QStringLiteral("searchSettingsEntry"));
    const QStringList engines = settings->searchEngineNames();
    QCOMPARE(entry->property("summary").toString(), engines.at(settings->searchEngineIndex()));
    click(entry);
    QCOMPARE(currentPage()->objectName(), QStringLiteral("searchSettingsPage"));

    QObject *combo = find(QStringLiteral("searchEngineCombo"));
    QCOMPARE(combo->property("currentIndex").toInt(), settings->searchEngineIndex());
    const int other = settings->searchEngineIndex() == 1 ? 0 : 1;
    combo->setProperty("currentIndex", other);
    QCOMPARE(settings->searchEngineIndex(), other);
    QCOMPARE(entry->property("summary").toString(), engines.at(other));

    // A switch for each source, in the order the address bar lists them, each writing
    // its own setting and no other.
    using Flag = bool (Settings::*)() const;
    const QList<QPair<QString, Flag>> sources{
        {QStringLiteral("omnibarTabsSwitch"), &Settings::omnibarTabs},
        {QStringLiteral("omnibarBookmarksSwitch"), &Settings::omnibarBookmarks},
        {QStringLiteral("omnibarHistorySwitch"), &Settings::omnibarHistory},
        {QStringLiteral("omnibarDownloadsSwitch"), &Settings::omnibarDownloads},
    };
    const QStringList layout{
        QStringLiteral("searchEngineCombo"),    QStringLiteral("#Address bar suggestions"),
        QStringLiteral("omnibarTabsSwitch"),    QStringLiteral("omnibarBookmarksSwitch"),
        QStringLiteral("omnibarHistorySwitch"), QStringLiteral("omnibarDownloadsSwitch"),
    };
    QCOMPARE(columnOf(combo), layout);
    for (const auto &source : sources) {
        QObject *toggle = find(source.first);
        QVERIFY2(!toggle->property("text").toString().isEmpty(), qPrintable(source.first));
        QVERIFY2(toggle->property("checked").toBool(), qPrintable(source.first));
        QVERIFY((settings->*source.second)());
        toggle->setProperty("checked", false);
        QVERIFY2(!(settings->*source.second)(), qPrintable(source.first));
        for (const auto &rest : sources) {
            if (rest.first != source.first) {
                QVERIFY2((settings->*rest.second)(), qPrintable(rest.first));
            }
        }
        toggle->setProperty("checked", true);
        QVERIFY((settings->*source.second)());
    }
}

// The reader view's look: each choice the stored value, Firefox's middle text size
// written as the whole of itself, and the way in says all three in a line.
void tst_qmlload::readerSettingsPage()
{
    openMenuItem(QStringLiteral("settingsMenuButton"));
    QObject *entry = find(QStringLiteral("readerSettingsEntry"));
    QCOMPARE(entry->property("summary").toString(),
             QStringLiteral("Ambience · Sans serif · 100 %"));
    click(entry);
    QCOMPARE(currentPage()->objectName(), QStringLiteral("readerSettingsPage"));

    QObject *readerColors = find(QStringLiteral("readerColorsCombo"));
    QCOMPARE(readerColors->property("currentIndex").toInt(), int(Settings::ReaderAmbience));
    readerColors->setProperty("currentIndex", int(Settings::ReaderSepia));
    QCOMPARE(m_core->settings()->readerColors(), int(Settings::ReaderSepia));
    QCOMPARE(entry->property("summary").toString(), QStringLiteral("Sepia · Sans serif · 100 %"));
    QObject *readerTypeface = find(QStringLiteral("readerTypefaceCombo"));
    QCOMPARE(readerTypeface->property("currentIndex").toInt(), int(Settings::ReaderSansSerif));
    readerTypeface->setProperty("currentIndex", int(Settings::ReaderSerif));
    QCOMPARE(m_core->settings()->readerTypeface(), int(Settings::ReaderSerif));
    QCOMPARE(entry->property("summary").toString(), QStringLiteral("Sepia · Serif · 100 %"));
    QObject *readerSize = find(QStringLiteral("readerTextSizeSlider"));
    QCOMPARE(readerSize->property("minimumValue").toInt(), int(Settings::ReaderTextSizeMin));
    QCOMPARE(readerSize->property("maximumValue").toInt(), int(Settings::ReaderTextSizeMax));
    QCOMPARE(readerSize->property("value").toInt(), int(Settings::ReaderTextSizeDefault));
    QCOMPARE(readerSize->property("valueText").toString(), QStringLiteral("100 %"));
    readerSize->setProperty("value", 9);
    QCOMPARE(m_core->settings()->readerTextSize(), 9);
    QCOMPARE(readerSize->property("valueText").toString(), QStringLiteral("140 %"));
    QCOMPARE(entry->property("summary").toString(), QStringLiteral("Sepia · Serif · 140 %"));
    readerSize->setProperty("value", 1);
    QCOMPARE(readerSize->property("valueText").toString(), QStringLiteral("60 %"));
    QCOMPARE(entry->property("summary").toString(), QStringLiteral("Sepia · Serif · 60 %"));

    // The line follows the setting wherever it is written from.
    m_core->settings()->setReaderColors(Settings::ReaderDark);
    QCOMPARE(entry->property("summary").toString(), QStringLiteral("Dark · Serif · 60 %"));
}

// Privacy: tracking protection, which reaches the engine at once, and the way to clear
// browsing data, which asks first. The way in names the level.
void tst_qmlload::privacySettingsPage()
{
    openMenuItem(QStringLiteral("settingsMenuButton"));
    QObject *entry = find(QStringLiteral("privacySettingsEntry"));
    QCOMPARE(entry->property("summary").toString(),
             QStringLiteral("Tracking protection: Standard"));
    click(entry);
    QObject *page = currentPage();
    QCOMPARE(page->objectName(), QStringLiteral("privacySettingsPage"));

    // Tracking protection is Standard until it is changed here, and a change reaches
    // the engine at once, every preference of the new level after the old ones. The
    // browsing page writes them, so they are read through something of BrowserPage.qml's
    // own.
    QObject *pageScope = find(QStringLiteral("viewArea"));
    QObject *trackingCombo = find(QStringLiteral("trackingProtectionCombo"));
    QCOMPARE(trackingCombo->property("currentIndex").toInt(),
             int(Settings::TrackingProtectionStandard));
    const QString standardDescription = trackingCombo->property("description").toString();
    const int given =
        evaluate(pageScope, QStringLiteral("WebEngineSettings.preferences.length")).toInt();
    trackingCombo->setProperty("currentIndex", int(Settings::TrackingProtectionStrict));
    QCOMPARE(m_core->settings()->trackingProtection(), int(Settings::TrackingProtectionStrict));
    const QVariantList strict =
        EngineMessages::trackingProtectionPreferences(Settings::TrackingProtectionStrict);
    const QVariantList preferences =
        evaluate(pageScope, QStringLiteral("WebEngineSettings.preferences")).toList();
    QCOMPARE(preferences.count(), given + strict.count());
    for (int i = 0; i < strict.count(); ++i) {
        QCOMPARE(preferences.at(given + i).toMap().value(QStringLiteral("key")),
                 strict.at(i).toMap().value(QStringLiteral("name")));
        QCOMPARE(preferences.at(given + i).toMap().value(QStringLiteral("value")),
                 strict.at(i).toMap().value(QStringLiteral("value")));
    }
    const QString strictDescription = trackingCombo->property("description").toString();
    QVERIFY(!strictDescription.isEmpty());
    QVERIFY(strictDescription != standardDescription);
    QCOMPARE(entry->property("summary").toString(), QStringLiteral("Tracking protection: Strict"));
    trackingCombo->setProperty("currentIndex", int(Settings::TrackingProtectionOff));
    QCOMPARE(m_core->settings()->trackingProtection(), int(Settings::TrackingProtectionOff));
    QCOMPARE(evaluate(pageScope, QStringLiteral("WebEngineSettings.preferences.length")).toInt(),
             given + 2 * strict.count());
    const QString offDescription = trackingCombo->property("description").toString();
    QVERIFY(!offDescription.isEmpty());
    QVERIFY(offDescription != standardDescription && offDescription != strictDescription);
    QCOMPARE(entry->property("summary").toString(), QStringLiteral("Tracking protection: Off"));

    // Clearing browsing data is a way in of its own, under the level, to a dialog that
    // asks which kinds (clearDataDialog()); backed out of, it clears nothing.
    const int visits = m_core->history()->count();
    const int remorses = evaluate(page, QStringLiteral("Remorse.popupCount")).toInt();
    QObject *clear = find(QStringLiteral("clearDataEntry"));
    QCOMPARE(clear->property("iconSource").toString(),
             QStringLiteral("image://theme/icon-m-delete"));
    QCOMPARE(clear->property("text").toString(), QStringLiteral("Clear browsing data"));
    QCOMPARE(columnOf(clear), QStringList({QStringLiteral("trackingProtectionCombo"),
                                           QStringLiteral("clearDataEntry")}));
    click(clear);
    QCOMPARE(currentPage()->objectName(), QStringLiteral("clearDataDialog"));
    popPage();
    QCOMPARE(currentPage(), page);
    QCOMPARE(evaluate(page, QStringLiteral("Remorse.popupCount")).toInt(), remorses);
    QCOMPARE(evaluate(page, QStringLiteral("WebEngine.notifications.length")).toInt(), 0);
    QCOMPARE(m_core->history()->count(), visits);
    QCOMPARE(m_core->tabs()->count(), 1);
}

// Clear browsing data asks which kinds in a dialog -- the open tabs off to begin with,
// the rest on, and Clear dimmed while none is -- and what it is accepted with is
// cleared under one remorse on the privacy page, each kind as its own button used to
// clear it.
void tst_qmlload::clearDataDialog()
{
    TabModel *tabs = m_core->tabs();
    tabs->newTab(QStringLiteral("https://two.example/"));
    QVERIFY(m_core->history()->count() > 0);
    openMenuItem(QStringLiteral("settingsMenuButton"));
    click(find(QStringLiteral("privacySettingsEntry")));
    QObject *privacy = currentPage();
    QCOMPARE(privacy->objectName(), QStringLiteral("privacySettingsPage"));
    const auto remorses = [this, privacy]() {
        return evaluate(privacy, QStringLiteral("Remorse.popupCount")).toInt();
    };
    const auto sent = [this, privacy]() {
        return evaluate(privacy, QStringLiteral("WebEngine.notifications.length")).toInt();
    };
    const auto notification = [this, privacy](int index, const QString &field) {
        return evaluate(privacy,
                        QStringLiteral("WebEngine.notifications[%1].%2").arg(index).arg(field))
            .toString();
    };
    const QStringList switches{
        QStringLiteral("clearTabsSwitch"),
        QStringLiteral("clearHistorySwitch"),
        QStringLiteral("clearSiteDataSwitch"),
        QStringLiteral("clearCacheSwitch"),
    };
    // Asks again, with the switches set in the order above, and accepts. The stub page
    // stack leaves popping an accepted dialog to its caller, as the other tests do.
    const auto clear = [this, &switches](const QList<bool> &on) {
        click(find(QStringLiteral("clearDataEntry")));
        QObject *dialog = currentPage();
        QCOMPARE(dialog->objectName(), QStringLiteral("clearDataDialog"));
        for (int i = 0; i < switches.count(); ++i) {
            find(switches.at(i))->setProperty("checked", on.at(i));
        }
        QMetaObject::invokeMethod(dialog, "accept");
        popPage();
    };

    // The dialog as it opens, in Firefox's order: all but the open tabs on.
    click(find(QStringLiteral("clearDataEntry")));
    QObject *dialog = currentPage();
    QCOMPARE(dialog->objectName(), QStringLiteral("clearDataDialog"));
    const QList<bool> initially{false, true, true, true};
    QCOMPARE(columnOf(find(switches.first())), switches);
    for (int i = 0; i < switches.count(); ++i) {
        QObject *toggle = find(switches.at(i));
        QVERIFY2(!toggle->property("text").toString().isEmpty(), qPrintable(switches.at(i)));
        QCOMPARE(toggle->property("checked").toBool(), initially.at(i));
    }
    QVERIFY(dialog->property("canAccept").toBool());

    // With nothing on, Clear is dimmed and does nothing; any one on is enough.
    for (const QString &name : switches) {
        find(name)->setProperty("checked", false);
    }
    QVERIFY(!dialog->property("canAccept").toBool());
    QMetaObject::invokeMethod(dialog, "accept");
    QCOMPARE(currentPage(), dialog);
    for (const QString &name : switches) {
        find(name)->setProperty("checked", true);
        QVERIFY2(dialog->property("canAccept").toBool(), qPrintable(name));
        find(name)->setProperty("checked", false);
    }
    // Backed out of, it clears nothing.
    popPage();
    QCOMPARE(currentPage(), privacy);
    QCOMPARE(remorses(), 0);
    QCOMPARE(tabs->count(), 2);
    QVERIFY(m_core->history()->count() > 0);
    QCOMPARE(sent(), 0);

    // The history alone, as its button cleared it, under one remorse on the privacy
    // page that says what it is doing.
    clear({false, true, false, false});
    QCOMPARE(remorses(), 1);
    QCOMPARE(evaluate(privacy, QStringLiteral("Remorse.popupItem")).value<QObject *>(), privacy);
    QCOMPARE(evaluate(privacy, QStringLiteral("Remorse.popupText")).toString(),
             QStringLiteral("Clearing browsing data"));
    QCOMPARE(currentPage(), privacy);
    QCOMPARE(m_core->history()->count(), 0);
    QCOMPARE(tabs->count(), 2);
    QCOMPARE(sent(), 0);

    // Cookies and site data alone: the engine's own notification, and nothing else.
    tabs->newTab(QStringLiteral("https://three.example/"));
    const int visits = m_core->history()->count();
    QVERIFY(visits > 0);
    clear({false, false, true, false});
    QCOMPARE(remorses(), 2);
    QCOMPARE(sent(), 1);
    QCOMPARE(notification(0, QStringLiteral("topic")), QStringLiteral("clear-private-data"));
    QCOMPARE(notification(0, QStringLiteral("value")), QStringLiteral("cookies-and-site-data"));
    QCOMPARE(m_core->history()->count(), visits);
    QCOMPARE(tabs->count(), 3);

    // The cache alone.
    clear({false, false, false, true});
    QCOMPARE(remorses(), 3);
    QCOMPARE(sent(), 2);
    QCOMPARE(notification(1, QStringLiteral("topic")), QStringLiteral("clear-private-data"));
    QCOMPARE(notification(1, QStringLiteral("value")), QStringLiteral("cache"));
    QCOMPARE(m_core->history()->count(), visits);
    QCOMPARE(tabs->count(), 3);

    // The open tabs alone: every one closed, and the browsing page opens its home page
    // in their place.
    clear({true, false, false, false});
    QCOMPARE(remorses(), 4);
    QCOMPARE(tabs->count(), 1);
    QCOMPARE(tabs->activeUrl(), Settings::defaultHomePage());
    QVERIFY(m_core->history()->count() >= visits);
    QCOMPARE(sent(), 2);
    QCOMPARE(currentPage(), privacy);

    // All four together are still one remorse. The tabs go first, so the history
    // cleared after them does not keep the page that took their place.
    tabs->newTab(QStringLiteral("https://four.example/"));
    clear({true, true, true, true});
    QCOMPARE(remorses(), 5);
    QCOMPARE(tabs->count(), 1);
    QCOMPARE(tabs->activeUrl(), Settings::defaultHomePage());
    QCOMPARE(m_core->history()->count(), 0);
    QCOMPARE(sent(), 4);
    QCOMPARE(notification(2, QStringLiteral("value")), QStringLiteral("cookies-and-site-data"));
    QCOMPARE(notification(3, QStringLiteral("value")), QStringLiteral("cache"));
}

// The cover's style, the one choice among the settings that another page has to
// answer, and under it the cover's quick action. The way in says what the cover shows
// and what its action is, in the words the choices are offered in.
void tst_qmlload::coverSettingsPage()
{
    Settings *settings = m_core->settings();
    openMenuItem(QStringLiteral("settingsMenuButton"));
    QObject *entry = find(QStringLiteral("coverSettingsEntry"));
    QCOMPARE(entry->property("summary").toString(),
             QStringLiteral("The tab count and the most recent tabs \u00b7 Search"));
    click(entry);
    QObject *page = currentPage();
    QCOMPARE(page->objectName(), QStringLiteral("coverSettingsPage"));

    QObject *coverCombo = find(QStringLiteral("coverStyleCombo"));
    const QStringList layout{
        QStringLiteral("coverStyleCombo"),      QStringLiteral("#Quick action"),
        QStringLiteral("quickActionExplained"), QStringLiteral("quickActionPreviews"),
        QStringLiteral("quickActionChoice"),    QStringLiteral("quickActionIcons"),
    };
    QCOMPARE(columnOf(coverCombo), layout);
    QCOMPARE(coverCombo->property("currentIndex").toInt(), int(Settings::CoverEveryTab));
    coverCombo->setProperty("currentIndex", int(Settings::CoverIconOnly));
    QCOMPARE(settings->coverStyle(), int(Settings::CoverIconOnly));
    auto *coverItem = m_window->property("coverItem").value<QObject *>();
    QVERIFY(!coverItem->findChild<QObject *>(QStringLiteral("coverHeading"))
                 ->property("visible")
                 .toBool());

    const QList<QPair<Settings::CoverStyle, QString>> styles{
        {Settings::CoverIconOnly, QStringLiteral("coverIconOnlyItem")},
        {Settings::CoverLatestTab, QStringLiteral("coverLatestTabItem")},
        {Settings::CoverEveryTab, QStringLiteral("coverEveryTabItem")},
    };
    for (const auto &style : styles) {
        coverCombo->setProperty("currentIndex", int(style.first));
        QCOMPARE(settings->coverStyle(), int(style.first));
        QCOMPARE(entry->property("summary").toString(),
                 find(style.second)->property("text").toString() +
                     QStringLiteral(" \u00b7 Search"));
    }
    settings->setCoverStyle(Settings::CoverLatestTab);
    QCOMPARE(entry->property("summary").toString(),
             QStringLiteral("The tab count and the last tab \u00b7 Search"));
    settings->setQuickAction(Settings::QuickActionNone);
    QCOMPARE(entry->property("summary").toString(),
             QStringLiteral("The tab count and the last tab \u00b7 No quick action"));
    settings->setQuickAction(Settings::QuickActionBookmark);
    QCOMPARE(entry->property("summary").toString(),
             QStringLiteral("The tab count and the last tab \u00b7 Open a bookmark"));

    // Why there is one action, in the voice of a hint rather than a control: small, in
    // the secondary highlight, and the words of a sentence rather than rich text.
    QObject *explained = find(QStringLiteral("quickActionExplained"));
    QCOMPARE(explained->property("text").toString(),
             QStringLiteral("The cover on the home screen shows one quick action. The place "
                            "beside it is kept for the media control, which appears there "
                            "while the tab in front plays something."));
    QCOMPARE(explained->property("textFormat").toInt(), int(Qt::PlainText));
    QCOMPARE(explained->property("wrapMode").toInt(),
             evaluate(explained, QStringLiteral("Text.Wrap")).toInt());
    QCOMPARE(explained->property("font").value<QFont>().pixelSize(),
             evaluate(page, QStringLiteral("Theme.fontSizeSmall")).toInt());
    QCOMPARE(explained->property("color"),
             evaluate(page, QStringLiteral("Theme.secondaryHighlightColor")));

    // The pictures are a real cover's shape, two thirds its size, side by side, and the
    // row is drawn as Silica draws a ComboBox: the name, and what it is set to in the
    // highlight colour.
    const qreal coverWidth = evaluate(page, QStringLiteral("Theme.coverSizeLarge.width")).toReal();
    const qreal coverHeight =
        evaluate(page, QStringLiteral("Theme.coverSizeLarge.height")).toReal();
    auto *quiet = qobject_cast<QQuickItem *>(find(QStringLiteral("quietCoverPreview")));
    auto *playing = qobject_cast<QQuickItem *>(find(QStringLiteral("playingCoverPreview")));
    QCOMPARE(quiet->width(), coverWidth * 2 / 3);
    QCOMPARE(playing->width(), quiet->width());
    QVERIFY(quiet->x() + quiet->width() < playing->x());
    auto *picture =
        qobject_cast<QQuickItem *>(findObjects(quiet, QStringLiteral("previewPicture")).first());
    QCOMPARE(picture->height(), qreal(qRound(picture->width() * coverHeight / coverWidth)));
    QCOMPARE(quiet->property("text").toString(), QStringLiteral("Nothing playing"));
    QCOMPARE(playing->property("text").toString(), QStringLiteral("While a tab plays"));
    QObject *choice = find(QStringLiteral("quickActionChoice"));
    QCOMPARE(choice->property("contentHeight"),
             evaluate(page, QStringLiteral("Theme.itemSizeSmall")));
    QCOMPARE(textIn(choice, QStringLiteral("quickActionLabel")), QStringLiteral("Action"));
    QCOMPARE(findObjects(choice, QStringLiteral("quickActionValue")).first()->property("color"),
             evaluate(page, QStringLiteral("Theme.highlightColor")));
    click(choice);
    QVERIFY(choice->property("menuOpen").toBool());
}

void tst_qmlload::cover()
{
    auto *coverItem = m_window->property("coverItem").value<QObject *>();
    QVERIFY(coverItem != nullptr);
    QCOMPARE(
        coverItem->findChild<QObject *>(QStringLiteral("coverBrand"))->property("text").toString(),
        QStringLiteral("Salama"));
    QCOMPARE(coverItem->findChild<QObject *>(QStringLiteral("coverSubtitle"))
                 ->property("text")
                 .toString(),
             QStringLiteral("Tabs"));

    // The number is what the cover is for, and the field under it holds one cell
    // per tab -- no cell stands in for a tab that is not there, and none is left
    // out for a tab that has no picture yet.
    auto *count = coverItem->findChild<QObject *>(QStringLiteral("coverTabCount"));
    QCOMPARE(count->property("text").toString(), QStringLiteral("1"));
    QCOMPARE(findObjects(coverItem, QStringLiteral("coverTabCell")).count(), 1);

    // The quick action is a search until another is chosen: the window raised, and the
    // address bar opened for a new tab, which is made once something is chosen and
    // counted from then on.
    QMetaObject::invokeMethod(coverItem->findChild<QObject *>(QStringLiteral("quickCoverAction")),
                              "triggered");
    QCOMPARE(m_window->property("activateCount").toInt(), 1);
    QVERIFY(find(QStringLiteral("navigationBar"))->property("editing").toBool());
    QVERIFY(find(QStringLiteral("navigationBar"))->property("forNewTab").toBool());
    QCOMPARE(m_core->tabs()->count(), 1);
    QObject *field = find(QStringLiteral("addressField"));
    field->setProperty("text", QStringLiteral("example.org"));
    enterKey(field);
    QCOMPARE(m_core->tabs()->count(), 2);
    QCOMPARE(count->property("text").toString(), QStringLiteral("2"));
    QCOMPARE(findObjects(coverItem, QStringLiteral("coverTabCell")).count(), 2);
}

void tst_qmlload::coverFieldFollowsTheFront()
{
    auto *coverItem = m_window->property("coverItem").value<QObject *>();
    QVERIFY(coverItem != nullptr);
    TabModel *tabs = m_core->tabs();
    const int first = tabs->activeTabId();
    const int second = tabs->newTab(QStringLiteral("https://second.example/"));

    // A picture for each, so the order the cover draws them in can be read off the
    // cells' own sources.
    QObject *webView = currentWebView();
    webView->setProperty("loading", true);
    webView->setProperty("loading", false);
    const QString secondShot = webView->property("lastGrabPath").toString();
    QVERIFY(!secondShot.isEmpty());
    tabs->activateTabById(first);
    webView = currentWebView();
    webView->setProperty("loading", true);
    webView->setProperty("loading", false);
    const QString firstShot = webView->property("lastGrabPath").toString();
    QVERIFY(!firstShot.isEmpty());
    QVERIFY(firstShot != secondShot);

    // The tab in front leads the field, whatever the grid's own order is.
    QList<QObject *> cells = findObjects(coverItem, QStringLiteral("coverTabCell"));
    QCOMPARE(cells.count(), 2);
    QCOMPARE(evaluate(cells.first(), QStringLiteral("modelData")).toString(), firstShot);

    tabs->activateTabById(second);
    cells = findObjects(coverItem, QStringLiteral("coverTabCell"));
    QCOMPARE(evaluate(cells.first(), QStringLiteral("modelData")).toString(), secondShot);
}

void tst_qmlload::coverStyleIsConfigurable()
{
    auto *coverItem = m_window->property("coverItem").value<QObject *>();
    QVERIFY(coverItem != nullptr);
    m_core->tabs()->newTab(QStringLiteral("https://second.example/"));
    m_core->tabs()->newTab(QStringLiteral("https://third.example/"));

    auto *heading = coverItem->findChild<QObject *>(QStringLiteral("coverHeading"));
    auto *count = coverItem->findChild<QObject *>(QStringLiteral("coverTabCount"));
    auto *field = coverItem->findChild<QObject *>(QStringLiteral("coverTabField"));
    auto *icon = coverItem->findChild<QObject *>(QStringLiteral("coverIcon"));

    // Every tab, which is what a reader who has not been to Settings gets.
    QVERIFY(heading->property("visible").toBool());
    QVERIFY(count->property("visible").toBool());
    QVERIFY(field->property("visible").toBool());
    QVERIFY(!icon->property("visible").toBool());
    QCOMPARE(findObjects(coverItem, QStringLiteral("coverTabCell")).count(), 3);

    // The middle one: the heading stays, and the field is cut to the tab last read --
    // one cell, which the grid draws across the whole of the room it has.
    m_core->settings()->setCoverStyle(Settings::CoverLatestTab);
    QVERIFY(heading->property("visible").toBool());
    QVERIFY(count->property("visible").toBool());
    QVERIFY(field->property("visible").toBool());
    QCOMPARE(findObjects(coverItem, QStringLiteral("coverTabCell")).count(), 1);

    // The icon alone: no heading, no number, no pictures. The quick action stays
    // whatever the style is -- it is what the cover is there to offer.
    m_core->settings()->setCoverStyle(Settings::CoverIconOnly);
    QVERIFY(!heading->property("visible").toBool());
    QVERIFY(!count->property("visible").toBool());
    QVERIFY(!field->property("visible").toBool());
    QVERIFY(icon->property("visible").toBool());
    QVERIFY(icon->property("source").toUrl().toString().endsWith(
        QStringLiteral("art/harbour-salama.png")));
    QVERIFY(coverItem->findChild<QObject *>(QStringLiteral("quickCoverActions"))
                ->property("enabled")
                .toBool());

    m_core->settings()->setCoverStyle(Settings::CoverEveryTab);
    QCOMPARE(findObjects(coverItem, QStringLiteral("coverTabCell")).count(), 3);
}

namespace {

// Whether a cover action's picture is the file named, drawn for the stub's small icon
// size and dark ambience, and there to be read by the home screen.
bool drawnFrom(const QUrl &picture, const QString &file)
{
    return picture.toString().endsWith(QStringLiteral("art/cover/") + file) &&
           QFile::exists(picture.toLocalFile());
}

// The glyphs a picture of the cover draws its actions in, left to right; "" for the
// dot of an action not chosen.
QStringList previewGlyphs(QObject *preview)
{
    QList<QObject *> actions = findObjects(preview, QStringLiteral("previewAction"));
    std::stable_sort(actions.begin(), actions.end(), [](QObject *one, QObject *other) {
        return one->property("x").toReal() < other->property("x").toReal();
    });
    QStringList glyphs;
    for (QObject *action : actions) {
        glyphs.append(action->property("glyph").toString());
    }
    return glyphs;
}

} // namespace

// The cover's quick action, done by the window from wherever the application was left:
// the page on top popped, and what lay over the browsing page put away -- the sheet, the
// address being edited, the grid -- before the action opens what it names
// (docs/DECISIONS/0029-quick-action.md).
void tst_qmlload::quickActions()
{
    ScriptErrors errors;
    Settings *settings = m_core->settings();
    TabModel *tabs = m_core->tabs();
    const Forest forest = plantForest(m_core.data());
    auto *coverItem = m_window->property("coverItem").value<QObject *>();
    auto *action = coverItem->findChild<QObject *>(QStringLiteral("quickCoverAction"));
    QObject *page = find(QStringLiteral("browserPage"));
    QObject *menu = find(QStringLiteral("browserMenu"));
    QObject *bar = find(QStringLiteral("navigationBar"));
    const int open = tabs->count();
    int taps = 0;
    const auto tap = [action, &taps]() {
        QMetaObject::invokeMethod(action, "triggered");
        ++taps;
    };

    // Search: the address bar opened for a new tab over the page, from over the
    // settings; nothing is made until something is chosen.
    openMenuItem(QStringLiteral("settingsMenuButton"));
    tap();
    QCOMPARE(currentPage(), page);
    QVERIFY(bar->property("editing").toBool());
    QVERIFY(bar->property("forNewTab").toBool());
    QCOMPARE(tabs->count(), open);

    // The bookmarks, the downloads and the history: their pages, over the browsing page
    // rather than over what was left on it. The address that was being edited is not,
    // the sheet is put away, and the grid is closed.
    settings->setQuickAction(Settings::QuickActionBookmarks);
    tap();
    QCOMPARE(currentPage()->objectName(), QStringLiteral("bookmarksPage"));
    QCOMPARE(pageStack()->property("depth").toInt(), 2);
    QVERIFY(!bar->property("editing").toBool());
    popPage();
    settings->setQuickAction(Settings::QuickActionDownloads);
    tapBar(QStringLiteral("menu"));
    QVERIFY(menu->property("open").toBool());
    tap();
    QCOMPARE(currentPage()->objectName(), QStringLiteral("downloadsPage"));
    QVERIFY(!menu->property("open").toBool());
    popPage();
    settings->setQuickAction(Settings::QuickActionHistory);
    pullUpToTabs();
    QVERIFY(page->property("tabsOpen").toBool());
    openMenuItem(QStringLiteral("settingsMenuButton"));
    tap();
    QCOMPARE(currentPage()->objectName(), QStringLiteral("historyPage"));
    QCOMPARE(pageStack()->property("depth").toInt(), 2);
    QVERIFY(!page->property("tabsOpen").toBool());
    popPage();

    // One bookmark: the tab it is open in already, though that is in another group, is
    // brought to the front, the grid closed over it; nothing new is made.
    const QString work = QStringLiteral("https://forest.example/work");
    settings->setQuickActionBookmark(m_core->bookmarks()->add(work, QStringLiteral("Work")), work,
                                     QStringLiteral("Work"));
    settings->setQuickAction(Settings::QuickActionBookmark);
    pullUpToTabs();
    tap();
    QCOMPARE(tabs->activeTabId(), forest.away);
    QCOMPARE(tabs->currentGroupId(), forest.work);
    QCOMPARE(tabs->count(), open);
    QVERIFY(!page->property("tabsOpen").toBool());
    QCOMPARE(currentPage(), page);

    // Open in no tab, it opens in a new one -- which is the tab found the next time.
    const QString wiki = QStringLiteral("https://forest.example/wiki");
    const int wikiId = m_core->bookmarks()->idForUrl(wiki);
    settings->setQuickActionBookmark(wikiId, wiki, QStringLiteral("Forest wiki"));
    tap();
    QCOMPARE(tabs->count(), open + 1);
    QCOMPARE(tabs->activeUrl(), wiki);
    const int opened = tabs->activeTabId();
    tabs->activateTabById(forest.front);
    tap();
    QCOMPARE(tabs->count(), open + 1);
    QCOMPARE(tabs->activeTabId(), opened);

    // A bookmark no longer found anywhere opens the bookmarks, where another is a tap
    // away; the action is still the one chosen.
    m_core->bookmarks()->removeByUrl(wiki);
    tap();
    QCOMPARE(currentPage()->objectName(), QStringLiteral("bookmarksPage"));
    QCOMPARE(tabs->count(), open + 1);
    QCOMPARE(settings->quickAction(), int(Settings::QuickActionBookmark));
    QCOMPARE(settings->quickActionBookmark(), wikiId);

    // Every tap raised the window.
    QCOMPARE(m_window->property("activateCount").toInt(), taps);
    QVERIFY2(errors.all().isEmpty(), qPrintable(errors.all()));
}

// What the home screen is offered: the quick action alone while nothing plays, in the
// glyph of what it opens; with the tab in front's mute beside it while that plays or is
// muted; the mute alone when there is no quick action; and nothing when neither is there.
// Each list is one the home screen draws only while it is the one enabled.
void tst_qmlload::quickActionLists()
{
    ScriptErrors errors;
    Settings *settings = m_core->settings();
    TabModel *tabs = m_core->tabs();
    auto *coverItem = m_window->property("coverItem").value<QObject *>();
    const auto enabled = [coverItem]() {
        QStringList lists;
        for (const QString &name :
             {QStringLiteral("quickCoverActions"), QStringLiteral("mediaCoverActions"),
              QStringLiteral("muteCoverActions")}) {
            if (coverItem->findChild<QObject *>(name)->property("enabled").toBool()) {
                lists.append(name);
            }
        }
        return lists;
    };
    const auto picture = [coverItem](const QString &name) {
        return coverItem->findChild<QObject *>(name)->property("iconSource").toUrl();
    };
    const QString quick = QStringLiteral("quickCoverAction");

    QCOMPARE(enabled(), QStringList{QStringLiteral("quickCoverActions")});
    QVERIFY(drawnFrom(picture(quick), QStringLiteral("search-32-white.png")));
    const QList<QPair<Settings::QuickAction, QString>> glyphs{
        {Settings::QuickActionBookmarks, QStringLiteral("bookmarks")},
        {Settings::QuickActionDownloads, QStringLiteral("downloads")},
        {Settings::QuickActionHistory, QStringLiteral("history")},
        {Settings::QuickActionBookmark, QStringLiteral("globe")},
    };
    for (const auto &glyph : glyphs) {
        settings->setQuickAction(glyph.first);
        QVERIFY2(drawnFrom(picture(quick), glyph.second + QStringLiteral("-32-white.png")),
                 qPrintable(picture(quick).toString()));
    }
    // A bookmark's action wears the glyph picked for it.
    settings->setQuickActionIcon(QStringLiteral("music"));
    QVERIFY(drawnFrom(picture(quick), QStringLiteral("music-32-white.png")));

    // The tab in front muted: the action, and the mute beside it.
    tabs->setMuted(tabs->activeTabId(), true);
    QCOMPARE(enabled(), QStringList{QStringLiteral("mediaCoverActions")});
    QCOMPARE(picture(QStringLiteral("mediaQuickCoverAction")), picture(quick));
    QVERIFY(drawnFrom(picture(QStringLiteral("muteCoverAction")),
                      QStringLiteral("speaker-mute-32-white.png")));

    // No quick action: the mute alone, and it is the mute.
    settings->setQuickAction(Settings::QuickActionNone);
    QCOMPARE(enabled(), QStringList{QStringLiteral("muteCoverActions")});
    QVERIFY(drawnFrom(picture(QStringLiteral("loneMuteCoverAction")),
                      QStringLiteral("speaker-mute-32-white.png")));
    QMetaObject::invokeMethod(
        coverItem->findChild<QObject *>(QStringLiteral("loneMuteCoverAction")), "triggered");
    QVERIFY(!tabs->isMuted(tabs->activeTabId()));

    // Neither: nothing on the home screen.
    QCOMPARE(enabled(), QStringList());
    settings->setQuickAction(Settings::QuickActionSearch);
    QCOMPARE(enabled(), QStringList{QStringLiteral("quickCoverActions")});
    QVERIFY2(errors.all().isEmpty(), qPrintable(errors.all()));
}

// The choice of the quick action on the cover's settings page: each kind in the row's
// menu writes it, the row says it, and the pictures of the cover draw it. "Open a
// bookmark" asks which, and backing out has chosen nothing; under a bookmark's action are
// the glyphs it can wear, drawn from the files the cover hands the home screen.
void tst_qmlload::quickActionChoice()
{
    ScriptErrors errors;
    Settings *settings = m_core->settings();
    BookmarkModel *bookmarks = m_core->bookmarks();
    const int wiki = bookmarks->add(QStringLiteral("https://forest.example/wiki"),
                                    QStringLiteral("Forest wiki"));
    const int sea = bookmarks->add(QStringLiteral("https://sea.example/"), QStringLiteral("Sea"));
    openMenuItem(QStringLiteral("settingsMenuButton"));
    click(find(QStringLiteral("coverSettingsEntry")));
    QObject *page = currentPage();
    QObject *value = find(QStringLiteral("quickActionValue"));
    QObject *icons = find(QStringLiteral("quickActionIcons"));
    QObject *quiet = find(QStringLiteral("quietCoverPreview"));
    QObject *playing = find(QStringLiteral("playingCoverPreview"));
    const QString speaker = QStringLiteral("speaker-on");

    // A search until another is chosen: alone in the middle while nothing plays, on the
    // left of the mute while a tab plays, drawn from the cover's own files.
    QCOMPARE(value->property("text").toString(), QStringLiteral("Search"));
    QCOMPARE(previewGlyphs(quiet), QStringList{QStringLiteral("search")});
    QCOMPARE(previewGlyphs(playing), (QStringList{QStringLiteral("search"), speaker}));
    QVERIFY(drawnFrom(
        findObjects(quiet, QStringLiteral("previewAction")).first()->property("source").toUrl(),
        QStringLiteral("search-32-white.png")));
    QVERIFY(!icons->property("visible").toBool());

    struct Choice
    {
        QString item;
        Settings::QuickAction action;
        QString value;
        QString glyph;
    };
    const QList<Choice> choices{
        {QStringLiteral("quickAction-none"), Settings::QuickActionNone, QStringLiteral("None"),
         QString()},
        {QStringLiteral("quickAction-bookmarks"), Settings::QuickActionBookmarks,
         QStringLiteral("Bookmarks"), QStringLiteral("bookmarks")},
        {QStringLiteral("quickAction-downloads"), Settings::QuickActionDownloads,
         QStringLiteral("Downloads"), QStringLiteral("downloads")},
        {QStringLiteral("quickAction-history"), Settings::QuickActionHistory,
         QStringLiteral("History"), QStringLiteral("history")},
        {QStringLiteral("quickAction-search"), Settings::QuickActionSearch,
         QStringLiteral("Search"), QStringLiteral("search")},
    };
    for (const Choice &choice : choices) {
        QObject *item = find(choice.item);
        QCOMPARE(item->property("text").toString(), choice.value);
        click(item);
        QCOMPARE(settings->quickAction(), int(choice.action));
        QCOMPARE(value->property("text").toString(), choice.value);
        // No action is a dot in its place; while a tab plays, the mute is alone.
        QCOMPARE(previewGlyphs(quiet), QStringList{choice.glyph});
        QCOMPARE(previewGlyphs(playing), choice.glyph.isEmpty()
                                             ? QStringList{speaker}
                                             : (QStringList{choice.glyph, speaker}));
        QVERIFY(!icons->property("visible").toBool());
    }

    // "Open a bookmark" asks which, and backing out leaves the action as it was.
    QObject *bookmarkItem = find(QStringLiteral("quickAction-bookmark"));
    QCOMPARE(bookmarkItem->property("text").toString(), QStringLiteral("Open a bookmark"));
    click(bookmarkItem);
    QCOMPARE(currentPage()->objectName(), QStringLiteral("bookmarkPickerPage"));
    QCOMPARE(findAll(QStringLiteral("bookmarkPickerRow")).count(), 2);
    popPage();
    QCOMPARE(currentPage(), page);
    QCOMPARE(settings->quickAction(), int(Settings::QuickActionSearch));
    QCOMPARE(settings->quickActionBookmark(), 0);
    QCOMPARE(value->property("text").toString(), QStringLiteral("Search"));

    // The picker narrows to every word typed, says when nothing matches, and a tap
    // picks: the action is that bookmark's, and the page goes back.
    click(bookmarkItem);
    QObject *search = find(QStringLiteral("bookmarkPickerSearch"));
    search->setProperty("text", QStringLiteral("wiki forest"));
    QList<QObject *> rows = findAll(QStringLiteral("bookmarkPickerRow"));
    QCOMPARE(rows.count(), 1);
    QCOMPARE(textIn(rows.first(), QStringLiteral("bookmarkPickerTitle")),
             QStringLiteral("Forest wiki"));
    search->setProperty("text", QStringLiteral("desert"));
    QCOMPARE(findAll(QStringLiteral("bookmarkPickerRow")).count(), 0);
    QObject *placeholder = find(QStringLiteral("bookmarkPickerPlaceholder"));
    QVERIFY(placeholder->property("enabled").toBool());
    QCOMPARE(placeholder->property("text").toString(), QStringLiteral("No matches"));
    search->setProperty("text", QStringLiteral("forest"));
    click(findAll(QStringLiteral("bookmarkPickerRow")).first());
    QCOMPARE(currentPage(), page);
    QCOMPARE(settings->quickAction(), int(Settings::QuickActionBookmark));
    QCOMPARE(settings->quickActionBookmark(), wiki);
    QCOMPARE(settings->quickActionBookmarkUrl(), QStringLiteral("https://forest.example/wiki"));
    QCOMPARE(settings->quickActionBookmarkTitle(), QStringLiteral("Forest wiki"));
    QCOMPARE(value->property("text").toString(), QStringLiteral("Bookmark: Forest wiki"));

    // Under it, every glyph it can wear, the one it wears lit; a tap on another writes
    // it, and the pictures and the cover follow.
    QVERIFY(icons->property("visible").toBool());
    for (const QString &name : settings->quickActionIcons()) {
        QObject *cell = find(QStringLiteral("quickActionIcon-") + name);
        QVERIFY2(cell != nullptr, qPrintable(name));
        QVERIFY(drawnFrom(findObjects(cell, QStringLiteral("quickActionIconImage"))
                              .first()
                              ->property("source")
                              .toUrl(),
                          name + QStringLiteral("-32-white.png")));
        QCOMPARE(cell->property("highlighted").toBool(), name == QStringLiteral("globe"));
    }
    QCOMPARE(previewGlyphs(quiet), QStringList{QStringLiteral("globe")});
    click(find(QStringLiteral("quickActionIcon-heart")));
    QCOMPARE(settings->quickActionIcon(), QStringLiteral("heart"));
    QVERIFY(find(QStringLiteral("quickActionIcon-heart"))->property("highlighted").toBool());
    QVERIFY(!find(QStringLiteral("quickActionIcon-globe"))->property("highlighted").toBool());
    QCOMPARE(previewGlyphs(quiet), QStringList{QStringLiteral("heart")});
    QCOMPARE(previewGlyphs(playing), (QStringList{QStringLiteral("heart"), speaker}));
    auto *coverItem = m_window->property("coverItem").value<QObject *>();
    QVERIFY(drawnFrom(coverItem->findChild<QObject *>(QStringLiteral("quickCoverAction"))
                          ->property("iconSource")
                          .toUrl(),
                      QStringLiteral("heart-32-white.png")));

    // Chosen again, it picks again; the glyph stays.
    click(bookmarkItem);
    for (QObject *row : findAll(QStringLiteral("bookmarkPickerRow"))) {
        if (textIn(row, QStringLiteral("bookmarkPickerTitle")) == QStringLiteral("Sea")) {
            click(row);
            break;
        }
    }
    QCOMPARE(currentPage(), page);
    QCOMPARE(settings->quickActionBookmark(), sea);
    QCOMPARE(value->property("text").toString(), QStringLiteral("Bookmark: Sea"));
    QCOMPARE(settings->quickActionIcon(), QStringLiteral("heart"));

    // With no bookmarks at all, the picker says so.
    bookmarks->clear();
    click(bookmarkItem);
    placeholder = find(QStringLiteral("bookmarkPickerPlaceholder"));
    QVERIFY(placeholder->property("enabled").toBool());
    QCOMPARE(placeholder->property("text").toString(), QStringLiteral("No bookmarks"));
    popPage();
    QCOMPARE(settings->quickActionBookmark(), sea);
    QVERIFY2(errors.all().isEmpty(), qPrintable(errors.all()));
}

// The action's bookmark is kept by its id and found again by its address: renamed, the
// row calls it what it is called now; taken away and added back with the menu's
// Bookmark, under a new id, it is still the one; gone for good, the row says so, and the
// cover keeps the action, which opens the bookmarks then.
void tst_qmlload::quickActionBookmarkFollows()
{
    ScriptErrors errors;
    Settings *settings = m_core->settings();
    BookmarkModel *bookmarks = m_core->bookmarks();
    TabModel *tabs = m_core->tabs();
    const QString url = QStringLiteral("https://bee.example/");
    const int front = tabs->activeTabId();
    const int beeTab = tabs->newTab(url);
    const int first = bookmarks->add(url, QStringLiteral("Bee"));
    settings->setQuickActionBookmark(first, url, QStringLiteral("Bee"));
    settings->setQuickAction(Settings::QuickActionBookmark);
    const auto openCoverSettings = [this]() {
        openMenuItem(QStringLiteral("settingsMenuButton"));
        click(find(QStringLiteral("coverSettingsEntry")));
        return find(QStringLiteral("quickActionValue"));
    };
    const auto backToBrowser = [this]() {
        popPage();
        popPage();
    };
    QObject *value = openCoverSettings();
    QCOMPARE(value->property("text").toString(), QStringLiteral("Bookmark: Bee"));

    // Renamed, while the page is open: read again, and what the setting keeps of it with it.
    bookmarks->edit(bookmarks->count() - 1, url, QStringLiteral("Bee hive"));
    QCOMPARE(value->property("text").toString(), QStringLiteral("Bookmark: Bee hive"));
    QCOMPARE(settings->quickActionBookmarkTitle(), QStringLiteral("Bee hive"));
    backToBrowser();

    // The menu's Bookmark, tapped off and on: the bookmark is back under a new id, and
    // the action is pointed at it without a word.
    openMenuItem(QStringLiteral("bookmarkMenuButton"));
    QVERIFY(!bookmarks->contains(url));
    QCOMPARE(settings->quickActionBookmark(), first);
    openMenuItem(QStringLiteral("bookmarkMenuButton"));
    const int second = bookmarks->idForUrl(url);
    QVERIFY(second > 0 && second != first);
    QCOMPARE(settings->quickActionBookmark(), second);
    QCOMPARE(settings->quickActionBookmarkUrl(), url);
    QCOMPARE(settings->quickAction(), int(Settings::QuickActionBookmark));
    value = openCoverSettings();
    QCOMPARE(value->property("text").toString(),
             QStringLiteral("Bookmark: ") + bookmarks->titleOf(second));
    backToBrowser();
    auto *coverItem = m_window->property("coverItem").value<QObject *>();
    auto *action = coverItem->findChild<QObject *>(QStringLiteral("quickCoverAction"));
    tabs->activateTabById(front);
    QMetaObject::invokeMethod(action, "triggered");
    QCOMPARE(tabs->activeTabId(), beeTab);

    // Gone for good: the row says so, and the cover still offers the action, in the
    // glyph picked for it, which opens the bookmarks now.
    bookmarks->removeByUrl(url);
    value = openCoverSettings();
    QCOMPARE(value->property("text").toString(), QStringLiteral("Deleted bookmark"));
    QVERIFY(coverItem->findChild<QObject *>(QStringLiteral("quickCoverActions"))
                ->property("enabled")
                .toBool());
    QVERIFY(
        drawnFrom(action->property("iconSource").toUrl(), QStringLiteral("globe-32-white.png")));
    QMetaObject::invokeMethod(action, "triggered");
    QCOMPARE(currentPage()->objectName(), QStringLiteral("bookmarksPage"));
    QCOMPARE(pageStack()->property("depth").toInt(), 2);
    QVERIFY2(errors.all().isEmpty(), qPrintable(errors.all()));
}

void tst_qmlload::thumbnailCapturedOnLeavingTheApp()
{
    QObject *webView = currentWebView();
    webView->setProperty("loading", true);
    webView->setProperty("loading", false);
    const QString onLoad = webView->property("lastGrabPath").toString();
    QVERIFY(!onLoad.isEmpty());

    // Nothing is taken while the application is still the one on screen.
    QObject *page = find(QStringLiteral("browserPage"));
    QMetaObject::invokeMethod(page, "applicationStateChanged",
                              Q_ARG(QVariant, Qt::ApplicationActive));
    QCOMPARE(webView->property("lastGrabPath").toString(), onLoad);

    // Leaving it is the cover's last chance at a current picture of this tab.
    QMetaObject::invokeMethod(page, "applicationStateChanged",
                              Q_ARG(QVariant, Qt::ApplicationInactive));
    const QString onLeaving = webView->property("lastGrabPath").toString();
    QVERIFY(!onLeaving.isEmpty());
    QVERIFY(onLeaving != onLoad);
    QCOMPARE(m_core->tabs()->data(m_core->tabs()->index(0, 0), TabModel::ThumbnailRole).toString(),
             onLeaving);
}

// Out of sight for a moment, every loaded page is put to sleep -- unless one is making
// a sound -- and each wakes when its view is next on the screen. When is PageActivity's
// to say, and tst_pageactivity tests it; this is what the browsing page does about it.
void tst_qmlload::pagesSleepOutOfSight()
{
    const int firstTab = m_core->tabs()->activeTabId();
    m_core->tabs()->newTab(QStringLiteral("https://two.example/"));
    QList<QObject *> views = findAll(QStringLiteral("webView"));
    QCOMPARE(views.count(), 2);
    QObject *front = currentWebView();
    QObject *behind = views.at(0) == front ? views.at(1) : views.at(0);
    QObject *page = find(QStringLiteral("browserPage"));
    // Something of BrowserPage.qml's own, whose scope has the engine: the page item's
    // is the root file's.
    QObject *scope = find(QStringLiteral("viewArea"));
    Salama::PageActivity *activity = m_core->pageActivity();
    const auto calls = [](QObject *view, const char *name) {
        return view->property("calls").toStringList().count(QLatin1String(name));
    };
    const auto setState = [page](Qt::ApplicationState state) {
        QMetaObject::invokeMethod(page, "applicationStateChanged", Q_ARG(QVariant, state));
    };

    // The engine is asked for what says a page is playing, and after that for what it
    // says of downloads.
    QCOMPARE(evaluate(scope, QStringLiteral("WebEngine.observers")).toStringList(),
             activity->topics() + QStringList{m_core->downloads()->topic()});

    // Not the moment the application is left, but a moment after: every page, the one
    // behind the one in front as well. Until then the one in front stays active --
    // an inactive view's document is hidden, and a hidden document's media paused.
    QVERIFY(front->property("active").toBool());
    QVERIFY(!behind->property("active").toBool());
    setState(Qt::ApplicationInactive);
    QVERIFY(activity->background());
    QVERIFY(front->property("active").toBool());
    QCOMPARE(calls(front, "suspendView"), 0);
    QTRY_VERIFY(activity->asleep());
    QVERIFY(!front->property("active").toBool());
    QCOMPARE(calls(front, "suspendView"), 1);
    QCOMPARE(calls(behind, "suspendView"), 1);
    QVERIFY(front->property("suspended").toBool());

    // A document that arrives while its view is asleep is put to sleep with the rest.
    front->setProperty("loading", true);
    front->setProperty("loading", false);
    QCOMPARE(calls(front, "suspendView"), 3);

    // Back, and a view wakes as it goes active on the screen: the one in front now, the
    // one behind when it next comes to the front. Once each.
    setState(Qt::ApplicationActive);
    QVERIFY(!activity->asleep());
    QVERIFY(front->property("active").toBool());
    QCOMPARE(calls(front, "resumeView"), 1);
    QVERIFY(!front->property("suspended").toBool());
    QCOMPARE(calls(behind, "resumeView"), 0);
    QVERIFY(behind->property("suspended").toBool());
    // The one behind still sleeps, but a document arriving in it now is left awake:
    // suspending a view stops the one window every view draws into, and the page on
    // the screen with it.
    behind->setProperty("loading", true);
    behind->setProperty("loading", false);
    QCOMPARE(calls(behind, "suspendView"), 1);
    m_core->tabs()->activateTabById(firstTab);
    QVERIFY(behind->property("active").toBool());
    QCOMPARE(calls(behind, "resumeView"), 1);
    QVERIFY(!behind->property("suspended").toBool());
    // Awake, a load changes nothing.
    front->setProperty("loading", true);
    front->setProperty("loading", false);
    QCOMPARE(calls(front, "suspendView"), 3);
    m_core->tabs()->activateTabById(
        m_core->tabs()->data(m_core->tabs()->index(1, 0), TabModel::TabIdRole).toInt());
    QCOMPARE(currentWebView(), front);

    // Something with sound playing keeps every page awake out of sight.
    evaluate(scope, QStringLiteral("WebEngine.recvObserve('media-decoder-info',"
                                   " {owner: '0x1', state: 'meta', a: 1, v: 0})"));
    evaluate(scope, QStringLiteral("WebEngine.recvObserve('media-decoder-info',"
                                   " {owner: '0x1', state: 'play'})"));
    QVERIFY(activity->audible());
    setState(Qt::ApplicationInactive);
    QTest::qWait(activity->settleDelay() * 3 / 2);
    QVERIFY(!activity->asleep());
    QVERIFY(front->property("active").toBool());
    QCOMPARE(calls(front, "suspendView"), 3);
    QCOMPARE(calls(behind, "suspendView"), 1);
    setState(Qt::ApplicationActive);
}

// A page that plays something says so on the bar and on its preview, with a control to
// pause it and one to mute its tab; and while the tab in front plays, no other does
// (docs/DECISIONS/0026-media-controls.md). The engine's word is that something plays,
// not where: every loaded page is asked, and the stub's scriptResult is its answer.
// The rows along the grid's head and foot: their tint denser than Silica's overlay, at
// which the cells showed through them, and still short of opaque -- glass.
void tst_qmlload::gridRowsAreDenseGlass()
{
    QObject *headRow = find(QStringLiteral("gridHeadRow"));
    QObject *footRow = find(QStringLiteral("gridFootRow"));
    QVERIFY(headRow != nullptr);
    QVERIFY(footRow != nullptr);
    const QColor tint = headRow->property("color").value<QColor>();
    QVERIFY(tint.alphaF() > 0.8 && tint.alphaF() < 1.0);
    QCOMPARE(footRow->property("color"), headRow->property("color"));
}

void tst_qmlload::mediaControls()
{
    ScriptErrors errors;
    TabModel *tabs = m_core->tabs();
    Salama::PageMedia *media = m_core->pageMedia();
    const int first = tabs->activeTabId();
    QObject *behind = currentWebView();
    const int second = tabs->newTab(QStringLiteral("https://two.example/"));
    QObject *front = currentWebView();
    QVERIFY(front != behind);
    // A real window: the bar's row is laid out as it is drawn, and the grid's controls
    // are tapped with a finger.
    auto *root = qobject_cast<QQuickItem *>(m_window.data());
    FingerWindow fingers(root);
    QQuickWindow &window = *fingers.window();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto *bar = qobject_cast<QQuickItem *>(find(QStringLiteral("navigationBar")));
    QObject *scope = find(QStringLiteral("viewArea"));
    QObject *mute = find(QStringLiteral("muteButton"));
    QObject *host = find(QStringLiteral("addressLabel"));
    auto *coverItem = m_window->property("coverItem").value<QObject *>();
    auto *quickActions = coverItem->findChild<QObject *>(QStringLiteral("quickCoverActions"));
    auto *mediaActions = coverItem->findChild<QObject *>(QStringLiteral("mediaCoverActions"));
    auto *coverMute = coverItem->findChild<QObject *>(QStringLiteral("muteCoverAction"));
    const auto engineSays = [this, scope](const char *state) {
        evaluate(scope, QStringLiteral("WebEngine.recvObserve('media-decoder-info',"
                                       " {owner: '0x1', state: '%1'})")
                            .arg(QLatin1String(state)));
    };
    const auto asked = [](QObject *view) {
        QStringList commands;
        for (const QString &script : view->property("scripts").toStringList()) {
            if (script.contains(QLatin1String("var command = 'query'"))) {
                commands.append(QStringLiteral("query"));
            } else if (script.contains(QLatin1String("var command = 'pause'"))) {
                commands.append(QStringLiteral("pause"));
            } else if (script.contains(QLatin1String("var command = 'play'"))) {
                commands.append(QStringLiteral("play"));
            }
        }
        return commands;
    };
    const auto icon = [](QObject *item) { return item->property("source").toString(); };
    const auto coverIcon = [coverMute]() { return coverMute->property("iconSource").toUrl(); };
    const auto centreX = [bar](QObject *object) {
        auto *item = qobject_cast<QQuickItem *>(object);
        return item->mapToItem(bar, QPointF(item->width() / 2, 0)).x();
    };
    const auto regionOf = [this, bar, centreX](QObject *object) {
        return evaluate(bar, QStringLiteral("regionAt(%1)").arg(centreX(object))).toString();
    };

    // Nothing plays, and neither the bar nor the cover says anything of it.
    QVERIFY(!mute->property("visible").toBool());
    QVERIFY(quickActions->property("enabled").toBool());
    QVERIFY(!mediaActions->property("enabled").toBool());
    // Anchors put a centred item on a whole pixel: to within half of one.
    const auto centred = [bar, centreX](QObject *object) {
        return qAbs(centreX(object) - bar->width() / 2) <= 0.5;
    };
    QTRY_VERIFY(centred(host));

    // The engine says a decoder plays: a moment later every loaded page is asked once,
    // and the one that answers that it plays shows it.
    front->setProperty("scriptResult", QStringLiteral("playing"));
    engineSays("meta");
    engineSays("play");
    QVERIFY(asked(front).isEmpty());
    QTRY_COMPARE(tabs->mediaState(second), TabModel::MediaPlaying);
    QTest::qWait(media->queryDelay() * 2);
    QCOMPARE(asked(front), QStringList({QStringLiteral("query")}));
    QCOMPARE(asked(behind), QStringList({QStringLiteral("query")}));
    QCOMPARE(tabs->mediaState(first), TabModel::NoMedia);
    QVERIFY(mute->property("visible").toBool());
    QVERIFY(icon(mute).endsWith(QLatin1String("icon-m-speaker-on")));
    // In the ambience's colour, and a step larger than the small icons.
    QCOMPARE(mute->property("color").value<QColor>(), QColor(QStringLiteral("#aaccff")));
    QCOMPARE(mute->property("width").toReal(), qreal(48));
    // The cover offers it beside the quick action, in a picture of its own drawn for
    // this size and ambience.
    QVERIFY(!quickActions->property("enabled").toBool());
    QVERIFY(mediaActions->property("enabled").toBool());
    QVERIFY(coverIcon().toString().endsWith(QLatin1String("art/cover/speaker-on-32-white.png")));
    QVERIFY2(QFile::exists(coverIcon().toLocalFile()), qPrintable(coverIcon().toString()));

    // Left of the host, which stays where it was, in the middle of the bar: the mute
    // takes what lies between back and the host, and the host is still the address's.
    QTRY_COMPARE(regionOf(mute), QStringLiteral("mute"));
    QVERIFY(centred(host));
    QCOMPARE(regionOf(host), QStringLiteral("address"));
    QCOMPARE(evaluate(bar, QStringLiteral("regionAt(0)")).toString(), QStringLiteral("back"));
    QCOMPARE(evaluate(bar, QStringLiteral("regionAt(addressLeft)")).toString(),
             QStringLiteral("mute"));
    auto *muteItem = qobject_cast<QQuickItem *>(mute);
    QVERIFY(muteItem->mapToItem(bar, QPointF(muteItem->width(), 0)).x() <
            qobject_cast<QQuickItem *>(host)->mapToItem(bar, QPointF(0, 0)).x());

    // Muted from the bar: the flag is the tab's, and the page is paused at once, muted
    // as it is paused. Unmuted, it plays again what that paused.
    front->setProperty("scriptResult", QStringLiteral("paused"));
    tapBar(QStringLiteral("mute"));
    QVERIFY(tabs->isMuted(second));
    QCOMPARE(asked(front).last(), QStringLiteral("pause"));
    QVERIFY(front->property("lastScript").toString().contains(QLatin1String("muted = true,")));
    QCOMPARE(tabs->mediaState(second), TabModel::MediaPaused);
    QVERIFY(icon(mute).endsWith(QLatin1String("icon-m-speaker-mute")));
    QVERIFY(coverIcon().toString().endsWith(QLatin1String("speaker-mute-32-white.png")));
    QVERIFY(QFile::exists(coverIcon().toLocalFile()));
    front->setProperty("scriptResult", QStringLiteral("playing"));
    tapBar(QStringLiteral("mute"));
    QVERIFY(!tabs->isMuted(second));
    QCOMPARE(asked(front).last(), QStringLiteral("play"));
    QVERIFY(front->property("lastScript").toString().contains(QLatin1String("muted = false,")));
    QCOMPARE(tabs->mediaState(second), TabModel::MediaPlaying);

    // Left for another tab while it plays, it is paused while its view is still the
    // one in front -- before its page is told it is hidden -- and plays again when it
    // is back in front.
    QVERIFY(front->property("active").toBool());
    front->setProperty("scriptResult", QStringLiteral("paused"));
    tabs->activateTabById(first);
    QCOMPARE(asked(front).last(), QStringLiteral("pause"));
    QVERIFY(front->property("activeWhenRun").toList().last().toBool());
    QVERIFY(!front->property("active").toBool());
    QCOMPARE(tabs->mediaState(second), TabModel::MediaPaused);
    front->setProperty("scriptResult", QStringLiteral("playing"));
    tabs->activateTabById(second);
    QCOMPARE(asked(front).last(), QStringLiteral("play"));
    QCOMPARE(tabs->mediaState(second), TabModel::MediaPlaying);

    // The tab behind starts too, and is paused: the one in front plays. Behind, a page
    // that says it plays is held by the engine, and shows as paused.
    behind->setProperty("scriptResult", QStringLiteral("playing"));
    engineSays("play");
    QTRY_COMPARE(asked(behind).last(), QStringLiteral("pause"));
    QCOMPARE(tabs->shownMediaState(first), TabModel::MediaPaused);
    QCOMPARE(tabs->mediaState(second), TabModel::MediaPlaying);

    // Muted from the cover, as from the bar.
    front->setProperty("scriptResult", QStringLiteral("paused"));
    QMetaObject::invokeMethod(coverMute, "triggered");
    QVERIFY(tabs->isMuted(second));
    QCOMPARE(asked(front).last(), QStringLiteral("pause"));

    // A new page takes what the old one played with it; the tab stays muted, and the
    // mute stays where it can be undone.
    front->setProperty("loading", true);
    QCOMPARE(tabs->mediaState(second), TabModel::NoMedia);
    QVERIFY(mute->property("visible").toBool());
    QVERIFY(mediaActions->property("enabled").toBool());
    front->setProperty("scriptResult", QString());
    front->setProperty("loading", false);

    QVERIFY2(errors.all().isEmpty(), qPrintable(errors.all()));
}

// The mute over a tab's preview in the grid.
void tst_qmlload::muteOnTheGrid()
{
    ScriptErrors errors;
    TabModel *tabs = m_core->tabs();
    Salama::PageMedia *media = m_core->pageMedia();
    const int first = tabs->activeTabId();
    QObject *behind = currentWebView();
    const int second = tabs->newTab(QStringLiteral("https://two.example/"));
    auto *root = qobject_cast<QQuickItem *>(m_window.data());
    FingerWindow fingers(root);
    QQuickWindow &window = *fingers.window();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QObject *page = find(QStringLiteral("browserPage"));
    // The command in the last script the page was asked to run.
    const auto asked = [](QObject *view) {
        const QString script = view->property("lastScript").toString();
        for (const QString &command : {QStringLiteral("play"), QStringLiteral("pause")}) {
            if (script.contains(QStringLiteral("var command = '%1'").arg(command))) {
                return command;
            }
        }
        return QString();
    };
    const auto icon = [](QObject *item) { return item->property("source").toString(); };
    // The tab behind says it plays, which there means held, and goes on saying so each
    // time it is asked; the one in front plays nothing, and is muted.
    behind->setProperty("scriptResult", QStringLiteral("playing"));
    media->answer(first, Salama::PageMedia::Query, QStringLiteral("playing"));
    tabs->setMuted(second, true);

    // Under a finger. The tab behind is held by the engine and is not heard: its
    // speaker is struck through. A tap on it brings it to the front to be played,
    // with the grid staying open; heard, a tap silences it where it is. The cell is not
    // opened by either. While the mute is drawn, the picture under it fades out.
    pullUpToTabs();
    QTRY_COMPARE(page->property("tabsOffset").toReal(), page->property("fullHeight").toReal());
    QCOMPARE(tabs->shownMediaState(first), TabModel::MediaPaused);
    const QList<QObject *> cells = byRow(findAll(QStringLiteral("tabPreview")));
    QCOMPARE(cells.count(), 2);
    QObject *firstAction = findObjects(cells.at(0), QStringLiteral("previewMuteAction")).first();
    QObject *secondAction = findObjects(cells.at(1), QStringLiteral("previewMuteAction")).first();
    QObject *firstIcon = findObjects(cells.at(0), QStringLiteral("previewMuteIcon")).first();
    QVERIFY(firstAction->property("visible").toBool());
    QVERIFY(icon(firstIcon).endsWith(QLatin1String("icon-m-speaker-mute")));
    // Nothing plays in the tab in front, but it is muted, and says so.
    QVERIFY(secondAction->property("visible").toBool());
    QVERIFY(icon(findObjects(cells.at(1), QStringLiteral("previewMuteIcon")).first())
                .endsWith(QLatin1String("icon-m-speaker-mute")));
    QObject *firstPicture = findObjects(cells.at(0), QStringLiteral("tabPreviewPicture")).first();
    QVERIFY(evaluate(firstPicture, QStringLiteral("layer.enabled")).toBool());
    // Centred along the foot of the picture.
    auto *actionItem = qobject_cast<QQuickItem *>(firstAction);
    auto *shotItem = qobject_cast<QQuickItem *>(
        findObjects(cells.at(0), QStringLiteral("tabPreviewShot")).first());
    QCOMPARE(actionItem->x() + actionItem->width() / 2, shotItem->width() / 2);
    QCOMPARE(actionItem->y() + actionItem->height(), shotItem->height());

    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, centreOf(firstAction));
    QCOMPARE(tabs->activeTabId(), first);
    QCOMPARE(asked(behind), QStringLiteral("play"));
    QCOMPARE(tabs->shownMediaState(first), TabModel::MediaPlaying);
    QVERIFY(icon(firstIcon).endsWith(QLatin1String("icon-m-speaker-on")));
    QVERIFY(page->property("tabsOpen").toBool());
    behind->setProperty("scriptResult", QStringLiteral("paused"));
    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, centreOf(firstAction));
    QVERIFY(tabs->isMuted(first));
    QCOMPARE(asked(behind), QStringLiteral("pause"));
    QVERIFY(behind->property("lastScript").toString().contains(QLatin1String("muted = true,")));
    QCOMPARE(tabs->activeTabId(), first);
    QVERIFY(icon(firstIcon).endsWith(QLatin1String("icon-m-speaker-mute")));
    QVERIFY(page->property("tabsOpen").toBool());

    // A page that plays nothing and is not muted has no mute, and its picture runs to
    // the foot.
    behind->setProperty("scriptResult", QString());
    tabs->setMuted(first, false);
    media->forget(first);
    QVERIFY(!firstAction->property("visible").toBool());
    QVERIFY(!evaluate(firstPicture, QStringLiteral("layer.enabled")).toBool());
    QVERIFY2(errors.all().isEmpty(), qPrintable(errors.all()));
}

QTEST_MAIN(tst_qmlload)
#include "tst_qmlload.moc"
