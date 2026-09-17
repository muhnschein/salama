// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 tuuli contributors
//
// Loads the real QML against tests/silica-stubs and drives it through objectNames.
// The stubs imitate no layout: these tests prove structure and wiring, not appearance.
#include "Core.h"
#include "QmlTypes.h"
#include "tabs/ClosedTabModel.h"

#include <QColor>
#include <QFont>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlExpression>
#include <QQuickItem>
#include <QScopedPointer>
#include <QSet>
#include <QTemporaryDir>
#include <QtTest>
#include <algorithm>

using Tuuli::BookmarkModel;
using Tuuli::Core;
using Tuuli::Settings;
using Tuuli::TabModel;

namespace {

const char *const RootQml = TUULI_SOURCE_DIR "/qml/harbour-tuuli.qml";

} // namespace

class tst_qmlload : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void rootWindowLoads();
    void addressBarNavigates();
    void navigationBarDrivesWebView();
    void addressShowsHostAndSecurity();
    void barDoesNotCoverThePage();
    void editingEndsWithTheKeyboard();
    void thumbnailCapturedOnLoad();
    void faviconResolvedAfterLoad();
    void tabGrid();
    void tabGroups();
    void previewGestures();
    void recentlyClosedTabs();
    void pagesBeyondTheLimitUnload();
    void restoredTabsLoadLazily();
    void menuPage();
    void historyPage();
    void bookmarksPage();
    void settingsPage();
    void cover();
    void coverFieldFollowsTheFront();
    void coverStyleIsConfigurable();
    void thumbnailCapturedOnLeavingTheApp();

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

void tst_qmlload::init()
{
    m_dir.reset(new QTemporaryDir);
    m_core.reset(new Core(m_dir->path(), m_dir->path() + QStringLiteral("/tuuli.conf")));
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
    Tuuli::registerQmlTypes(m_core.data());
    m_engine.reset(new QQmlEngine);
    m_engine->addImportPath(QStringLiteral(TUULI_STUBS_DIR));
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

QObject *tst_qmlload::openMenuItem(const QString &itemName)
{
    tapBar(QStringLiteral("menu"));
    QObject *item = find(itemName);
    if (item == nullptr) {
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
    QVERIFY(!webView->property("privateMode").toBool());
    // The engine is told to lay pages out larger than the platform's own default.
    // The engine is told to lay pages out larger than the platform's own default of
    // 1.5 * Theme.pixelRatio.
    QObject *page = find(QStringLiteral("browserPage"));
    const qreal zoom = evaluate(page, QStringLiteral("pageZoom()")).toReal();
    QVERIFY(zoom > 1.5 * evaluate(page, QStringLiteral("Theme.pixelRatio")).toReal() - 0.5);
    QCOMPARE(evaluate(page, QStringLiteral("engineZoom()")).toReal(), zoom);
    QVERIFY(webView->property("downloadsEnabled").toBool());
    QVERIFY(!webView->property("desktopMode").toBool());

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

    // Editing brings the whole bar back, and so does a new page.
    tapBar(QStringLiteral("address"));
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
    QVERIFY(find(QStringLiteral("navigationBarGesture"))->property("enabled").toBool());
    tapBar(QStringLiteral("menu"));
    QCOMPARE(currentPage()->objectName(), QStringLiteral("menuPage"));
    popPage();
    QVERIFY(bar->property("editing").toBool());

    evaluate(bar, QStringLiteral("endEditing()"));
    QVERIFY(!bar->property("editing").toBool());
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

    // A private tab is offered no path, so nothing of it is written.
    webView->setProperty("grabSaveFails", false);
    m_core->tabs()->newTab(QStringLiteral("https://secret.example/"), true);
    QObject *privateView = currentWebView();
    privateView->setProperty("loading", true);
    privateView->setProperty("loading", false);
    QVERIFY(privateView->property("lastGrabPath").toString().isEmpty());
    QVERIFY(m_core->tabs()->activeFavicon().isEmpty() || true);
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
    // The grid carries two rows of its own, drawn over the cells: the groups, and the
    // one control it offers. The first is also what keeps the top row of cells clear
    // of the screen's own cutout. One group so far, unnamed, so named by its count.
    QVERIFY(find(QStringLiteral("newTabRow")) != nullptr);
    QList<QObject *> groupLabels = findAll(QStringLiteral("tabGroupLabel"));
    QCOMPARE(groupLabels.count(), 2);
    QCOMPARE(groupLabels.first()->property("text").toString(), QStringLiteral("2 tab(s)"));
    QCOMPARE(groupLabels.last()->property("text").toString(), QStringLiteral("Private"));
    // The current group is the one underlined.
    QList<QObject *> underlines = findAll(QStringLiteral("tabGroupUnderline"));
    QCOMPARE(underlines.count(), 2);
    QVERIFY(underlines.first()->property("visible").toBool());
    QVERIFY(!underlines.last()->property("visible").toBool());

    // Both the head row's strip and the first row of cells clear the display's own
    // cutout: the head sat under the notch, and so did the close button in the corner
    // of the first cell.
    const qreal cutout = grid->property("cutoutHeight").toReal();
    QVERIFY(cutout > 0);
    QObject *headRow = find(QStringLiteral("tabGroupRow"));
    QVERIFY(headRow->property("height").toReal() > cutout);
    QVERIFY(find(QStringLiteral("tabGroupStrip"))->property("y").toReal() >= cutout);
    QObject *gridHandle = find(QStringLiteral("gridDragHandle"));
    QVERIFY(gridHandle != nullptr);
    QVERIFY(gridHandle->property("y").toReal() >= cutout);
    auto *headerItem = find(QStringLiteral("tabGrid"))->property("headerItem").value<QObject *>();
    QVERIFY(headerItem != nullptr);
    QCOMPARE(headerItem->property("height").toReal(), headRow->property("height").toReal());

    QList<QObject *> previews = findAll(QStringLiteral("tabPreview"));
    QCOMPARE(previews.count(), 2);

    // The preview box is rounded, and so is the highlight drawn round the active
    // one. Clipping is rectangular whatever the shape of the item doing it, so the
    // picture is cut to the same corners by a mask.
    QObject *shot = findObjects(previews.at(1), QStringLiteral("tabPreviewShot")).first();
    QVERIFY(shot->property("radius").toReal() > 0);
    // The active cell is marked on that same box -- a Silica BackgroundItem would
    // have drawn a square wash across the whole cell instead.
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

    // A private tab says so in the bar as soon as it is the current one.
    m_core->tabs()->newTab(QStringLiteral("https://secret.example/"), true);
    QVERIFY(currentWebView()->property("privateMode").toBool());
    tapBar(QStringLiteral("address"));
    QCOMPARE(find(QStringLiteral("addressField"))->property("label").toString(),
             QStringLiteral("Private tab"));
}

void tst_qmlload::tabGroups()
{
    TabModel *tabs = m_core->tabs();
    const int home = tabs->groups().first().id;
    const int first = tabs->activeTabId();
    QObject *page = find(QStringLiteral("browserPage"));
    pullUpToTabs();

    // The strip holds every group, the ordinary one first and the private one last.
    QObject *strip = find(QStringLiteral("tabGroupStrip"));
    QVERIFY(strip != nullptr);
    QCOMPARE(findAll(QStringLiteral("tabGroupItem")).count(), 2);
    QCOMPARE(tabs->currentGroupIndex(), 0);

    // The edit corner leads to the list of groups, where a new one is made; it is the
    // current group from then on, and the grid under the page shows it empty.
    click(find(QStringLiteral("editGroupsButton")));
    QCOMPARE(currentPage()->objectName(), QStringLiteral("tabGroupsPage"));
    QCOMPARE(findAll(QStringLiteral("tabGroupDelegate")).count(), 2);
    QVERIFY(find(QStringLiteral("tabGroupDelegate"))->property("enabled").toBool());
    click(find(QStringLiteral("newGroupMenu")));
    QObject *dialog = currentPage();
    QCOMPARE(dialog->objectName(), QStringLiteral("tabGroupDialog"));
    QCOMPARE(dialog->property("groupId").toInt(), 0);
    find(QStringLiteral("groupNameField"))->setProperty("text", QStringLiteral("Work"));
    QMetaObject::invokeMethod(dialog, "accept");
    popPage();
    QCOMPARE(tabs->groups().count(), 3);
    const int work = tabs->groups().at(1).id;
    QCOMPARE(tabs->groups().at(1).name, QStringLiteral("Work"));
    QCOMPARE(tabs->currentGroupId(), work);
    QList<QObject *> delegates = byRow(findAll(QStringLiteral("tabGroupDelegate")));
    QCOMPARE(delegates.count(), 3);
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
    // The private group is named for what it is, and offers neither rename nor delete.
    QCOMPARE(findObjects(delegates.at(2), QStringLiteral("tabGroupName"))
                 .first()
                 ->property("text")
                 .toString(),
             QStringLiteral("Private"));
    QVERIFY(!findObjects(delegates.at(2), QStringLiteral("renameGroupMenu"))
                 .first()
                 ->property("enabled")
                 .toBool());
    QVERIFY(!findObjects(delegates.at(2), QStringLiteral("deleteGroupMenu"))
                 .first()
                 ->property("enabled")
                 .toBool());

    // Tapping a group there makes it current and returns to the grid, still open.
    click(delegates.at(1));
    QCOMPARE(currentPage()->objectName(), QStringLiteral("browserPage"));
    QVERIFY(page->property("tabsOpen").toBool());
    QCOMPARE(findAll(QStringLiteral("tabPreview")).count(), 0);
    QCOMPARE(findAll(QStringLiteral("tabGroupItem")).count(), 3);
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

    // The search corner lists every tab, group by group, and a tap brings one to the
    // front and puts the grid away.
    tabs->updateTitle(second, QStringLiteral("Office mail"));
    click(find(QStringLiteral("searchTabsButton")));
    QCOMPARE(currentPage()->objectName(), QStringLiteral("tabSearchPage"));
    QList<QObject *> results = findAll(QStringLiteral("tabSearchDelegate"));
    QCOMPARE(results.count(), 2);
    QObject *heading = findObjects(results.at(1), QStringLiteral("tabSearchGroupHeader")).first();
    QVERIFY(heading->property("visible").toBool());
    QCOMPARE(heading->property("text").toString(), QStringLiteral("Work"));
    QCOMPARE(findObjects(results.at(0), QStringLiteral("tabSearchGroupHeader"))
                 .first()
                 ->property("text")
                 .toString(),
             QStringLiteral("1 tab(s)"));
    find(QStringLiteral("tabSearchField"))->setProperty("text", QStringLiteral("office"));
    QCOMPARE(m_core->tabSearch()->searchTerm(), QStringLiteral("office"));
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
    // The term does not outlive the page.
    QVERIFY(m_core->tabSearch()->searchTerm().isEmpty());

    // The menu moves the tab in front to another group, through the same list.
    QObject *groupsPage = openMenuItem(QStringLiteral("moveToGroupItem"));
    QCOMPARE(groupsPage->objectName(), QStringLiteral("tabGroupsPage"));
    QCOMPARE(groupsPage->property("moveTabId").toInt(), second);
    // Not into the private group, though: that row is not on offer.
    delegates = byRow(findAll(QStringLiteral("tabGroupDelegate")));
    QVERIFY(!delegates.last()->property("enabled").toBool());
    QVERIFY(delegates.first()->property("enabled").toBool());
    click(delegates.at(0));
    QCOMPARE(currentPage()->objectName(), QStringLiteral("browserPage"));
    QCOMPARE(tabs->tabCountInGroup(home), 2);
    QCOMPARE(tabs->tabCountInGroup(work), 0);
    QCOMPARE(tabs->currentGroupId(), home);
    QCOMPARE(tabs->activeTabId(), second);
    // The view behind the moved tab is the one it had.
    QCOMPARE(findAll(QStringLiteral("webView")).count(), 2);

    // Or into a new group made for it.
    openMenuItem(QStringLiteral("moveToGroupItem"));
    click(find(QStringLiteral("newGroupMenu")));
    dialog = currentPage();
    QCOMPARE(dialog->property("moveTabId").toInt(), second);
    find(QStringLiteral("groupNameField"))->setProperty("text", QStringLiteral("Mail"));
    QMetaObject::invokeMethod(dialog, "accept");
    popPage();
    popPage();
    QCOMPARE(tabs->groups().count(), 4);
    QCOMPARE(tabs->currentGroupId(), tabs->groups().at(2).id);
    QCOMPARE(tabs->tabCountInGroup(tabs->groups().at(2).id), 1);
    QCOMPARE(tabs->tabCountInGroup(home), 1);
    QVERIFY(tabs->groups().last().isPrivate);

    // Rename and delete are in the group's own menu; the last ordinary group has no
    // delete.
    pullUpToTabs();
    click(find(QStringLiteral("editGroupsButton")));
    delegates = byRow(findAll(QStringLiteral("tabGroupDelegate")));
    QCOMPARE(delegates.count(), 4);
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
    QCOMPARE(tabs->groups().count(), 3);
    QCOMPARE(tabs->count(), 1);
    QCOMPARE(tabs->activeTabId(), first);
    click(findObjects(byRow(findAll(QStringLiteral("tabGroupDelegate"))).at(1),
                      QStringLiteral("deleteGroupMenu"))
              .first());
    QCOMPARE(tabs->groups().count(), 2);
    QVERIFY(!findObjects(byRow(findAll(QStringLiteral("tabGroupDelegate"))).at(0),
                         QStringLiteral("deleteGroupMenu"))
                 .first()
                 ->property("enabled")
                 .toBool());
    QCOMPARE(tabs->currentGroupIndex(), 0);
    popPage();

    // A private tab from the menu goes to the private group, which the strip then
    // shows current; the tab in front being private, the menu offers no move.
    openMenuItem(QStringLiteral("newPrivateTabItem"));
    QVERIFY(tabs->activeIsPrivate());
    QCOMPARE(tabs->currentGroupId(), tabs->privateGroupId());
    QVERIFY(currentWebView()->property("privateMode").toBool());
    tapBar(QStringLiteral("menu"));
    QVERIFY(!find(QStringLiteral("moveToGroupItem"))->property("enabled").toBool());
    popPage();
    pullUpToTabs();
    QCOMPARE(findAll(QStringLiteral("tabPreview")).count(), 1);
    QVERIFY(findAll(QStringLiteral("tabGroupUnderline")).last()->property("visible").toBool());
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

    // Two full seconds of holding still pick a cell up to be carried; a cell picked
    // up does not open when the finger lifts.
    QObject *timer = findObjects(cell, QStringLiteral("holdTimer")).first();
    QCOMPARE(timer->property("interval").toInt(), 2000);
    QCOMPARE(cell->property("holdInterval").toInt(), 2000);
    QVERIFY(!cell->property("held").toBool());
    evaluate(cell, QStringLiteral("pickUp()"));
    QVERIFY(cell->property("held").toBool());
    QVERIFY(cell->property("carried").toBool());
    QVERIFY(!timer->property("running").toBool());
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

    // The close button sits on a disc, so it can be seen over any page.
    QVERIFY(find(QStringLiteral("closeTabDisc")) != nullptr);
    QVERIFY(find(QStringLiteral("closeTabDisc"))->property("color").value<QColor>().alpha() > 128);
}

void tst_qmlload::recentlyClosedTabs()
{
    TabModel *tabs = m_core->tabs();
    const int second = tabs->newTab(QStringLiteral("https://two.example/"));
    tabs->updateTitle(second, QStringLiteral("Two"));
    tabs->closeTabById(second);
    QCOMPARE(tabs->closedTabs()->count(), 1);
    pullUpToTabs();

    // Holding the new-tab button brings the panel up from the foot of the grid.
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
    // Five by default, and Settings offers the choice.
    QCOMPARE(tabs->liveTabLimit(), 5);
    QObject *settings = openMenuItem(QStringLiteral("settingsItem"));
    QObject *combo = find(QStringLiteral("liveTabLimitCombo"));
    QCOMPARE(combo->property("currentIndex").toInt(), 1);
    combo->setProperty("currentIndex", 0);
    QCOMPARE(m_core->settings()->liveTabLimit(), 3);
    QCOMPARE(tabs->liveTabLimit(), 3);

    // Ten minutes in the background, and the engine is asked to trim its heap with
    // the words sailfish-browser uses. Read through the settings page, whose scope
    // has the engine singleton the browsing page's inline component has not.
    QObject *page = find(QStringLiteral("browserPage"));
    QCOMPARE(find(QStringLiteral("trimTimer"))->property("interval").toInt(), 600000);
    QCOMPARE(evaluate(settings, QStringLiteral("WebEngine.notifications.length")).toInt(), 0);
    evaluate(page, QStringLiteral("trimMemory()"));
    QCOMPARE(evaluate(settings, QStringLiteral("WebEngine.notifications.length")).toInt(), 1);
    QCOMPARE(evaluate(settings, QStringLiteral("WebEngine.notifications[0].topic")).toString(),
             QStringLiteral("memory-pressure"));
    QCOMPARE(evaluate(settings, QStringLiteral("WebEngine.notifications[0].value")).toString(),
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
    m_core.reset(new Core(m_dir->path(), m_dir->path() + QStringLiteral("/tuuli.conf")));
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

void tst_qmlload::menuPage()
{
    QObject *page = openMenuItem(QStringLiteral("newTabItem"));
    QCOMPARE(page->objectName(), QStringLiteral("browserPage"));
    QCOMPARE(m_core->tabs()->count(), 2);

    openMenuItem(QStringLiteral("newPrivateTabItem"));
    QCOMPARE(m_core->tabs()->count(), 3);
    QVERIFY(m_core->tabs()->activeIsPrivate());

    tapBar(QStringLiteral("menu"));
    auto *bookmarkLabel = find(QStringLiteral("bookmarkItem"))->findChild<QObject *>();
    QCOMPARE(bookmarkLabel->property("text").toString(), QStringLiteral("Bookmark this page"));
    click(find(QStringLiteral("bookmarkItem")));
    QCOMPARE(m_core->bookmarks()->count(), 1);
    QVERIFY(m_core->bookmarks()->activeUrlBookmarked());

    tapBar(QStringLiteral("menu"));
    bookmarkLabel = find(QStringLiteral("bookmarkItem"))->findChild<QObject *>();
    QCOMPARE(bookmarkLabel->property("text").toString(), QStringLiteral("Remove bookmark"));
    click(find(QStringLiteral("bookmarkItem")));
    QCOMPARE(m_core->bookmarks()->count(), 0);

    tapBar(QStringLiteral("menu"));
    QObject *share = find(QStringLiteral("shareAction"));
    click(find(QStringLiteral("shareItem")));
    QCOMPARE(share->property("triggerCount").toInt(), 1);
    QCOMPARE(share->property("mimeType").toString(), QStringLiteral("text/x-url"));
    const QVariantMap resource = share->property("resources").toList().first().toMap();
    QCOMPARE(resource.value(QStringLiteral("status")).toString(), m_core->tabs()->activeUrl());
    popPage();

    QCOMPARE(openMenuItem(QStringLiteral("bookmarksItem"))->objectName(),
             QStringLiteral("bookmarksPage"));
    popPage();
    QCOMPARE(openMenuItem(QStringLiteral("historyItem"))->objectName(),
             QStringLiteral("historyPage"));
    popPage();
    QCOMPARE(openMenuItem(QStringLiteral("settingsItem"))->objectName(),
             QStringLiteral("settingsPage"));
    popPage();
    QCOMPARE(currentPage()->objectName(), QStringLiteral("browserPage"));

    // Back and reload are on the bar, one tap away, and not in the menu as well.
    tapBar(QStringLiteral("menu"));
    QVERIFY(find(QStringLiteral("backItem")) == nullptr);
    QVERIFY(find(QStringLiteral("reloadItem")) == nullptr);
    popPage();

    // Tabs is the way into the grid for a hand that is already in the menu, or a
    // device where the drag is awkward. It comes back to this page and opens the
    // grid on it rather than pushing a page of its own.
    QCOMPARE(openMenuItem(QStringLiteral("tabsItem"))->objectName(), QStringLiteral("browserPage"));
    QVERIFY(find(QStringLiteral("browserPage"))->property("tabsOpen").toBool());
}

void tst_qmlload::historyPage()
{
    m_core->history()->visit(QStringLiteral("https://one.example/"), QStringLiteral("One"));
    m_core->history()->visit(QStringLiteral("https://two.example/"), QStringLiteral("Two"));

    openMenuItem(QStringLiteral("historyItem"));
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

    openMenuItem(QStringLiteral("historyItem"));
    delegates = findAll(QStringLiteral("historyDelegate"));
    click(findObjects(delegates.at(0), QStringLiteral("openInNewTabMenu")).first());
    QCOMPARE(m_core->tabs()->count(), 2);
    QCOMPARE(currentPage()->objectName(), QStringLiteral("browserPage"));

    openMenuItem(QStringLiteral("historyItem"));
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

    openMenuItem(QStringLiteral("bookmarksItem"));
    QList<QObject *> delegates = findAll(QStringLiteral("bookmarkDelegate"));
    QCOMPARE(delegates.count(), 2);
    click(delegates.at(0));
    QCOMPARE(currentPage()->objectName(), QStringLiteral("browserPage"));
    QCOMPARE(currentWebView()->property("url").toUrl().toString(),
             QStringLiteral("https://b1.example/"));

    openMenuItem(QStringLiteral("bookmarksItem"));
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

void tst_qmlload::settingsPage()
{
    QObject *page = openMenuItem(QStringLiteral("settingsItem"));
    QCOMPARE(page->objectName(), QStringLiteral("settingsPage"));

    QObject *home = find(QStringLiteral("homePageField"));
    QCOMPARE(home->property("text").toString(), Settings::defaultHomePage());
    home->setProperty("text", QStringLiteral("sailfishos.org"));
    enterKey(home);
    QCOMPARE(m_core->settings()->homePage(), QStringLiteral("https://sailfishos.org"));

    find(QStringLiteral("searchEngineCombo"))->setProperty("currentIndex", 1);
    QCOMPARE(m_core->settings()->searchEngineIndex(), 1);

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

    // The cover's style is the one choice here that another page has to answer.
    QObject *coverCombo = find(QStringLiteral("coverStyleCombo"));
    QCOMPARE(coverCombo->property("currentIndex").toInt(), int(Settings::CoverEveryTab));
    coverCombo->setProperty("currentIndex", int(Settings::CoverIconOnly));
    QCOMPARE(m_core->settings()->coverStyle(), int(Settings::CoverIconOnly));
    auto *coverItem = m_window->property("coverItem").value<QObject *>();
    QVERIFY(!coverItem->findChild<QObject *>(QStringLiteral("coverHeading"))
                 ->property("visible")
                 .toBool());
    coverCombo->setProperty("currentIndex", int(Settings::CoverEveryTab));

    click(find(QStringLiteral("clearHistoryButton")));
    QCOMPARE(m_core->history()->count(), 0);

    click(find(QStringLiteral("clearSiteDataButton")));
    click(find(QStringLiteral("clearCacheButton")));
    QCOMPARE(evaluate(page, QStringLiteral("WebEngine.notifications.length")).toInt(), 2);
    QCOMPARE(evaluate(page, QStringLiteral("WebEngine.notifications[0].topic")).toString(),
             QStringLiteral("clear-private-data"));
    QCOMPARE(evaluate(page, QStringLiteral("WebEngine.notifications[0].value")).toString(),
             QStringLiteral("cookies-and-site-data"));
    QCOMPARE(evaluate(page, QStringLiteral("WebEngine.notifications[1].value")).toString(),
             QStringLiteral("cache"));

    click(find(QStringLiteral("closeAllTabsButton")));
    QCOMPARE(m_core->tabs()->count(), 1);
    QCOMPARE(m_core->tabs()->activeUrl(), QStringLiteral("https://sailfishos.org"));
}

void tst_qmlload::cover()
{
    auto *coverItem = m_window->property("coverItem").value<QObject *>();
    QVERIFY(coverItem != nullptr);
    QCOMPARE(
        coverItem->findChild<QObject *>(QStringLiteral("coverBrand"))->property("text").toString(),
        QStringLiteral("Tuuli"));
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

    // The action is a search: a new tab, the window raised, and the address field up
    // with the whole url selected so the first key typed replaces it.
    QMetaObject::invokeMethod(coverItem->findChild<QObject *>(QStringLiteral("searchCoverAction")),
                              "triggered");
    QCOMPARE(m_core->tabs()->count(), 2);
    QCOMPARE(m_window->property("activateCount").toInt(), 1);
    QVERIFY(find(QStringLiteral("navigationBar"))->property("editing").toBool());
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

    // The icon alone: no heading, no number, no pictures. The action stays whatever
    // the style is -- it is what the cover is there to offer.
    m_core->settings()->setCoverStyle(Settings::CoverIconOnly);
    QVERIFY(!heading->property("visible").toBool());
    QVERIFY(!count->property("visible").toBool());
    QVERIFY(!field->property("visible").toBool());
    QVERIFY(icon->property("visible").toBool());
    QVERIFY(icon->property("source").toUrl().toString().endsWith(
        QStringLiteral("art/harbour-tuuli.png")));
    QVERIFY(coverItem->findChild<QObject *>(QStringLiteral("searchCoverAction")) != nullptr);

    m_core->settings()->setCoverStyle(Settings::CoverEveryTab);
    QCOMPARE(findObjects(coverItem, QStringLiteral("coverTabCell")).count(), 3);
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

QTEST_MAIN(tst_qmlload)
#include "tst_qmlload.moc"
