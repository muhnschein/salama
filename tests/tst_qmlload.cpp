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
#include "tabs/ThumbnailWriter.h"

#include <QColor>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFont>
#include <QGuiApplication>
#include <QImage>
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
#include <QRegularExpression>
#include <QSGRendererInterface>
#include <QScopedPointer>
#include <QSet>
#include <QSqlQuery>
#include <QStyleHints>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QUrl>
#include <QtTest>
#include <algorithm>
#include <utility>

using Salama::BookmarkModel;
using Salama::Core;
using Salama::CoverSettings;
using Salama::EngineMessages;
using Salama::NotificationPermissions;
using Salama::PrivacySettings;
using Salama::Reader;
using Salama::ReaderSettings;
using Salama::SearchEngines;
using Salama::SearchSettings;
using Salama::Settings;
using Salama::SitePermissions;
using Salama::SitePermissionSettings;
using Salama::StartPageSettings;
using Salama::TabModel;
using Salama::WebNotifications;

namespace {

const char *const RootQml = SALAMA_SOURCE_DIR "/qml/harbour-salama.qml";
// A site that shows notifications, as the frame script names a page's.
const char *const ChatSite = "https://chat.example";
// The page the tests start on, open in the one tab.
const char *const FirstPage = "https://www.qwant.com/";

} // namespace

// A site's OpenSearch descriptions, served from the loopback: what the page fetches with
// XMLHttpRequest when an engine found while browsing is tapped. Anything not in `pages`
// is a 404 with an HTML page, which is what a description's address often is by the time
// someone taps it.
class DescriptionServer : public QObject
{
public:
    explicit DescriptionServer(QMap<QString, QByteArray> pages)
        : m_pages(std::move(pages))
    {
        m_listening = m_server.listen(QHostAddress::LocalHost);
        connect(&m_server, &QTcpServer::newConnection, this, [this]() { accept(); });
    }

    bool isListening() const
    {
        return m_listening;
    }

    QString url(const QString &path) const
    {
        return QStringLiteral("http://127.0.0.1:%1%2").arg(m_server.serverPort()).arg(path);
    }

    int requests() const
    {
        return m_requests;
    }

private:
    void accept()
    {
        while (QTcpSocket *socket = m_server.nextPendingConnection()) {
            connect(socket, &QTcpSocket::readyRead, this, [this, socket]() { answer(socket); });
            connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        }
    }

    void answer(QTcpSocket *socket)
    {
        const QList<QByteArray> line = socket->readAll().split(' ');
        ++m_requests;
        const QString path = line.count() > 1 ? QString::fromLatin1(line.at(1)) : QString();
        const bool found = m_pages.contains(path);
        const QByteArray body = found ? m_pages.value(path) : QByteArray("<html>Not found</html>");
        socket->write(QByteArray(found ? "HTTP/1.1 200 OK\r\n" : "HTTP/1.1 404 Not Found\r\n") +
                      "Content-Type: text/xml\r\nConnection: close\r\nContent-Length: " +
                      QByteArray::number(body.size()) + "\r\n\r\n" + body);
        socket->disconnectFromHost();
    }

    QTcpServer m_server;
    QMap<QString, QByteArray> m_pages;
    bool m_listening = false;
    int m_requests = 0;
};

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
    void startsQuiet();
    void firstStartShowsTheStartPage();
    void startPage();
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
    void gridCellsArePicturesAlone();
    void gridRowsAreOpaque();
    void tabGroups();
    void tabGroupRows();
    void tabGroupsCarryAndUngroup();
    void tabSearch();
    void tabsDropOntoGroups();
    void previewGestures();
    void gridGesturesUnderAFinger();
    void gridHeadPullUnderAFinger();
    void carryToGroupUnderAFinger();
    void carryOverTheStripUnderAFinger();
    void tabGroupStripFades();
    void tabGroupsReorderUnderAFinger();
    void barReachUnderAFinger();
    void barDragStartsWithoutAStutter();
    void barStaysSlimWhileDragged();
    void recentlyClosedTabs();
    void pagesBeyondTheLimitUnload();
    void restoredTabsLoadLazily();
    void browserMenu();
    void menuSheetLayout();
    void menuNamesThePage();
    void menuShowsDownloadsComing();
    void menuSheetUnderAFinger();
    void findInPage();
    void readerView();
    void downloadsPage();
    void downloadAgain();
    void downloadBanner();
    void downloadBannerUnderAFinger();
    void linkMenuOnALongPress();
    void linkMenuActions();
    void linkMenuForOtherApps();
    void linkMenuForPictures();
    void linkPreview();
    void bannersEndThePage();
    void historyPage();
    void bookmarksPage();
    void settingsPage();
    void sailfishBrowserSettings();
    void startPageSettingsPage();
    void searchSettingsPage();
    void searchEnginesFound();
    void searchEnginesAdd();
    void searchEnginesRemove();
    void readerSettingsPage();
    void trackingSettingsPage();
    void historySettingsPage();
    void clearDataDialog();
    void coverSettingsPage();
    void cover();
    void coverWithNothingToSay();
    void coverShowsDownloads();
    void coverShowsWhatPlays();
    void quickActions();
    void quickActionLists();
    void quickActionChoice();
    void quickActionBookmarkFollows();
    void thumbnailCapturedOnLeavingTheApp();
    void pagesSleepOutOfSight();
    void mediaControls();
    void muteOnTheGrid();
    void webNotifications();
    void notificationPermissions();
    void notificationSettingsPage();
    void sitePermissionsPage();
    void sitePermissionsCookiesAndWaysOn();
    void siteExceptionsPage();
    void siteExceptionsAskEachTime();
    void menuHeadOpensSiteDetails();
    void siteDetailsConnection();
    void siteDetailsTrackingProtection();
    void siteDetailsPermissions();
    void tutorialOnFirstStart();
    void tutorial();
    void tutorialGrid();
    void tutorialUnderAFinger();

private:
    bool loadWindow();
    void forgetStartupMessages();
    bool startWithoutTabs(bool tutorialShown = true);
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
    // Most of these tests are about a page, and start with one open, as a session
    // restored with one tab does. A first start opens the start page instead
    // (firstStartShowsTheStartPage()), with the tutorial over it (tutorialOnFirstStart()),
    // which the rest have seen.
    // Opening it brings a tab to the front, and a moment later PageMedia asks the pages
    // what they play. That question is let through and answered here, before the test:
    // left pending, it came due in whichever test was slow enough to reach it, and the
    // stub page's answer -- nothing plays -- undid what the test had set playing.
    QSignalSpy pagesAsked(m_core->pageMedia(), &Salama::PageMedia::requested);
    m_core->tabs()->newTab(QLatin1String(FirstPage));
    m_core->settings()->setTutorialShown(true);
    QVERIFY(loadWindow());
    forgetStartupMessages();
    QTRY_VERIFY(!pagesAsked.isEmpty());
    QObject *view = currentWebView();
    view->setProperty("scripts", QStringList());
    view->setProperty("lastScript", QString());
    view->setProperty("activeWhenRun", QVariantList());
}

// What the browser tells the engine as it starts -- it asks for the sites allowed to
// send notifications (docs/DECISIONS/0033-web-notifications.md) -- is
// notificationsStart()'s to check; the others count what they send from nothing.
void tst_qmlload::forgetStartupMessages()
{
    evaluate(find(QStringLiteral("viewArea")), QStringLiteral("WebEngine.notifications = []"));
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

// A first start: a new data directory, and no tab to restore. The tutorial, which a
// first start shows, is taken as seen unless asked for.
bool tst_qmlload::startWithoutTabs(bool tutorialShown)
{
    cleanup();
    m_dir.reset(new QTemporaryDir);
    m_core.reset(new Core(m_dir->path(), m_dir->path() + QStringLiteral("/salama.conf"),
                          m_dir->path() + QStringLiteral("/Downloads/Salama")));
    m_core->settings()->setTutorialShown(tutorialShown);
    return loadWindow();
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
    evaluate(bar, QStringLiteral("dragArmed()"));
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
    QCOMPARE(m_core->tabs()->activeUrl(), QLatin1String(FirstPage));
    QVERIFY(!find(QStringLiteral("startPageLayer"))->property("active").toBool());

    QObject *webView = find(QStringLiteral("webView"));
    QVERIFY(webView != nullptr);
    QCOMPARE(currentWebView(), webView);
    QCOMPARE(webView->property("url").toUrl().toString(), QLatin1String(FirstPage));
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
    // holds: Standard, until it is changed; whether sites may ask to send
    // notifications, which they may until that is changed; and whether pages are drawn
    // dark, which they are as the ambience is -- the stub's is dark -- until that is
    // changed.
    QVariantList given =
        evaluate(find(QStringLiteral("viewArea")), QStringLiteral("WebEngineSettings.preferences"))
            .toList();
    const auto takeGiven = [&given](const QVariantMap &expected) {
        const auto preference =
            std::find_if(given.cbegin(), given.cend(), [&expected](const QVariant &one) {
                return one.toMap().value(QStringLiteral("key")) ==
                       expected.value(QStringLiteral("name"));
            });
        if (preference == given.cend() || preference->toMap().value(QStringLiteral("value")) !=
                                              expected.value(QStringLiteral("value"))) {
            return false;
        }
        given.removeAt(preference - given.cbegin());
        return true;
    };
    QVERIFY(takeGiven(NotificationPermissions::defaultPreference(false)));
    QVERIFY(
        takeGiven(EngineMessages::websiteColorPreferences(Settings::WebsiteColorsAutomatic, true)
                      .first()
                      .toMap()));
    // Do not track off and JavaScript on, as sailfish-browser starts.
    for (const QVariant &content : EngineMessages::contentPreferences(false, true)) {
        QVERIFY(takeGiven(content.toMap()));
    }
    // And the defaults of Site permissions: pop-ups blocked, the rest asked.
    for (const QVariant &preference :
         EngineMessages::sitePermissionPreferences(false, false, false, false)) {
        QVERIFY(takeGiven(preference.toMap()));
    }
    const QVariantList standard = EngineMessages::trackingProtectionPreferences(
        PrivacySettings::TrackingProtectionStandard, SitePermissionSettings::CookiesBlockCrossSite);
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

// A test starts with nothing of the start still to come, however slowly it then runs: a
// runner that took longer than PageMedia's delay before setting something playing saw
// the pages asked anyway, and the stub page's answer put the tab back to nothing playing
// (coverShowsWhatPlays() and mediaControls() failed on CI that way).
void tst_qmlload::startsQuiet()
{
    Salama::PageMedia *media = m_core->pageMedia();
    TabModel *tabs = m_core->tabs();
    const int front = tabs->activeTabId();
    // As a loaded runner would: nothing is handled for longer than the delay.
    QTest::qSleep(media->queryDelay() * 2);
    tabs->setMediaState(front, TabModel::MediaPlaying);
    QSignalSpy asked(media, &Salama::PageMedia::requested);
    QTest::qWait(media->queryDelay() * 2);
    QVERIFY(asked.isEmpty());
    QCOMPARE(tabs->mediaState(front), TabModel::MediaPlaying);
    QVERIFY(currentWebView()->property("scripts").toStringList().isEmpty());
}

// A first start opens one tab, on the start page: no view, no visit, and a bar that
// asks for an address (docs/DECISIONS/0032-start-page.md).
void tst_qmlload::firstStartShowsTheStartPage()
{
    QVERIFY(startWithoutTabs());
    TabModel *tabs = m_core->tabs();
    QCOMPARE(tabs->count(), 1);
    QVERIFY(tabs->activeUrl().isEmpty());
    QVERIFY(find(QStringLiteral("startPageLayer"))->property("active").toBool());
    QVERIFY(find(QStringLiteral("startPage")) != nullptr);
    QVERIFY(currentWebView() == nullptr);
    QVERIFY(find(QStringLiteral("webView")) == nullptr);
    QCOMPARE(m_core->history()->count(), 0);

    // Nothing visited and nothing bookmarked yet: the page says what will be there.
    QVERIFY(find(QStringLiteral("startPagePlaceholder"))->property("enabled").toBool());
    QVERIFY(!find(QStringLiteral("topSitesSection"))->property("visible").toBool());
    QVERIFY(!find(QStringLiteral("bookmarksSection"))->property("visible").toBool());
    QVERIFY(!find(QStringLiteral("recentPagesSection"))->property("visible").toBool());

    // Back and reload have no page to act on, and are dimmed.
    QCOMPARE(find(QStringLiteral("addressLabel"))->property("text").toString(),
             QStringLiteral("Search or enter address"));
    QVERIFY(!find(QStringLiteral("navigationBar"))->property("canGoBack").toBool());
    QVERIFY(find(QStringLiteral("backButton"))->property("opacity").toReal() < 1);
    QVERIFY(find(QStringLiteral("reloadButton"))->property("opacity").toReal() < 1);
    tapBar(QStringLiteral("back"));
    tapBar(QStringLiteral("reload"));
    QVERIFY(tabs->activeUrl().isEmpty());

    // Tapped, the address is a field with nothing in it.
    tapBar(QStringLiteral("address"));
    QVERIFY(find(QStringLiteral("addressField"))->property("text").toString().isEmpty());
    evaluate(find(QStringLiteral("navigationBar")), QStringLiteral("endEditing()"));

    // Closing it leaves a new one on the start page, and nothing to open again.
    tabs->closeActiveTab();
    QCOMPARE(tabs->count(), 1);
    QVERIFY(tabs->activeUrl().isEmpty());
    QCOMPARE(tabs->closedTabs()->count(), 0);
}

// What is opened from the start page opens in its tab, and back from the first page of
// it is the start page again. What it shows is what was visited and bookmarked, as
// Settings > Start page chooses (docs/DECISIONS/0032-start-page.md).
void tst_qmlload::startPage()
{
    QVERIFY(startWithoutTabs());
    TabModel *tabs = m_core->tabs();
    QObject *layer = find(QStringLiteral("startPageLayer"));
    QObject *bar = find(QStringLiteral("navigationBar"));

    typeAddress(QStringLiteral("example.org"));
    QCOMPARE(tabs->count(), 1);
    QCOMPARE(tabs->activeUrl(), QStringLiteral("https://example.org"));
    QVERIFY(!layer->property("active").toBool());
    QObject *view = currentWebView();
    QVERIFY(view != nullptr);
    QCOMPARE(view->property("url").toUrl().toString(), QStringLiteral("https://example.org"));
    QCOMPARE(m_core->history()->count(), 1);
    QVERIFY(bar->property("canGoBack").toBool());
    QVERIFY(find(QStringLiteral("reloadButton"))->property("opacity").toReal() == 1);
    // Stop, while the page loads, is sailfish-browser's plain cross, not a cross on a disc.
    view->setProperty("loading", true);
    QCOMPARE(find(QStringLiteral("reloadButton"))->property("source").toUrl(),
             QUrl(QStringLiteral("image://theme/icon-m-reset")));
    view->setProperty("loading", false);
    QCOMPARE(find(QStringLiteral("reloadButton"))->property("source").toUrl(),
             QUrl(QStringLiteral("image://theme/icon-m-refresh")));

    // Further on in the page, back is the page's; from its first page, the start page.
    view->setProperty("canGoBack", true);
    tapBar(QStringLiteral("back"));
    QCOMPARE(view->property("calls").toStringList(), QStringList{QStringLiteral("goBack")});
    QVERIFY(!tabs->activeUrl().isEmpty());
    view->setProperty("canGoBack", false);
    QVERIFY(bar->property("canGoBack").toBool());
    tapBar(QStringLiteral("back"));
    QVERIFY(tabs->activeUrl().isEmpty());
    QVERIFY(layer->property("active").toBool());
    QVERIFY(currentWebView() == nullptr);
    QVERIFY(find(QStringLiteral("webView")) == nullptr);
    QVERIFY(!bar->property("canGoBack").toBool());
    QCOMPARE(m_core->history()->count(), 1);

    // The page just read is on it: a tile for its site, lettered while it has no icon,
    // and a row for the page.
    QVERIFY(!find(QStringLiteral("startPagePlaceholder"))->property("enabled").toBool());
    QVERIFY(find(QStringLiteral("topSitesSection"))->property("visible").toBool());
    QList<QObject *> tiles = findAll(QStringLiteral("topSiteTile"));
    QCOMPARE(tiles.count(), 1);
    QCOMPARE(findObjects(tiles.first(), QStringLiteral("siteTileName")).first()->property("text"),
             QVariant(QStringLiteral("example.org")));
    QCOMPARE(findObjects(tiles.first(), QStringLiteral("siteTileLetter")).first()->property("text"),
             QVariant(QStringLiteral("E")));
    QList<QObject *> rows = findAll(QStringLiteral("recentPageRow"));
    QCOMPARE(rows.count(), 1);
    QCOMPARE(rows.first()->property("subtitle").toString(), QStringLiteral("https://example.org"));

    // A tile opens its site in the tab, and back returns from it too.
    click(tiles.first());
    QCOMPARE(tabs->activeUrl(), QStringLiteral("https://example.org"));
    QVERIFY(currentWebView() != nullptr);
    QVERIFY(bar->property("canGoBack").toBool());
    tapBar(QStringLiteral("back"));
    QVERIFY(tabs->activeUrl().isEmpty());

    // The grid names the start page's cell until its picture is taken.
    pullUpToTabs();
    QList<QObject *> previews = findAll(QStringLiteral("tabPreview"));
    QCOMPARE(previews.count(), 1);
    QCOMPARE(findObjects(previews.first(), QStringLiteral("tabPreviewPlaceholder"))
                 .first()
                 ->property("text")
                 .toString(),
             QStringLiteral("Start page"));
    pullDownToBrowser();

    // Bookmarks are tiles as well, under their own titles.
    m_core->bookmarks()->add(QStringLiteral("https://sailfishos.org/"),
                             QStringLiteral("Sailfish OS"));
    QVERIFY(find(QStringLiteral("bookmarksSection"))->property("visible").toBool());
    QList<QObject *> marks = findAll(QStringLiteral("bookmarkTile"));
    QCOMPARE(marks.count(), 1);
    QCOMPARE(findObjects(marks.first(), QStringLiteral("siteTileName")).first()->property("text"),
             QVariant(QStringLiteral("Sailfish OS")));

    // A row's menu opens the page in a new tab, or takes it out of the history.
    click(findObjects(findAll(QStringLiteral("recentPageRow")).first(),
                      QStringLiteral("recentPageNewTabMenu"))
              .first());
    QCOMPARE(tabs->count(), 2);
    QCOMPARE(tabs->activeUrl(), QStringLiteral("https://example.org"));
    tabs->activateTab(0);
    QVERIFY(layer->property("active").toBool());
    click(findObjects(findAll(QStringLiteral("recentPageRow")).first(),
                      QStringLiteral("recentPageRemoveMenu"))
              .first());
    QCOMPARE(m_core->history()->count(), 0);
    QVERIFY(!find(QStringLiteral("topSitesSection"))->property("visible").toBool());
    QVERIFY(!find(QStringLiteral("recentPagesSection"))->property("visible").toBool());
    QVERIFY(find(QStringLiteral("bookmarksSection"))->property("visible").toBool());

    // Settings choose the sections, and a blank start page shows nothing at all.
    StartPageSettings *settings = m_core->startPageSettings();
    settings->setBookmarks(false);
    QVERIFY(!find(QStringLiteral("bookmarksSection"))->property("visible").toBool());
    QVERIFY(find(QStringLiteral("startPagePlaceholder"))->property("enabled").toBool());
    settings->setBookmarks(true);
    settings->setBlank(true);
    QVERIFY(!find(QStringLiteral("bookmarksSection"))->property("visible").toBool());
    QVERIFY(!find(QStringLiteral("startPagePlaceholder"))->property("enabled").toBool());
    settings->setBlank(false);
    QVERIFY(find(QStringLiteral("bookmarksSection"))->property("visible").toBool());

    // A page opened from elsewhere -- the bookmarks, here -- goes into the tab too.
    QObject *bookmarksPage = openMenuItem(QStringLiteral("bookmarksMenuButton"));
    QCOMPARE(bookmarksPage->objectName(), QStringLiteral("bookmarksPage"));
    click(findAll(QStringLiteral("bookmarkDelegate")).first());
    QCOMPARE(currentPage()->objectName(), QStringLiteral("browserPage"));
    QCOMPARE(tabs->count(), 2);
    QCOMPARE(tabs->activeUrl(), QStringLiteral("https://sailfishos.org/"));
    QVERIFY(currentWebView() != nullptr);
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

// The omnibar's rows in the model's order: the list is laid out from the bottom up, so
// the first is the lowest.
QList<QObject *> omnibarRows(QObject *root)
{
    QList<QObject *> rows = byRow(findObjects(root, QStringLiteral("omnibarResult")));
    std::reverse(rows.begin(), rows.end());
    return rows;
}

// A row's title as it reads, without the bold of the words typed.
QString titleOf(QObject *row)
{
    return textIn(row, QStringLiteral("omnibarResultTitle"))
        .remove(QStringLiteral("<b>"))
        .remove(QStringLiteral("</b>"));
}

// The row the omnibar lists under this title.
QObject *omnibarRowTitled(QObject *root, const QString &title)
{
    for (QObject *row : omnibarRows(root)) {
        if (titleOf(row) == title) {
            return row;
        }
    }
    return nullptr;
}

// Whether what was typed has been learnt to lead to the address.
bool learnt(Core *core, const QString &typed, const QString &url)
{
    return core->history()->inputRanks(typed, QDateTime::currentMSecsSinceEpoch()).contains(url);
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
    // And opaque, the tint the grid's rows have: nothing of the page shows through.
    const QColor tint = find(QStringLiteral("omnibarTint"))->property("color").value<QColor>();
    QCOMPARE(tint.alphaF(), 1.0);
    QCOMPARE(tint, find(QStringLiteral("gridHeadRow"))->property("color").value<QColor>());

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
        m_core->searchEngines()->engineNames().at(m_core->searchSettings()->engineIndex());
    QCOMPARE(textIn(searchAction, QStringLiteral("omnibarActionTitle")),
             QStringLiteral("Search %1 for “forest”").arg(engine));
    QVERIFY(!find(QStringLiteral("omnibarGoAction"))->property("visible").toBool());
    QCOMPARE(m_core->omnibar()->query(), QString());
    QVERIFY(omnibarRows(root).isEmpty());
    QObject *debounce = find(QStringLiteral("omnibarDebounce"));
    QCOMPARE(debounce->property("interval").toInt(), 150);
    QMetaObject::invokeMethod(debounce, "triggered");
    QCOMPARE(m_core->omnibar()->query(), QStringLiteral("forest"));

    // One list, no headings, ranked: the bookmark, weighing most, then the rest by how
    // lately they were used and in the order they were found -- the tabs, the one in
    // front last first, and the history -- and the download last; laid out from the
    // bottom up, the first next to the rows that go and search. The tab in front is not
    // among them. The words typed are in bold. A tab says it is one, in the ambience's
    // colour, and in which group when it is in another; a page, its host; a download,
    // where it came from and how far along it is -- each quieter than a tab's.
    const QList<QObject *> rows = omnibarRows(root);
    QCOMPARE(rows.count(), 5);
    for (int i = 1; i < rows.count(); ++i) {
        QVERIFY(rows.at(i)->property("y").toReal() < rows.at(i - 1)->property("y").toReal());
    }
    const QStringList titles{
        QStringLiteral("<b>Forest</b> wiki"), QStringLiteral("<b>Forest</b> work"),
        QStringLiteral("<b>Forest</b> near"), QStringLiteral("<b>Forest</b> blog"),
        QStringLiteral("<b>forest</b>-map.pdf")};
    const QStringList details{
        QStringLiteral("<b>forest</b>.example"), QStringLiteral("Switch to tab in Work"),
        QStringLiteral("Switch to tab"), QStringLiteral("<b>forest</b>.example"),
        QStringLiteral("files.example · Downloading, 40%")};
    const QColor highlight = evaluate(pane, QStringLiteral("Theme.highlightColor")).value<QColor>();
    for (int i = 0; i < rows.count(); ++i) {
        QVERIFY(evaluate(rows.at(i), QStringLiteral("model.tabId")).toInt() != forest.front);
        QObject *title = findObjects(rows.at(i), QStringLiteral("omnibarResultTitle")).first();
        QCOMPARE(title->property("text").toString(), titles.at(i));
        QCOMPARE(title->property("textFormat"), evaluate(title, QStringLiteral("Text.StyledText")));
        QObject *detail = findObjects(rows.at(i), QStringLiteral("omnibarResultDetail")).first();
        QCOMPARE(detail->property("text").toString(), details.at(i));
        const bool tab = i == 1 || i == 2;
        QCOMPARE(detail->property("color").value<QColor>() == highlight, tab);
        QCOMPARE(detail->property("opacity").toReal() < 1, !tab);
        // No site's icon to be had: a tile with its initial for a page, the downloads'
        // glyph for a file.
        QObject *glyph = findObjects(rows.at(i), QStringLiteral("omnibarResultGlyph")).first();
        QObject *letter = findObjects(rows.at(i), QStringLiteral("omnibarResultLetter")).first();
        QCOMPARE(glyph->property("visible").toBool(), i == 4);
        QCOMPARE(letter->property("visible").toBool(), i != 4);
        QCOMPARE(shownIn(rows.at(i), QStringLiteral("omnibarResultProgress")), i == 4);
    }
    QCOMPARE(evaluate(rows.first(), QStringLiteral("initial")).toString(), QStringLiteral("F"));
    QCOMPARE(findObjects(rows.last(), QStringLiteral("omnibarResultGlyph"))
                 .first()
                 ->property("source")
                 .toUrl(),
             QUrl(QStringLiteral("image://theme/icon-m-downloads")));
    QVERIFY(findObjects(root, QStringLiteral("omnibarSection")).isEmpty());

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
// than making it again under the finger, and no more than eight are listed.
void tst_qmlload::omnibarFollowsItsSources()
{
    const Forest forest = plantForest(m_core.data());
    QObject *root = m_window.data();
    typeIntoBar(root, QStringLiteral("forest"));
    QCOMPARE(omnibarRows(root).count(), 5);

    m_core->bookmarks()->add(QStringLiteral("https://forest.example/camp"),
                             QStringLiteral("Forest camp"));
    QTRY_COMPARE(omnibarRows(root).count(), 6);
    QCOMPARE(titleOf(omnibarRows(root).at(1)), QStringLiteral("Forest camp"));
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
    const QString nearTitle = QStringLiteral("Forest near");
    const QPointer<QObject> near = omnibarRowTitled(root, nearTitle);
    QVERIFY(!near.isNull());
    tabs->updateFavicon(
        forest.near,
        QUrl::fromLocalFile(QStringLiteral(SALAMA_SOURCE_DIR "/icons/86x86/harbour-salama.png"))
            .toString());
    QTRY_VERIFY(shownIn(omnibarRowTitled(root, nearTitle), QStringLiteral("omnibarResultFavicon")));
    QVERIFY(!shownIn(omnibarRowTitled(root, nearTitle), QStringLiteral("omnibarResultLetter")));
    tabs->updateFavicon(
        forest.near, QUrl::fromLocalFile(m_dir->path() + QStringLiteral("/none.png")).toString());
    QTRY_VERIFY(shownIn(omnibarRowTitled(root, nearTitle), QStringLiteral("omnibarResultLetter")));
    QVERIFY(!shownIn(omnibarRowTitled(root, nearTitle), QStringLiteral("omnibarResultFavicon")));
    QVERIFY(!near.isNull());
    QCOMPARE(omnibarRowTitled(root, nearTitle), near.data());
    tabs->updateFavicon(forest.near, QString());

    for (int i = 0; i < 11; ++i) {
        m_core->history()->visit(QStringLiteral("https://forest.example/post/%1").arg(i),
                                 QStringLiteral("Forest post %1").arg(i));
    }
    QTRY_COMPARE(omnibarRows(root).count(), int(Salama::OmnibarModel::MaxRows));
    QCOMPARE(titleOf(omnibarRows(root).last()), QStringLiteral("forest-map.pdf"));
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
    // What was chosen is learnt: the same words lead there first next time.
    QVERIFY(learnt(m_core.data(), QStringLiteral("forest work"),
                   QStringLiteral("https://forest.example/work")));

    // A bookmark and a page of the history open in the tab in front.
    typeIntoBar(root, QStringLiteral("forest wiki"));
    QCOMPARE(omnibarRows(root).count(), 1);
    QCOMPARE(evaluate(omnibarRows(root).first(), QStringLiteral("model.kind")).toString(),
             QStringLiteral("bookmark"));
    click(omnibarRows(root).first());
    QVERIFY(!bar->property("editing").toBool());
    QCOMPARE(webView->property("url").toUrl(), QUrl(QStringLiteral("https://forest.example/wiki")));
    typeIntoBar(root, QStringLiteral("forest blog"));
    QCOMPARE(omnibarRows(root).count(), 1);
    QCOMPARE(evaluate(omnibarRows(root).first(), QStringLiteral("model.kind")).toString(),
             QStringLiteral("history"));
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
    QVERIFY(learnt(m_core.data(), QStringLiteral("forest.example/path"),
                   QStringLiteral("https://forest.example/path")));
    typeIntoBar(root, QStringLiteral("forest.example"));
    QVERIFY(find(QStringLiteral("omnibarGoAction"))->property("visible").toBool());
    click(find(QStringLiteral("omnibarSearchAction")));
    const QString searched = m_core->searchSettings()->searchUrl(QStringLiteral("forest.example"));
    QCOMPARE(webView->property("url").toUrl(), QUrl(searched));
    QCOMPARE(tabs->count(), open);
    // A search is not learnt.
    QVERIFY(!learnt(m_core.data(), QStringLiteral("forest.example"), searched));
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
    QCOMPARE(omnibarRows(root).count(), 1);
    QCOMPARE(titleOf(omnibarRows(root).first()), QStringLiteral("Forest wiki"));
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
    // An address entered is learnt, as one chosen is.
    QVERIFY(learnt(m_core.data(), QStringLiteral("example.org"),
                   QStringLiteral("https://example.org")));
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
    const auto thumbnail = [this]() {
        return m_core->tabs()
            ->data(m_core->tabs()->index(0, 0), roleId(TabModel::Role::Thumbnail))
            .toString();
    };

    webView->setProperty("loading", true);
    webView->setProperty("loading", false);
    QCOMPARE(webView->property("grabCount").toInt(), 1);
    // Grabbed at half size: the read back lands in the middle of a gesture, and the
    // grid never draws the picture wider than half the screen.
    QCOMPARE(webView->property("lastGrabSize").toSize().width(),
             int(webView->property("width").toReal() / 2));
    // The picture is handed to the model to encode and write on its worker: the GUI
    // thread never saves it, which in the grab callback was the stutter at the start
    // of the drag that opens the grid (issue #27).
    QVERIFY(webView->property("lastGrabPath").toString().isEmpty());
    QTRY_VERIFY(!thumbnail().isEmpty());
    const QString captured = thumbnail();
    QVERIFY(QFile::exists(captured));

    // A grab with nothing in it leaves the previous preview in place.
    webView->setProperty("grabSaveFails", true);
    webView->setProperty("loading", true);
    webView->setProperty("loading", false);
    m_core->tabs()->thumbnailWriter()->waitForDone();
    QCoreApplication::processEvents();
    QCOMPARE(thumbnail(), captured);
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

    // And whether it asked for the whole screen, cutout and all, which Automatic gives
    // it; a page that says nothing of it is kept below the cutout.
    QVERIFY(webView->property("scripts").toStringList().contains(
        m_core->engineMessages()->viewportScript()));
    auto *viewport = webView->property("viewport").value<QObject *>();
    QVERIFY(viewport != nullptr);
    QVERIFY(!viewport->property("coversCutout").toBool());
    webView->setProperty("scriptResult", QStringLiteral("width=device-width, viewport-fit=cover"));
    webView->setProperty("loading", true);
    webView->setProperty("loading", false);
    QVERIFY(viewport->property("coversCutout").toBool());
    QCOMPARE(find(QStringLiteral("browserPage"))->property("pageCutoutInset").toReal(), qreal(0));
    webView->setProperty("loading", true);
    QVERIFY(!viewport->property("coversCutout").toBool());
    webView->setProperty("loading", false);

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
        // The icon alone, nothing drawn behind it.
        QVERIFY(corner->childItems().isEmpty());
    }
    // New tab is the theme's ringed plus, whose ring is its own, and the same size of
    // button as the pencil across from it.
    QCOMPARE(
        item("newTabButton")->property("icon").value<QObject *>()->property("source").toString(),
        QStringLiteral("image://theme/icon-m-add"));
    QCOMPARE(item("editGroupsButton")
                 ->property("icon")
                 .value<QObject *>()
                 ->property("source")
                 .toString(),
             QStringLiteral("image://theme/icon-m-edit"));
    QCOMPARE(item("newTabButton")->width(), item("editGroupsButton")->width());
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

    // Opening the grid captured the tab being left, so that cell has a preview while
    // the one never displayed still shows its placeholder. The picture is written off
    // the GUI thread, so it arrives a moment later.
    QTRY_VERIFY(!m_core->tabs()
                     ->data(m_core->tabs()->index(1, 0), roleId(TabModel::Role::Thumbnail))
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
    QCOMPARE(currentWebView()->property("url").toUrl().toString(), QLatin1String(FirstPage));

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

    // That control opens a tab and hands the page back with it: the start page.
    click(find(QStringLiteral("newTabButton")));
    QCOMPARE(m_core->tabs()->count(), 2);
    QVERIFY(!page->property("tabsOpen").toBool());
    QVERIFY(m_core->tabs()->activeUrl().isEmpty());
    QVERIFY(find(QStringLiteral("startPageLayer"))->property("active").toBool());
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
    // The default group is named by its count, and has no menu: none of rename, ungroup
    // and delete applies to it.
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
    QVERIFY(delegates.at(0)->property("menu").value<QObject *>() == nullptr);
    QVERIFY(delegates.at(1)->property("menu").value<QObject *>() != nullptr);

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
    // It is on the start page, which needs no view; a page opened there has one.
    QVERIFY(currentWebView() == nullptr);
    typeAddress(QStringLiteral("work.example"));
    QCOMPARE(tabs->activeTabId(), second);
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

    // Rename and delete are in the group's own menu; the default group has no menu.
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
    click(findObjects(delegates.at(2), QStringLiteral("deleteGroupMenu")).first());
    QCOMPARE(tabs->groups().count(), 2);
    QCOMPARE(tabs->count(), 1);
    QCOMPARE(tabs->activeTabId(), first);
    click(findObjects(byRow(findAll(QStringLiteral("tabGroupDelegate"))).at(1),
                      QStringLiteral("deleteGroupMenu"))
              .first());
    QCOMPARE(tabs->groups().count(), 1);
    QVERIFY(byRow(findAll(QStringLiteral("tabGroupDelegate")))
                .at(0)
                ->property("menu")
                .value<QObject *>() == nullptr);
    QCOMPARE(tabs->currentGroupIndex(), 0);
    popPage();
}

namespace {

QString textOf(QObject *root, const char *name)
{
    return findObjects(root, QLatin1String(name)).first()->property("text").toString();
}

bool shownIn(QObject *root, const char *name)
{
    return findObjects(root, QLatin1String(name)).first()->property("visible").toBool();
}

} // namespace

// Each row of the list of groups: a picture of the group's tabs, its name and count, a
// grip and a menu -- neither on the default group -- and under the last row, one of the
// same height that makes a group, through a dialog that asks for a name and creates.
void tst_qmlload::tabGroupRows()
{
    TabModel *tabs = m_core->tabs();
    const int first = tabs->activeTabId();
    const int work = tabs->addGroup(QStringLiteral("Work"));
    pullUpToTabs();
    // A picture for the default group's to show, set once the grid has taken its own.
    const QString shot = tabs->thumbnailPath(first);
    tabs->updateThumbnail(first, shot);
    click(find(QStringLiteral("editGroupsButton")));
    QCOMPARE(currentPage()->objectName(), QStringLiteral("tabGroupsPage"));
    const QList<QObject *> rows = byRow(findAll(QStringLiteral("tabGroupDelegate")));
    QCOMPARE(rows.count(), 2);

    // An unnamed group is named by its count, and has nothing under its name; a named
    // one says its count under it. The default group has neither grip nor menu; the
    // others have both, and the menu renames, ungroups and deletes.
    QCOMPARE(textOf(rows.at(0), "tabGroupName"), QStringLiteral("1 tab(s)"));
    QVERIFY(!shownIn(rows.at(0), "tabGroupCount"));
    QCOMPARE(textOf(rows.at(1), "tabGroupName"), QStringLiteral("Work"));
    QCOMPARE(textOf(rows.at(1), "tabGroupCount"), QStringLiteral("0 tab(s)"));
    QVERIFY(rows.at(0)->property("menu").value<QObject *>() == nullptr);
    QVERIFY(!shownIn(rows.at(0), "tabGroupGrip"));
    auto *menu = rows.at(1)->property("menu").value<QObject *>();
    QVERIFY(menu != nullptr);
    for (const char *item : {"renameGroupMenu", "ungroupMenu", "deleteGroupMenu"}) {
        QCOMPARE(findObjects(menu, QLatin1String(item)).count(), 1);
    }
    QVERIFY(shownIn(rows.at(1), "tabGroupGrip"));

    // Each group's picture: the default group's one tab, the top left of four places,
    // and the new group, empty, the outline alone -- framed, being current.
    QObject *homePicture = findObjects(rows.at(0), QStringLiteral("tabGroupCollage")).first();
    QObject *workPicture = findObjects(rows.at(1), QStringLiteral("tabGroupCollage")).first();
    QCOMPARE(homePicture->property("tabCount").toInt(), 1);
    const QList<QObject *> cells = findObjects(homePicture, QStringLiteral("tabGroupCollageCell"));
    QCOMPARE(cells.count(), 4);
    QVERIFY(cells.at(0)->property("holdsTab").toBool());
    QVERIFY(!cells.at(1)->property("holdsTab").toBool());
    const auto imageIn = [](QObject *cell) {
        return findObjects(cell, QStringLiteral("tabGroupCollageImage"))
            .first()
            ->property("source")
            .toUrl();
    };
    QCOMPARE(imageIn(cells.at(0)), QUrl(QStringLiteral("file://") + shot));
    QVERIFY(imageIn(cells.at(1)).isEmpty());
    QVERIFY(shownIn(homePicture, "tabGroupCollagePicture"));
    QVERIFY(!shownIn(homePicture, "tabGroupCollageOutline"));
    QVERIFY(!shownIn(homePicture, "tabGroupCollageFrame"));
    QCOMPARE(workPicture->property("tabCount").toInt(), 0);
    QVERIFY(!shownIn(workPicture, "tabGroupCollagePicture"));
    QVERIFY(shownIn(workPicture, "tabGroupCollageOutline"));
    QVERIFY(shownIn(workPicture, "tabGroupCollageFrame"));
    // Square at the corners, frame and all, as Silica's pictures in a list are.
    QCOMPARE(findObjects(workPicture, QStringLiteral("tabGroupCollageFrame"))
                 .first()
                 ->property("radius")
                 .toReal(),
             qreal(0));

    // Under the last group, a row as tall as a group's with the theme's plus where a
    // group has its picture. Its dialog asks for a name and nothing else, and creates.
    QObject *newGroupRow = find(QStringLiteral("newGroupButton"));
    QCOMPARE(newGroupRow->property("contentHeight").toReal(),
             rows.at(0)->property("contentHeight").toReal());
    QCOMPARE(find(QStringLiteral("newGroupPlus"))->property("source").toUrl(),
             QUrl(QStringLiteral("image://theme/icon-m-add")));
    click(newGroupRow);
    QCOMPARE(currentPage()->objectName(), QStringLiteral("tabGroupDialog"));
    QObject *header = find(QStringLiteral("tabGroupDialogHeader"));
    QCOMPARE(header->property("title").toString(), QStringLiteral("New tab group"));
    QCOMPARE(header->property("acceptText").toString(), QStringLiteral("Create"));
    QObject *nameField = find(QStringLiteral("groupNameField"));
    QCOMPARE(nameField->property("label").toString(), QStringLiteral("Name"));
    QCOMPARE(nameField->property("placeholderText").toString(), QStringLiteral("Name"));
    popPage();
    QCOMPARE(tabs->groups().count(), 2);

    // Renaming, the same dialog saves.
    click(findObjects(rows.at(1), QStringLiteral("renameGroupMenu")).first());
    QCOMPARE(currentPage()->property("groupId").toInt(), work);
    header = find(QStringLiteral("tabGroupDialogHeader"));
    QCOMPARE(header->property("title").toString(), QStringLiteral("Rename tab group"));
    QCOMPARE(header->property("acceptText").toString(), QStringLiteral("Save"));
    popPage();
    popPage();
}

// A group carried by its grip trades places with the one its middle is carried into,
// and is lit while it is held; the default group stays first however far up anything
// is carried, and the strip takes the new order. Ungrouped, a group goes and its tabs
// stay open, in the default group, which the grid follows them to.
void tst_qmlload::tabGroupsCarryAndUngroup()
{
    TabModel *tabs = m_core->tabs();
    const int home = tabs->defaultGroupId();
    const int play = tabs->addGroup(QStringLiteral("Play"));
    const int mail = tabs->addGroup(QStringLiteral("Mail"));
    const int second = tabs->newTab(QStringLiteral("https://mail.example/"));
    QCOMPARE(tabs->currentGroupId(), mail);
    pullUpToTabs();
    click(find(QStringLiteral("editGroupsButton")));

    QObject *mailRow = byRow(findAll(QStringLiteral("tabGroupDelegate"))).at(2);
    const qreal rowHeight = mailRow->property("contentHeight").toReal();
    const qreal grab = mailRow->property("y").toReal() + rowHeight / 2;
    const auto carry = [&](const QString &call, qreal y) {
        evaluate(mailRow, QStringLiteral("%1(%2)").arg(call).arg(y));
    };
    carry(QStringLiteral("pickUp"), grab);
    QVERIFY(mailRow->property("carried").toBool());
    QVERIFY(shownIn(mailRow, "tabGroupCarryWash"));
    carry(QStringLiteral("carryTo"), grab - rowHeight * 0.4);
    QCOMPARE(tabs->groups().at(2).id, mail);
    carry(QStringLiteral("carryTo"), grab - rowHeight * 0.6);
    QCOMPARE(tabs->groups().at(1).id, mail);
    carry(QStringLiteral("carryTo"), grab - rowHeight * 3);
    QCOMPARE(tabs->groups().at(0).id, home);
    QCOMPARE(tabs->groups().at(1).id, mail);
    QCOMPARE(tabs->groups().at(2).id, play);
    evaluate(mailRow, QStringLiteral("drop()"));
    QVERIFY(!mailRow->property("carried").toBool());
    QVERIFY(!shownIn(mailRow, "tabGroupCarryWash"));
    QCOMPARE(tabs->currentGroupIndex(), 1);
    // The strip's row lays its names out in the order of its children, on its next frame.
    const auto stripName = [this](int place) {
        return textOf(qobject_cast<QQuickItem *>(find(QStringLiteral("tabGroupItem")))
                          ->parentItem()
                          ->childItems()
                          .at(place),
                      "tabGroupLabel");
    };
    QCOMPARE(stripName(1), QStringLiteral("Mail"));
    QCOMPARE(stripName(2), QStringLiteral("Play"));
    // The row it passed makes way rather than jump, so the list is read once it has.
    QTRY_COMPARE(textOf(byRow(findAll(QStringLiteral("tabGroupDelegate"))).at(1), "tabGroupName"),
                 QStringLiteral("Mail"));

    // Ungrouped, the group goes, and its tab is open in the default group, still in
    // front, with the grid showing the default group.
    click(findObjects(byRow(findAll(QStringLiteral("tabGroupDelegate"))).at(1),
                      QStringLiteral("ungroupMenu"))
              .first());
    QCOMPARE(tabs->groups().count(), 2);
    QCOMPARE(tabs->groupIndexOf(mail), -1);
    QCOMPARE(tabs->count(), 2);
    QCOMPARE(tabs->tabCountInGroup(home), 2);
    QCOMPARE(tabs->currentGroupId(), home);
    QCOMPARE(tabs->activeTabId(), second);
    QCOMPARE(findAll(QStringLiteral("tabPreview")).count(), 2);
    const QList<QObject *> rows = byRow(findAll(QStringLiteral("tabGroupDelegate")));
    QCOMPARE(rows.count(), 2);
    QCOMPARE(textOf(rows.at(0), "tabGroupName"), QStringLiteral("2 tab(s)"));
    QCOMPARE(findObjects(rows.at(0), QStringLiteral("tabGroupCollage"))
                 .first()
                 ->property("tabCount")
                 .toInt(),
             2);
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
    // What was typed is lit in each result, in the highlight colour, as Silica's own
    // search results light it (Theme.highlightText()); the rest is as it was.
    const QString highlight =
        evaluate(field, QStringLiteral("'' + Theme.highlightColor")).toString();
    const QString secondaryHighlight =
        evaluate(field, QStringLiteral("'' + Theme.secondaryHighlightColor")).toString();
    const auto line = [](QObject *result, const char *name) {
        return findObjects(result, QLatin1String(name)).first();
    };
    QCOMPARE(line(results.at(0), "tabRowTitle")->property("text").toString(),
             QStringLiteral("<font color=\"%1\">Office</font> hours").arg(highlight));
    QVERIFY(evaluate(line(results.at(0), "tabRowTitle"),
                     QStringLiteral("textFormat === Text.StyledText"))
                .toBool());
    QCOMPARE(line(results.at(0), "tabRowSubtitle")->property("text").toString(),
             QLatin1String(FirstPage));
    // Emptied, the field gives the cells back at once.
    field->setProperty("text", QString());
    QVERIFY(!found->property("visible").toBool());
    QVERIFY(cellsShown());
    // Two words, one in the title and the other in the address, in any case: each is lit
    // where it is, the address's in the secondary highlight colour its line is dimmed
    // to, and a word with a pattern's characters in it is only itself.
    field->setProperty("text", QStringLiteral("MAIL two"));
    QMetaObject::invokeMethod(debounce, "triggered");
    // Enter puts the keyboard away and leaves what was found.
    field->setProperty("focus", true);
    enterKey(field);
    QVERIFY(!field->property("focus").toBool());
    QVERIFY(found->property("visible").toBool());
    results = findAll(QStringLiteral("tabSearchDelegate"));
    QCOMPARE(results.count(), 1);
    QCOMPARE(line(results.at(0), "tabRowTitle")->property("text").toString(),
             QStringLiteral("Office <font color=\"%1\">mail</font>").arg(highlight));
    QCOMPARE(
        line(results.at(0), "tabRowSubtitle")->property("text").toString(),
        QStringLiteral("https://<font color=\"%1\">two</font>.example/").arg(secondaryHighlight));
    QVERIFY(evaluate(view, QStringLiteral("searchMatch.test('Two')")).toBool());
    m_core->tabSearch()->setSearchTerm(QStringLiteral("a.b (c"));
    QVERIFY(evaluate(view, QStringLiteral("searchMatch.test('a.b')")).toBool());
    QVERIFY(!evaluate(view, QStringLiteral("searchMatch.test('axb')")).toBool());
    QVERIFY(evaluate(view, QStringLiteral("searchMatch.test('(c')")).toBool());
    m_core->tabSearch()->setSearchTerm(QStringLiteral("  "));
    QVERIFY(evaluate(view, QStringLiteral("searchMatch === null")).toBool());
    m_core->tabSearch()->setSearchTerm(QStringLiteral("MAIL two"));
    results = findAll(QStringLiteral("tabSearchDelegate"));
    QCOMPARE(results.count(), 1);
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
    // seen on each cell -- at half, and opaque only under a finger, which
    // gridGesturesUnderAFinger() puts on it; there is no second disc under it. The disc
    // is in no colour of the ambience's but the ground Silica lays under what goes over
    // a picture, black in the stub's dark theme, and the cross on it is opaque in the
    // primary colour: the disc's colour carries its faintness, not the item.
    QObject *mark = find(QStringLiteral("closeTabMark"));
    QVERIFY(mark != nullptr);
    const QColor disc = mark->property("color").value<QColor>();
    QVERIFY(qAbs(disc.alphaF() - 0.5) < 0.01);
    QCOMPARE(disc.rgb(),
             evaluate(mark, QStringLiteral("Theme.overlayBackgroundColor")).value<QColor>().rgb());
    QCOMPARE(mark->property("opacity").toReal(), 1.0);
    const QList<QQuickItem *> cross = qobject_cast<QQuickItem *>(mark)->childItems();
    QCOMPARE(cross.count(), 3); // the two strokes and the Repeater that made them
    const qreal discWidth = mark->property("width").toReal();
    for (QQuickItem *stroke : cross) {
        if (stroke->property("rotation").toReal() != 0) {
            QCOMPARE(stroke->property("color").value<QColor>(),
                     evaluate(mark, QStringLiteral("Theme.primaryColor")).value<QColor>());
            // A thin cross, two fifths of the disc across.
            QCOMPARE(stroke->height(), evaluate(mark, QStringLiteral("Theme._lineWidth")).toReal());
            QCOMPARE(stroke->width(), discWidth * 2 / 5);
        }
    }
    QCOMPARE(mark->property("radius").toReal(), discWidth / 2);
    QVERIFY(find(QStringLiteral("closeTabDisc")) == nullptr);
    // About two thirds of the disc it was -- a small icon and a medium padding across --
    // while the target round it is as large as it was, and the disc in its middle.
    const qreal wasDisc =
        evaluate(mark, QStringLiteral("Theme.iconSizeSmall + Theme.paddingMedium")).toReal();
    QVERIFY(discWidth > wasDisc * 0.6);
    QVERIFY(discWidth < wasDisc * 0.75);
    auto *closeButton = qobject_cast<QQuickItem *>(find(QStringLiteral("closeTabButton")));
    QCOMPARE(closeButton->width(),
             evaluate(mark, QStringLiteral("Theme.iconSizeMedium + Theme.paddingSmall")).toReal());
    QCOMPARE(closeButton->height(), closeButton->width());
    QVERIFY(closeButton->width() > 2 * discWidth);
    QCOMPARE(qobject_cast<QQuickItem *>(mark)->x(), (closeButton->width() - discWidth) / 2);
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

    // Up, and at rest: a pull back to the page leaves the grid springing back from past
    // its top, and a press on a grid still moving stops it rather than reaching the cell
    // under the finger -- which the deck settling first does not rule out.
    QObject *gridView = find(QStringLiteral("tabGrid"));
    const auto openGrid = [&]() {
        pullUpToTabs();
        QTRY_COMPARE(page->property("tabsOffset").toReal(), page->property("fullHeight").toReal());
        QTRY_VERIFY(!gridView->property("moving").toBool());
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
    const auto discAlpha = [mark]() { return mark->property("color").value<QColor>().alphaF(); };
    QVERIFY(discAlpha() < 1.0);
    const QPoint onMark = centreOf(mark);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, onMark);
    QCOMPARE(discAlpha(), 1.0);
    const QPoint offMark = onMark - QPoint(3 * mark->property("width").toInt(), 0);
    QTest::mouseMove(&window, offMark);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, offMark);
    QVERIFY(discAlpha() < 1.0);
    QCOMPARE(tabs->count(), 2);
    QVERIFY(page->property("tabsOpen").toBool());

    // Held still for the hold interval -- or all but still: a thumb drifts, and a
    // drift short of a drag is still a hold -- a cell comes up and is carried to
    // another place in the grid, and letting go of it opens nothing.
    openGrid();
    QObject *first = cells().first();
    const QPoint grab = centreOf(first);
    const int firstId = tabs->data(tabs->index(0, 0), roleId(TabModel::Role::TabId)).toInt();
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, grab);
    QTest::mouseMove(&window, grab + QPoint(first->property("holdTolerance").toInt() / 2, 2));
    QTRY_VERIFY(first->property("held").toBool());
    const QPoint target = centreOf(cells().last());
    QTest::mouseMove(&window, (grab + target) / 2);
    QTest::mouseMove(&window, target);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, target);
    QCOMPARE(tabs->data(tabs->index(1, 0), roleId(TabModel::Role::TabId)).toInt(), firstId);
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

// A drag down the grid's head row brings the page back from wherever the grid is
// scrolled to: from anywhere else, a grid longer than the screen scrolls back to its
// top first. The row lies over the search field, so it has to hand the field the taps
// it takes, and a press on the field's clear button, and a drag up to the grid.
void tst_qmlload::gridHeadPullUnderAFinger()
{
    TabModel *tabs = m_core->tabs();
    for (int i = 0; i < 11; ++i) {
        tabs->newTab(QStringLiteral("https://more.example/%1").arg(i));
    }
    QObject *page = find(QStringLiteral("browserPage"));
    QObject *grid = find(QStringLiteral("tabGrid"));
    QObject *gesture = find(QStringLiteral("gridHeadPull"));
    auto *field = qobject_cast<QQuickItem *>(find(QStringLiteral("tabSearchField")));
    auto *root = qobject_cast<QQuickItem *>(m_window.data());
    FingerWindow host(root);
    QQuickWindow &window = *host.window();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    const ScriptErrors errors;
    const auto openGrid = [&]() {
        pullUpToTabs();
        QTRY_COMPARE(page->property("tabsOffset").toReal(), page->property("fullHeight").toReal());
        QTRY_VERIFY(!grid->property("moving").toBool());
    };
    const qreal threshold = page->property("pullThreshold").toReal();

    // Well down a long group, a pull down the row is the page's at once, and the grid
    // is where it was when it comes up again.
    openGrid();
    const QPoint head = centreOf(find(QStringLiteral("gridHeadControls")));
    const qreal scrolled =
        grid->property("contentY").toReal() + grid->property("cellHeight").toReal();
    grid->setProperty("contentY", scrolled);
    drag(&window, head, head + QPoint(0, 3 * int(threshold)));
    QVERIFY(!page->property("tabsOpen").toBool());
    QTRY_COMPARE(page->property("tabsOffset").toReal(), qreal(0));
    QCOMPARE(grid->property("contentY").toReal(), scrolled);

    // Halfway down, the row of cells cut by the grid's top edge reaches up over the
    // page and its bar, and the view clips it there rather than drawing it on them.
    openGrid();
    auto *view = qobject_cast<QQuickItem *>(find(QStringLiteral("tabsView")));
    const QPoint pulledTo = head + QPoint(0, 3 * int(threshold));
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, head);
    for (int step = 1; step <= 24; ++step) {
        QTest::mouseMove(&window, head + (pulledTo - head) * step / 24);
    }
    const qreal viewTop = view->mapToScene(QPointF(0, 0)).y();
    QVERIFY(viewTop > 0);
    const QList<QObject *> previews = findAll(QStringLiteral("tabPreview"));
    QVERIFY(std::any_of(previews.begin(), previews.end(), [viewTop](QObject *cell) {
        auto *item = qobject_cast<QQuickItem *>(cell);
        return item->isVisible() && item->mapToScene(QPointF(0, 0)).y() < viewTop;
    }));
    QVERIFY(view->clip());
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, pulledTo);
    QVERIFY(!page->property("tabsOpen").toBool());
    QTRY_COMPARE(page->property("tabsOffset").toReal(), qreal(0));

    // Short of the threshold the grid settles back, still scrolled.
    openGrid();
    QCOMPARE(grid->property("contentY").toReal(), scrolled);
    drag(&window, head, head + QPoint(0, int(threshold) / 2));
    QVERIFY(page->property("tabsOpen").toBool());
    QCOMPARE(grid->property("contentY").toReal(), scrolled);
    QVERIFY(!field->hasFocus());

    // Up is still the grid's, to scroll further down the group.
    drag(&window, head, head - QPoint(0, head.y() * 3 / 4));
    QTRY_VERIFY(!grid->property("moving").toBool());
    QVERIFY(grid->property("contentY").toReal() > scrolled);
    QVERIFY(page->property("tabsOpen").toBool());
    QVERIFY(!field->hasFocus());

    // A press on the clear button is left to the button, and a tap anywhere else on
    // the row is the field's. The stub field has no button of its own; this one counts
    // its taps.
    QQmlComponent button(m_engine.data());
    button.setData("import QtQuick 2.6\n"
                   "MouseArea { property int taps: 0; onClicked: taps += 1 }",
                   QUrl());
    QScopedPointer<QQuickItem> clear(qobject_cast<QQuickItem *>(button.create()));
    QVERIFY2(clear, qPrintable(button.errorString()));
    clear->setParentItem(field);
    clear->setSize(QSizeF(field->height(), field->height()));
    clear->setX(field->width() - clear->width());
    field->setProperty("rightItem", QVariant::fromValue(clear.data()));
    field->setProperty("text", QStringLiteral("more"));
    const QPoint onClear = centreOf(clear.data());
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, onClear);
    QVERIFY(!gesture->property("pressed").toBool());
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, onClear);
    QCOMPARE(clear->property("taps").toInt(), 1);
    QVERIFY(!field->hasFocus());
    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, head);
    QCOMPARE(clear->property("taps").toInt(), 1);
    QVERIFY(field->hasFocus());
    QVERIFY(page->property("tabsOpen").toBool());
    field->setProperty("rightItem", QVariant::fromValue<QQuickItem *>(nullptr));
    QVERIFY2(errors.all().isEmpty(), qPrintable(errors.all()));
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
            ids.append(tabs->data(tabs->index(row, 0), roleId(TabModel::Role::TabId)).toInt());
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
    // Silica's wash for a chosen item, with rounded corners, as the pictures have.
    QObject *wash = findAll(QStringLiteral("tabGroupDropHighlight")).at(1);
    QCOMPARE(wash->property("radius").toReal(),
             evaluate(wash, QStringLiteral("Theme.paddingSmall")).toReal());
    QCOMPARE(wash->property("color").value<QColor>(),
             evaluate(wash, QStringLiteral("Theme.rgba(Theme.highlightBackgroundColor,"
                                           " Theme.highlightBackgroundOpacity)"))
                 .value<QColor>());
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
            ids.append(tabs->data(tabs->index(row, 0), roleId(TabModel::Role::TabId)).toInt());
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

// A group carried by its grip under a real finger, in a real window. Whether the list
// leaves the grip a drag up or down, rather than taking it to scroll, is decided inside
// Qt's own delivery, which calling the grip's functions skips.
void tst_qmlload::tabGroupsReorderUnderAFinger()
{
    TabModel *tabs = m_core->tabs();
    // More groups than the screen holds, so the list has somewhere to scroll to.
    for (int i = 0; i < 24; ++i) {
        tabs->addGroup(QStringLiteral("Group %1").arg(i));
    }
    pullUpToTabs();
    click(find(QStringLiteral("editGroupsButton")));
    QCOMPARE(currentPage()->objectName(), QStringLiteral("tabGroupsPage"));
    QObject *list = find(QStringLiteral("tabGroupsList"));
    auto *root = qobject_cast<QQuickItem *>(m_window.data());
    FingerWindow host(root);
    QQuickWindow &window = *host.window();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    ScriptErrors errors;

    // Two rows down by the grip: the group is two places further on, the list has not
    // moved, and the row is put down.
    QList<QObject *> rows = byRow(findAll(QStringLiteral("tabGroupDelegate")));
    QObject *carried = rows.at(1);
    const int rowHeight = carried->property("contentHeight").toInt();
    const int moved = tabs->groups().at(1).id;
    const qreal top = list->property("contentY").toReal();
    const QPoint grip = centreOf(findObjects(carried, QStringLiteral("tabGroupGrip")).first());
    drag(&window, grip, grip + QPoint(0, rowHeight * 2));
    QCOMPARE(tabs->groups().at(3).id, moved);
    QCOMPARE(tabs->groups().at(0).id, tabs->defaultGroupId());
    QCOMPARE(list->property("contentY").toReal(), top);
    QVERIFY(!carried->property("carried").toBool());
    QVERIFY2(errors.all().isEmpty(), qPrintable(errors.all()));

    // Anywhere else on a row, the drag is the list's, to scroll, and no group moves.
    rows = byRow(findAll(QStringLiteral("tabGroupDelegate")));
    const QPoint name = centreOf(findObjects(rows.at(2), QStringLiteral("tabGroupName")).first());
    drag(&window, name, name - QPoint(0, rowHeight * 3));
    QTRY_VERIFY(!list->property("moving").toBool());
    QVERIFY(list->property("contentY").toReal() > top);
    QCOMPARE(tabs->groups().at(3).id, moved);
    popPage();
}

// Issue #27: the very start of the drag up from the bar stuttered, every time. Three
// things landed in its first frame: the deck leapt Theme.startDragDistance at once,
// because the distance was measured from the press rather than from where the drag was
// caught; the grid was drawn for the first time, every preview it shows uploaded in
// that one frame; and the picture of the tab being left was taken, its PNG encoded on
// the GUI thread. Under a real finger: the picture and the grid are seen to while the
// finger is still down and nothing moves, and the deck then follows the finger from
// where the drag was caught, pixel for pixel.
void tst_qmlload::barDragStartsWithoutAStutter()
{
    FingerWindow host(m_window.data());
    QQuickWindow &window = *host.window();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QObject *page = find(QStringLiteral("browserPage"));
    QObject *grid = find(QStringLiteral("tabsView"));
    QObject *webView = currentWebView();
    auto *gesture = qobject_cast<QQuickItem *>(find(QStringLiteral("navigationBarGesture")));
    const qreal reach = gesture->property("reach").toReal();
    const QPointF gestureTop = gesture->mapToScene(QPointF(0, 0));
    const int x = int(gesture->width()) / 2;
    const int onBar = int(gestureTop.y() + reach + gesture->property("strip").toReal() / 2);
    const int inReach = int(gestureTop.y() + reach / 2);
    const int shake = evaluate(page, QStringLiteral("Theme.startDragDistance")).toInt();
    const qreal threshold = page->property("pullThreshold").toReal();
    const auto offset = [page]() { return page->property("tabsOffset").toReal(); };
    const int grabs = webView->property("grabCount").toInt();
    QVERIFY(!grid->property("visible").toBool());

    // Down: the picture is taken and the grid drawn, out of sight below the page.
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(x, onBar));
    QCOMPARE(webView->property("grabCount").toInt(), grabs + 1);
    QVERIFY(grid->property("visible").toBool());
    QCOMPARE(offset(), qreal(0));

    // Up a pixel at a time until the drag is caught: nothing moves before it, and
    // nothing leaps as it is.
    int y = onBar;
    while (!gesture->property("dragging").toBool()) {
        QVERIFY(onBar - y <= shake);
        QCOMPARE(offset(), qreal(0));
        QTest::mouseMove(&window, QPoint(x, --y));
    }
    QCOMPARE(offset(), qreal(0));
    QVERIFY(page->property("dragging").toBool());
    // The drag that started the grab does not take another.
    QCOMPARE(webView->property("grabCount").toInt(), grabs + 1);

    // From there the deck goes where the finger goes.
    const int caught = y;
    for (int step = 1; step <= 10; ++step) {
        QTest::mouseMove(&window, QPoint(x, caught - step * 4));
        QCOMPARE(offset(), qreal(step * 4));
    }
    // Let go short of the threshold, measured from the same place, and it springs back.
    QVERIFY(40 < threshold);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(x, caught - 40));
    QVERIFY(!page->property("tabsOpen").toBool());
    QTRY_COMPARE(offset(), qreal(0));
    QVERIFY(!grid->property("visible").toBool());

    // A press that never drags puts the grid away again: a tap in the reach, which is
    // the page's.
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(x, inReach));
    QVERIFY(grid->property("visible").toBool());
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(x, inReach));
    QVERIFY(!grid->property("visible").toBool());
    // And so does one that turns out to be the page's drag.
    drag(&window, QPoint(x, inReach), QPoint(x + 10 * shake, inReach));
    QVERIFY(!grid->property("visible").toBool());

    // Past the threshold from where it was caught, the grid comes up.
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(x, onBar));
    y = onBar;
    while (!gesture->property("dragging").toBool()) {
        QTest::mouseMove(&window, QPoint(x, --y));
    }
    QTest::mouseMove(&window, QPoint(x, y - int(threshold) - 1));
    QCOMPARE(offset(), threshold + 1);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(x, y - int(threshold) - 1));
    QVERIFY(page->property("tabsOpen").toBool());
    QTRY_COMPARE(offset(), page->property("fullHeight").toReal());
    QVERIFY(grid->property("visible").toBool());
}

// Issue #27, with a page scrolled and the bar slim, as it is most of the time a page is
// read: a drag made the bar whole as it began, so the bar grew under the finger for the
// first fifth of a second and the engine's view was resized mid-drag -- its page laid
// out again -- and once more as the deck sprang back. The bar stays as it is.
void tst_qmlload::barStaysSlimWhileDragged()
{
    QObject *page = find(QStringLiteral("browserPage"));
    QObject *bar = find(QStringLiteral("navigationBar"));
    QObject *viewArea = find(QStringLiteral("viewArea"));
    QObject *webView = currentWebView();
    const qreal slimBar = bar->property("slimHeight").toReal();
    webView->setProperty("chrome", false);
    QTRY_COMPARE(bar->property("height").toReal(), slimBar);
    QTRY_VERIFY(!bar->property("resizing").toBool());
    const qreal viewHeight = viewArea->property("height").toReal();

    evaluate(bar, QStringLiteral("dragArmed()"));
    evaluate(bar, QStringLiteral("dragStarted()"));
    evaluate(bar, QStringLiteral("dragMoved(10)"));
    QVERIFY(page->property("dragging").toBool());
    QVERIFY(page->property("barCompact").toBool());
    QVERIFY(bar->property("compact").toBool());
    QVERIFY(!bar->property("resizing").toBool());
    QCOMPARE(bar->property("height").toReal(), slimBar);
    QCOMPARE(viewArea->property("height").toReal(), viewHeight);

    evaluate(bar, QStringLiteral("dragFinished(10)"));
    QVERIFY(!page->property("dragging").toBool());
    QVERIFY(bar->property("compact").toBool());
    QVERIFY(!bar->property("resizing").toBool());
    QCOMPARE(viewArea->property("height").toReal(), viewHeight);
    webView->setProperty("chrome", true);
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
    // The same sheet as the menu: its opaque ground, its handle at the top, and a
    // heading as Silica heads a section.
    // The menu's, the link sheet's, this one's, and the two banners' on the bar.
    QList<QObject *> grounds = findAll(QStringLiteral("sheetBackground"));
    QCOMPARE(grounds.count(), 5);
    QObject *ground = findObjects(panel, QStringLiteral("sheetBackground")).first();
    QCOMPARE(ground->property("color").value<QColor>(),
             findObjects(find(QStringLiteral("browserMenu")), QStringLiteral("sheetBackground"))
                 .first()
                 ->property("color")
                 .value<QColor>());
    QCOMPARE(ground->property("color").value<QColor>().alphaF(), 1.0);
    QCOMPARE(ground->property("height").toReal(), panel->property("height").toReal());
    auto *handle =
        qobject_cast<QQuickItem *>(findObjects(panel, QStringLiteral("panelDragHandle")).first());
    auto *title = qobject_cast<QQuickItem *>(
        findObjects(panel, QStringLiteral("recentlyClosedTitle")).first());
    QCOMPARE(title->property("text").toString(), QStringLiteral("Recently closed"));
    QVERIFY(title->inherits("QQuickItem"));
    QVERIFY(QString::fromLatin1(title->metaObject()->className())
                .startsWith(QLatin1String("SectionHeader")));
    QVERIFY(handle->mapToScene(QPointF()).y() < title->mapToScene(QPointF()).y());
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
    // Five, as in Jolla's browser; three here, so that four tabs are past it.
    QCOMPARE(tabs->liveTabLimit(), int(TabModel::LiveTabLimit));
    QCOMPARE(tabs->liveTabLimit(), 5);
    tabs->setLiveTabLimit(3);

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
    QCOMPARE(currentWebView()->property("url").toUrl().toString(), QLatin1String(FirstPage));
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

    // A new tab is the plus at the grid's foot, which opens it on the start page, where
    // there is no page for the menu's page row to act on.
    QMetaObject::invokeMethod(menu, "hide");
    pullUpToTabs();
    click(find(QStringLiteral("newTabButton")));
    QCOMPARE(tabs->count(), 2);
    QVERIFY(tabs->activeUrl().isEmpty());
    QVERIFY(find(QStringLiteral("startPageLayer"))->property("active").toBool());
    QVERIFY(!find(QStringLiteral("browserPage"))->property("tabsOpen").toBool());
    QVERIFY(!menu->property("open").toBool());
    tapBar(QStringLiteral("menu"));
    for (const QString &entry :
         {QStringLiteral("findMenuButton"), QStringLiteral("bookmarkMenuButton"),
          QStringLiteral("shareMenuButton"), QStringLiteral("desktopMenuButton"),
          QStringLiteral("readerMenuButton")}) {
        QVERIFY2(!find(entry)->property("enabled").toBool(), qPrintable(entry));
    }
    QVERIFY(find(QStringLiteral("settingsMenuButton"))->property("enabled").toBool());
    evaluate(menu, QStringLiteral("hide()"));
    typeAddress(QStringLiteral("two.example"));
    QCOMPARE(tabs->activeUrl(), QStringLiteral("https://two.example"));

    // Bookmarking is a switch, lit while it is on (menuSheetLayout()), and named alike
    // either way.
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
    QCOMPARE(bookmark->property("text").toString(), QStringLiteral("Bookmark"));
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
    // a tab opened while it is up, say.
    tapBar(QStringLiteral("menu"));
    tabs->newTab(QStringLiteral("https://three.example/"));
    QVERIFY(!menu->property("open").toBool());
}

// The sheet as the stubs can show it: the page's five actions in one row on discs, a
// line, the browser's four without; an opaque ground; and a switch that is on, or an
// entry under a finger, lit -- disc, icon and name -- with no wash across it.
void tst_qmlload::menuSheetLayout()
{
    QObject *menu = find(QStringLiteral("browserMenu"));
    tapBar(QStringLiteral("menu"));
    QVERIFY(menu->property("open").toBool());
    // All five of the page's actions in one row, a fifth of the sheet each, every one
    // on a disc; the browser's four in the row under a line, a quarter each, with none.
    // Each is named under its icon.
    const QStringList pageActions{
        QStringLiteral("findMenuButton"), QStringLiteral("bookmarkMenuButton"),
        QStringLiteral("shareMenuButton"), QStringLiteral("desktopMenuButton"),
        QStringLiteral("readerMenuButton")};
    const QStringList names{QStringLiteral("Find in page"), QStringLiteral("Bookmark"),
                            QStringLiteral("Share"), QStringLiteral("Desktop site"),
                            QStringLiteral("Reader view")};
    auto *sheetItem = qobject_cast<QQuickItem *>(menu);
    const qreal rowY =
        qobject_cast<QQuickItem *>(find(pageActions.first()))->mapToScene(QPointF()).y();
    for (int i = 0; i < pageActions.count(); ++i) {
        auto *button = qobject_cast<QQuickItem *>(find(pageActions.at(i)));
        QCOMPARE(button->mapToScene(QPointF()).y(), rowY);
        QCOMPARE(button->width(), sheetItem->width() / 5);
        QCOMPARE(button->property("text").toString(), names.at(i));
        QVERIFY(button->property("round").toBool());
        QVERIFY(findObjects(button, QStringLiteral("menuButtonDisc"))
                    .first()
                    ->property("visible")
                    .toBool());
    }
    for (const QString &entry :
         {QStringLiteral("bookmarksMenuButton"), QStringLiteral("historyMenuButton"),
          QStringLiteral("downloadsMenuButton"), QStringLiteral("settingsMenuButton")}) {
        auto *button = qobject_cast<QQuickItem *>(find(entry));
        QVERIFY(button->mapToScene(QPointF()).y() > rowY);
        QCOMPARE(button->width(), sheetItem->width() / 4);
        QVERIFY(!findObjects(button, QStringLiteral("menuButtonDisc"))
                     .first()
                     ->property("visible")
                     .toBool());
    }
    // The line between the two rows, and no headings over them: the head of the sheet
    // names the page instead.
    auto *line = qobject_cast<QQuickItem *>(find(QStringLiteral("menuSeparator")));
    QVERIFY(line != nullptr);
    const qreal lineY = line->mapToScene(QPointF()).y();
    QVERIFY(lineY > rowY + find(QStringLiteral("findMenuButton"))->property("height").toReal() - 1);
    QVERIFY(lineY < qobject_cast<QQuickItem *>(find(QStringLiteral("historyMenuButton")))
                        ->mapToScene(QPointF())
                        .y());
    // Two of Silica's lines end to end, each fading away from the middle.
    const QList<QQuickItem *> halves = line->childItems();
    QCOMPARE(halves.count(), 2);
    QCOMPARE(halves.at(0)->rotation(), 180.0);
    QCOMPARE(halves.at(1)->rotation(), 0.0);
    // The sheet is opaque: its ground is the tint of the grid's rows, whole.
    QObject *ground = findObjects(menu, QStringLiteral("sheetBackground")).first();
    QCOMPARE(ground->property("color").value<QColor>().alphaF(), 1.0);
    QCOMPARE(ground->property("color").value<QColor>(),
             evaluate(menu, QStringLiteral("Theme.highlightDimmerColor")).value<QColor>());
    QCOMPARE(ground->property("width").toReal(), sheetItem->width());
    QCOMPARE(ground->property("height").toReal(), sheetItem->height());
    QVERIFY(findObjects(menu, QStringLiteral("menuDragHandle")).count() == 1);

    // How much of an entry is lit: its disc, its icon, its name -- all three or none.
    const auto litParts = [this](QObject *button) {
        const QColor wash =
            evaluate(button, QStringLiteral("Theme.rgba(Theme.highlightBackgroundColor,"
                                            " Theme.highlightBackgroundOpacity)"))
                .value<QColor>();
        const QColor highlight =
            evaluate(button, QStringLiteral("Theme.highlightColor")).value<QColor>();
        const auto part = [button](const char *name) {
            return findObjects(button, QLatin1String(name)).first();
        };
        return int(part("menuButtonDisc")->property("color").value<QColor>() == wash) +
               int(part("menuButtonIcon")->property("highlighted").toBool()) +
               int(part("menuButtonLabel")->property("color").value<QColor>() == highlight);
    };
    QObject *bookmark = find(QStringLiteral("bookmarkMenuButton"));
    QObject *desktop = find(QStringLiteral("desktopMenuButton"));
    QObject *share = find(QStringLiteral("shareMenuButton"));
    QCOMPARE(litParts(bookmark), 0);
    QCOMPARE(litParts(desktop), 0);
    m_core->bookmarks()->add(m_core->tabs()->activeUrl(), m_core->tabs()->activeTitle(), QString());
    QVERIFY(bookmark->property("checked").toBool());
    QCOMPARE(litParts(bookmark), 3);
    currentWebView()->setProperty("desktopMode", true);
    QVERIFY(desktop->property("checked").toBool());
    QCOMPARE(litParts(desktop), 3);
    QCOMPARE(litParts(share), 0);
    QCOMPARE(share->property("highlightedColor").value<QColor>().alpha(), 0);
    share->setProperty("down", true);
    QCOMPARE(litParts(share), 3);
}

// The head of the sheet names the page its actions are for -- its icon, its title, and
// under that a padlock for https and the host -- and copies its address. On the start
// page there is no page: the head says so, and the page's actions are dimmed.
void tst_qmlload::menuNamesThePage()
{
    TabModel *tabs = m_core->tabs();
    const int front = tabs->activeTabId();
    const QString title = QStringLiteral("Qwant, the search engine that respects your privacy");
    tabs->updateTitle(front, title);
    QObject *menu = find(QStringLiteral("browserMenu"));
    tapBar(QStringLiteral("menu"));
    QVERIFY(menu->property("open").toBool());
    const auto item = [this](const char *name) {
        return qobject_cast<QQuickItem *>(find(QLatin1String(name)));
    };
    const auto shown = [&item](const char *name) { return item(name)->isVisible(); };
    const auto text = [&item](const char *name) { return item(name)->property("text").toString(); };
    const auto sceneY = [](QQuickItem *of) { return of->mapToScene(QPointF()).y(); };

    // Under the handle and over the page's row.
    QQuickItem *header = item("menuHeader");
    QVERIFY(header != nullptr);
    QVERIFY(sceneY(header) > sceneY(item("menuDragHandle")));
    QVERIFY(sceneY(header) + header->height() <= sceneY(item("findMenuButton")));
    QCOMPARE(text("menuPageTitle"), title);
    QVERIFY(
        evaluate(item("menuPageTitle"), QStringLiteral("truncationMode === TruncationMode.Fade"))
            .toBool());
    QVERIFY(
        evaluate(item("menuPageTitle"), QStringLiteral("textFormat === Text.PlainText")).toBool());
    QCOMPARE(text("menuPageHost"), QStringLiteral("qwant.com"));
    QVERIFY(shown("menuPageHost"));
    QVERIFY(shown("menuPageSecurity"));
    QCOMPARE(item("menuPageSecurity")->property("source").toString(),
             QStringLiteral("image://theme/icon-s-outline-secure"));
    QVERIFY(!shown("menuStartPageIcon"));
    // No icon yet: the host's initial on the tile. With one, the icon.
    QVERIFY(!shown("menuPageFavicon"));
    QVERIFY(shown("menuPageInitial"));
    QCOMPARE(text("menuPageInitial"), QStringLiteral("Q"));
    tabs->updateFavicon(front, QStringLiteral("image://theme/qwant-favicon"));
    QCOMPARE(item("menuPageFavicon")->property("source").toString(),
             QStringLiteral("image://theme/qwant-favicon"));
    QTRY_VERIFY(shown("menuPageFavicon"));
    QVERIFY(!shown("menuPageInitial"));
    // A page not yet titled is named by its host.
    tabs->updateTitle(front, QString());
    QCOMPARE(text("menuPageTitle"), QStringLiteral("qwant.com"));
    tabs->updateTitle(front, title);

    // While the engine is unhappy with the connection, the bar's warning in its colour
    // takes the padlock's place.
    auto *security = currentWebView()->property("security").value<QObject *>();
    security->setProperty("allGood", false);
    QVERIFY(menu->property("tlsBroken").toBool());
    QCOMPARE(item("menuPageSecurity")->property("source").toString(),
             QStringLiteral("image://theme/icon-s-filled-warning"));
    QCOMPARE(item("menuPageSecurity")->property("color").value<QColor>(),
             evaluate(menu, QStringLiteral("Theme.errorColor")).value<QColor>());
    security->setProperty("allGood", true);
    QVERIFY(!menu->property("tlsBroken").toBool());

    // The button at the right puts the address on the clipboard, says so for a moment,
    // and puts the sheet away, as every entry does.
    QObject *copy = find(QStringLiteral("copyAddressButton"));
    QVERIFY(shown("copyAddressButton"));
    QCOMPARE(copy->property("icon").value<QObject *>()->property("source").toString(),
             QStringLiteral("image://theme/icon-m-clipboard"));
    QVERIFY(sceneY(item("copyAddressButton")) >= sceneY(header));
    QObject *notice = find(QStringLiteral("addressCopiedNotice"));
    QCOMPARE(notice->property("shownCount").toInt(), 0);
    click(copy);
    QCOMPARE(evaluate(menu, QStringLiteral("Clipboard.text")).toString(), QLatin1String(FirstPage));
    QCOMPARE(notice->property("shownCount").toInt(), 1);
    QCOMPARE(notice->property("shownText").toString(), QStringLiteral("Address copied"));
    QCOMPARE(notice->property("duration").toInt(),
             evaluate(notice, QStringLiteral("Notice.Short")).toInt());
    QVERIFY(!menu->property("open").toBool());

    // A page over plain http has no padlock.
    typeAddress(QStringLiteral("http://plain.example/"));
    tapBar(QStringLiteral("menu"));
    QCOMPARE(text("menuPageHost"), QStringLiteral("plain.example"));
    QVERIFY(!shown("menuPageSecurity"));
    evaluate(menu, QStringLiteral("hide()"));

    // The start page: named as such beside the theme's home, nothing to copy, and the
    // page's five actions dimmed as a disabled Silica control is, doing nothing.
    tabs->newTab(QString());
    QVERIFY(tabs->activeUrl().isEmpty());
    tapBar(QStringLiteral("menu"));
    QVERIFY(menu->property("open").toBool());
    QCOMPARE(text("menuPageTitle"), QStringLiteral("Start page"));
    QVERIFY(shown("menuStartPageIcon"));
    QCOMPARE(item("menuStartPageIcon")->property("source").toString(),
             QStringLiteral("image://theme/icon-m-home"));
    QVERIFY(!shown("menuPageFavicon"));
    QVERIFY(!shown("menuPageInitial"));
    QVERIFY(!shown("menuPageHost"));
    QVERIFY(!shown("menuPageSecurity"));
    QVERIFY(!shown("copyAddressButton"));
    const qreal dimmed = evaluate(menu, QStringLiteral("Theme.opacityLow")).toReal();
    for (const char *entry : {"findMenuButton", "bookmarkMenuButton", "shareMenuButton",
                              "desktopMenuButton", "readerMenuButton"}) {
        QVERIFY2(!item(entry)->isEnabled(), entry);
        QCOMPARE(item(entry)->opacity(), dimmed);
    }
    // The browser's own are there as ever.
    for (const char *entry : {"bookmarksMenuButton", "historyMenuButton", "downloadsMenuButton",
                              "settingsMenuButton"}) {
        QVERIFY2(item(entry)->isEnabled(), entry);
        QCOMPARE(item(entry)->opacity(), 1.0);
    }
}

// Downloads wears a ring round its icon while anything is coming, filled as far as the
// downloads under way have gone together, and none once they are all there.
void tst_qmlload::menuShowsDownloadsComing()
{
    // Something of BrowserPage.qml's own, whose scope has the engine.
    QObject *scope = find(QStringLiteral("viewArea"));
    const auto send = [this, scope](const QString &message) {
        evaluate(scope, QStringLiteral("WebEngine.recvObserve('embed:download', %1)").arg(message));
    };
    tapBar(QStringLiteral("menu"));
    QObject *downloads = find(QStringLiteral("downloadsMenuButton"));
    auto *ring = qobject_cast<QQuickItem *>(
        findObjects(downloads, QStringLiteral("menuButtonProgress")).first());
    QVERIFY(!downloads->property("busy").toBool());
    QVERIFY(!ring->isVisible());
    // Round the icon, in the highlight colour over a faint track, and thinner than
    // Silica draws its own.
    auto *icon = qobject_cast<QQuickItem *>(
        findObjects(downloads, QStringLiteral("menuButtonIcon")).first());
    QCOMPARE(ring->mapToScene(QPointF(ring->width() / 2, ring->height() / 2)),
             icon->mapToScene(QPointF(icon->width() / 2, icon->height() / 2)));
    QVERIFY(ring->width() > icon->width());
    QCOMPARE(ring->property("progressColor").value<QColor>(),
             evaluate(downloads, QStringLiteral("Theme.highlightColor")).value<QColor>());
    QVERIFY(ring->property("borderWidth").toReal() <
            evaluate(downloads, QStringLiteral("Theme.paddingSmall")).toReal());

    send(QStringLiteral(
        "{msg: 'dl-start', id: 1, displayName: 'a.pdf', sourceUrl: 'https://files.example/a.pdf',"
        " targetPath: '/tmp/a.pdf', mimeType: 'application/pdf', size: 2048}"));
    QVERIFY(downloads->property("busy").toBool());
    QVERIFY(ring->isVisible());
    QCOMPARE(ring->property("value").toReal(), 0.0);
    send(QStringLiteral("{msg: 'dl-progress', id: 1, percent: 60}"));
    QCOMPARE(ring->property("value").toReal(), 0.6);
    // Two coming: the ring says how far they are together.
    send(QStringLiteral(
        "{msg: 'dl-start', id: 2, displayName: 'b.iso', sourceUrl: 'https://files.example/b.iso',"
        " targetPath: '/tmp/b.iso', mimeType: '', size: 0}"));
    send(QStringLiteral("{msg: 'dl-progress', id: 2, percent: 20}"));
    QCOMPARE(ring->property("value").toReal(), 0.4);
    send(QStringLiteral("{msg: 'dl-done', id: 1, targetPath: '/tmp/a.pdf'}"));
    QCOMPARE(ring->property("value").toReal(), 0.2);
    QVERIFY(ring->isVisible());
    send(QStringLiteral("{msg: 'dl-fail', id: 2}"));
    QVERIFY(!downloads->property("busy").toBool());
    QVERIFY(!ring->isVisible());
    // The other entries have no ring.
    for (const char *entry : {"findMenuButton", "historyMenuButton", "settingsMenuButton"}) {
        QVERIFY(!findObjects(find(QLatin1String(entry)), QStringLiteral("menuButtonProgress"))
                     .first()
                     ->property("visible")
                     .toBool());
    }
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

    // Short of the distance: the sheet goes down with the finger, never further, and
    // further than the flickable alone draws a pull -- half the finger's way past the
    // way a drag takes to start -- and the icon with it. It comes back up when the
    // finger lifts. Not the whole of the finger's way: the flickable reads the finger
    // where it is on the flickable, and the flickable goes down with the sheet under
    // it, so the sheet goes about two thirds of it (docs/DECISIONS/0021-menu-sheet.md).
    // The bound was the finger's way less two drag distances, which the first sheet,
    // short enough for its pull to be short, met by that margin alone.
    const int shortPull = int(closeDistance / 2);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, grab);
    pullTo(shortPull);
    const qreal travelled = menu->y() - openY;
    QVERIFY(travelled <= shortPull);
    QVERIFY2(travelled > (shortPull - slack) / 2.0, qPrintable(QString::number(travelled)));
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

// Find in page: a field over the navigation bar, whose search and steps are the
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

    // The settings restyle it where it is, in the ambience's own look until another is
    // chosen, set in the ambience the view hands the reader: the stub's is a dark one,
    // and its colours are the Theme's.
    const QVariantMap ambience = reader->property("ambience").toMap();
    QCOMPARE(ambience.value(QStringLiteral("highlightColor")).value<QColor>(),
             evaluate(reader, QStringLiteral("Theme.highlightColor")).value<QColor>());
    QVERIFY(html.contains(QLatin1String("class=\"ambience ambience-dark sans-serif\"")));
    QCOMPARE(runs(engine->styleScript(ambience)), 0);
    m_core->readerSettings()->setColors(ReaderSettings::Sepia);
    QCOMPARE(runs(engine->styleScript(ambience)), 1);
    QVERIFY(engine->styleScript(ambience).contains(QLatin1String("'sepia sans-serif'")));
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
    m_core->readerSettings()->setColors(ReaderSettings::Dark);
    QCOMPARE(runs(engine->styleScript(ambience)), 0);

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
    const QString folder = downloads->directory();
    const QString report = folder + QStringLiteral("/report.pdf");
    // Something of BrowserPage.qml's own, whose scope has the engine.
    QObject *scope = find(QStringLiteral("viewArea"));
    QVERIFY(evaluate(scope, QStringLiteral("WebEngine.observers"))
                .toStringList()
                .contains(downloads->topic()));
    const auto engineSays = [this, scope](const QString &message) {
        evaluate(scope, QStringLiteral("WebEngine.recvObserve('embed:download', %1)").arg(message));
    };
    // What the page last told the engine, as the engine would read it.
    const auto engineTold = [this, scope]() {
        const QVariantList told =
            evaluate(scope, QStringLiteral("WebEngine.notifications")).toList();
        if (told.isEmpty()) {
            return QVariantMap();
        }
        QVariantMap last = told.last().toMap();
        return QVariantMap{
            {QStringLiteral("topic"), last.value(QStringLiteral("topic"))},
            {QStringLiteral("msg"),
             last.value(QStringLiteral("value")).toMap().value(QStringLiteral("msg"))},
            {QStringLiteral("id"),
             last.value(QStringLiteral("value")).toMap().value(QStringLiteral("id"))}};
    };
    const auto told = [](const QString &msg, int id) {
        return QVariantMap{{QStringLiteral("topic"), QStringLiteral("embedui:download")},
                           {QStringLiteral("msg"), msg},
                           {QStringLiteral("id"), id}};
    };
    engineSays(QStringLiteral("{msg: 'dl-start', id: 1, displayName: 'report.pdf',"
                              " sourceUrl: 'https://files.example/report.pdf',"
                              " targetPath: '%1', mimeType: 'application/pdf', size: 2048}")
                   .arg(report));
    engineSays(QStringLiteral("{msg: 'dl-progress', id: 1, percent: 40}"));
    QCOMPARE(downloads->count(), 1);

    QObject *page = openMenuItem(QStringLiteral("downloadsMenuButton"));
    QCOMPARE(page->objectName(), QStringLiteral("downloadsPage"));
    QList<QObject *> rows = findAll(QStringLiteral("downloadDelegate"));
    QCOMPARE(rows.count(), 1);
    const auto part = [](QObject *row, const char *name) {
        return findObjects(row, QLatin1String(name)).first();
    };
    const auto text = [&part](QObject *row, const char *name) {
        return part(row, name)->property("text").toString();
    };
    const auto shows = [&part](QObject *row, const char *name) {
        return part(row, name)->property("visible").toBool();
    };
    const auto icon = [&part](QObject *row) {
        return part(row, "downloadAction")
            ->property("icon")
            .value<QObject *>()
            ->property("source")
            .toUrl();
    };
    const auto theme = [this, scope](const char *name) {
        return evaluate(scope, QStringLiteral("Theme.") + QLatin1String(name)).value<QColor>();
    };

    // Coming: how much of how much, a ring as far along as it is, and a pause in it.
    QCOMPARE(text(rows.first(), "downloadName"), QStringLiteral("report.pdf"));
    QCOMPARE(text(rows.first(), "downloadStatus"), QStringLiteral("819 B of 2.0 kB · 40%"));
    QVERIFY(shows(rows.first(), "downloadProgress"));
    QCOMPARE(part(rows.first(), "downloadProgress")->property("value").toReal(), 0.4);
    QCOMPARE(part(rows.first(), "downloadProgress")->property("progressColor").value<QColor>(),
             theme("highlightColor"));
    QVERIFY(shows(rows.first(), "downloadAction"));
    QVERIFY(!shows(rows.first(), "downloadFileIcon"));
    QCOMPARE(icon(rows.first()), QUrl(QStringLiteral("image://theme/icon-m-pause")));
    QVERIFY(shows(rows.first(), "pauseDownloadMenu"));
    QVERIFY(!shows(rows.first(), "resumeDownloadMenu"));
    QVERIFY(!shows(rows.first(), "openDownloadMenu"));
    QVERIFY(!shows(rows.first(), "deleteDownloadMenu"));
    // The menu holds these and nothing else: no folder to open, which the platform will
    // not open from inside Sailjail, and no link to copy.
    const auto entries = [](const QObject *menu) {
        QStringList names;
        for (const QObject *child : menu->children()) {
            if (child->inherits("QQuickItem")) {
                names.append(child->objectName());
            }
        }
        return names;
    };
    QCOMPARE(
        entries(rows.first()->property("menu").value<QObject *>()),
        QStringList({QStringLiteral("openDownloadMenu"), QStringLiteral("pauseDownloadMenu"),
                     QStringLiteral("resumeDownloadMenu"), QStringLiteral("deleteDownloadMenu"),
                     QStringLiteral("removeDownloadMenu")}));

    // Not yet there, a tap opens nothing.
    const UrlCatcher files(QStringLiteral("file"));
    click(rows.first());
    QVERIFY(files.opened.isEmpty());

    // The ring pauses it, by telling the engine; the row says so when the engine does.
    click(part(rows.first(), "downloadAction"));
    QCOMPARE(engineTold(), told(QStringLiteral("cancelDownload"), 1));
    QCOMPARE(text(rows.first(), "downloadStatus"), QStringLiteral("819 B of 2.0 kB · 40%"));
    engineSays(QStringLiteral("{msg: 'dl-cancel', id: 1}"));
    QCOMPARE(text(rows.first(), "downloadStatus"), QStringLiteral("Paused · 40%"));
    QCOMPARE(icon(rows.first()), QUrl(QStringLiteral("image://theme/icon-m-play")));
    QCOMPARE(part(rows.first(), "downloadProgress")->property("progressColor").value<QColor>(),
             theme("secondaryHighlightColor"));
    QVERIFY(!shows(rows.first(), "pauseDownloadMenu"));
    QVERIFY(shows(rows.first(), "resumeDownloadMenu"));
    QCOMPARE(text(rows.first(), "resumeDownloadMenu"), QStringLiteral("Resume"));
    // Resumed from its menu, as from the ring.
    click(part(rows.first(), "resumeDownloadMenu"));
    QCOMPARE(engineTold(), told(QStringLiteral("retryDownload"), 1));
    engineSays(QStringLiteral("{msg: 'dl-start', id: 1, displayName: 'report.pdf',"
                              " sourceUrl: 'https://files.example/report.pdf',"
                              " targetPath: '%1', mimeType: 'application/pdf', size: 2048}")
                   .arg(report));
    QCOMPARE(text(rows.first(), "downloadStatus"), QStringLiteral("819 B of 2.0 kB · 40%"));
    click(part(rows.first(), "pauseDownloadMenu"));
    QCOMPARE(engineTold(), told(QStringLiteral("cancelDownload"), 1));

    // Arrived, it says how big it is and where it came from, wears its kind's icon, and
    // a tap opens the file.
    QFile file(report);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();
    engineSays(QStringLiteral("{msg: 'dl-done', id: 1, targetPath: '%1'}").arg(report));
    QCOMPARE(text(rows.first(), "downloadStatus"), QStringLiteral("2.0 kB · files.example"));
    QVERIFY(!shows(rows.first(), "downloadProgress"));
    QVERIFY(!shows(rows.first(), "downloadAction"));
    QVERIFY(shows(rows.first(), "downloadFileIcon"));
    QCOMPARE(part(rows.first(), "downloadFileIcon")->property("source").toUrl(),
             QUrl(QStringLiteral("image://theme/icon-m-file-pdf")));
    click(rows.first());
    QCOMPARE(files.opened, QList<QUrl>{QUrl::fromLocalFile(report)});
    QCOMPARE(currentPage(), page);
    // Its menu opens it too.
    QVERIFY(!shows(rows.first(), "pauseDownloadMenu"));
    QVERIFY(!shows(rows.first(), "resumeDownloadMenu"));
    click(part(rows.first(), "openDownloadMenu"));
    QCOMPARE(files.opened.count(), 2);

    // One that failed says so in the error colour, newest first; a tap on it opens
    // nothing, and its ring tries it again.
    engineSays(QStringLiteral("{msg: 'dl-start', id: 2, displayName: 'big.iso',"
                              " sourceUrl: 'https://x.example/', targetPath: '/tmp/big.iso',"
                              " mimeType: '', size: 0}"));
    engineSays(QStringLiteral("{msg: 'dl-progress', id: 2, percent: 15}"));
    QCOMPARE(text(byRow(findAll(QStringLiteral("downloadDelegate"))).first(), "downloadStatus"),
             QStringLiteral("Downloading, 15%"));
    engineSays(QStringLiteral("{msg: 'dl-fail', id: 2}"));
    rows = byRow(findAll(QStringLiteral("downloadDelegate")));
    QCOMPARE(rows.count(), 2);
    QCOMPARE(text(rows.first(), "downloadName"), QStringLiteral("big.iso"));
    QCOMPARE(text(rows.first(), "downloadStatus"), QStringLiteral("Failed"));
    QCOMPARE(part(rows.first(), "downloadStatus")->property("color").value<QColor>(),
             theme("errorColor"));
    QCOMPARE(part(rows.first(), "downloadProgress")->property("progressColor").value<QColor>(),
             theme("errorColor"));
    QCOMPARE(icon(rows.first()), QUrl(QStringLiteral("image://theme/icon-m-refresh")));
    QCOMPARE(text(rows.first(), "resumeDownloadMenu"), QStringLiteral("Retry"));
    click(rows.first());
    QCOMPARE(files.opened.count(), 2);
    QCOMPARE(currentPage(), page);
    click(part(rows.first(), "downloadAction"));
    QCOMPARE(engineTold(), told(QStringLiteral("retryDownload"), 2));

    // Forgotten from its menu; the file is not the list's to delete.
    click(part(rows.first(), "removeDownloadMenu"));
    QCOMPARE(downloads->count(), 1);

    // Deleted from its menu, after the remorse timer, the file goes with its row.
    rows = findAll(QStringLiteral("downloadDelegate"));
    QCOMPARE(rows.first()->property("remorseCount").toInt(), 0);
    click(part(rows.first(), "deleteDownloadMenu"));
    QCOMPARE(rows.first()->property("remorseCount").toInt(), 1);
    QVERIFY(!QFileInfo::exists(report));
    QCOMPARE(downloads->count(), 0);

    // A file deleted elsewhere is found missing when the page comes back: it cannot be
    // opened or deleted, and says so.
    engineSays(QStringLiteral("{msg: 'dl-start', id: 3, displayName: 'photo.jpg',"
                              " sourceUrl: 'https://files.example/photo.jpg',"
                              " targetPath: '%1/photo.jpg', mimeType: 'image/jpeg', size: 0}")
                   .arg(folder));
    QFile photo(folder + QStringLiteral("/photo.jpg"));
    QVERIFY(photo.open(QIODevice::WriteOnly));
    photo.close();
    engineSays(QStringLiteral("{msg: 'dl-done', id: 3}"));
    rows = findAll(QStringLiteral("downloadDelegate"));
    QCOMPARE(text(rows.first(), "downloadStatus"), QStringLiteral("files.example"));
    QCOMPARE(part(rows.first(), "downloadFileIcon")->property("source").toUrl(),
             QUrl(QStringLiteral("image://theme/icon-m-file-image")));
    QVERIFY(photo.remove());
    page->setProperty("status", evaluate(scope, QStringLiteral("PageStatus.Activating")));
    QCOMPARE(text(rows.first(), "downloadStatus"), QStringLiteral("File not found"));
    QVERIFY(!shows(rows.first(), "openDownloadMenu"));
    QVERIFY(!shows(rows.first(), "deleteDownloadMenu"));
    QCOMPARE(part(rows.first(), "downloadFileIcon")->property("opacity").toReal(),
             evaluate(scope, QStringLiteral("Theme.opacityLow")).toReal());
    const int opened = files.opened.count();
    click(rows.first());
    QCOMPARE(files.opened.count(), opened);

    // The pulley has one entry: it forgets the finished ones, and leaves the rest.
    QCOMPARE(entries(find(QStringLiteral("downloadsPulley"))),
             QStringList{QStringLiteral("clearFinishedDownloadsMenu")});
    engineSays(QStringLiteral("{msg: 'dl-start', id: 4, displayName: 'later.zip',"
                              " sourceUrl: 'https://files.example/later.zip',"
                              " targetPath: '%1/later.zip', mimeType: 'application/zip', size: 0}")
                   .arg(folder));
    QCOMPARE(downloads->count(), 2);
    click(find(QStringLiteral("clearFinishedDownloadsMenu")));
    QCOMPARE(downloads->count(), 1);
    QCOMPARE(text(findAll(QStringLiteral("downloadDelegate")).first(), "downloadName"),
             QStringLiteral("later.zip"));
    // Removed from its menu while still coming, it is paused rather than left to arrive
    // unlisted.
    click(part(findAll(QStringLiteral("downloadDelegate")).first(), "removeDownloadMenu"));
    QCOMPARE(engineTold(), told(QStringLiteral("cancelDownload"), 4));
    QCOMPARE(downloads->count(), 0);
    QCOMPARE(findAll(QStringLiteral("downloadDelegate")).count(), 0);
    QVERIFY(!find(QStringLiteral("clearFinishedDownloadsMenu"))->property("enabled").toBool());
}

// One of an earlier run is fetched again from where it came from, and the engine's start
// for it is the row that takes its place.
void tst_qmlload::downloadAgain()
{
    cleanup();
    m_dir.reset(new QTemporaryDir);
    {
        Core core(m_dir->path(), m_dir->path() + QStringLiteral("/salama.conf"),
                  m_dir->path() + QStringLiteral("/Downloads/Salama"));
        core.downloads()->observe(
            core.downloads()->topic(),
            QVariantMap{
                {QStringLiteral("msg"), QStringLiteral("dl-start")},
                {QStringLiteral("id"), 1},
                {QStringLiteral("displayName"), QStringLiteral("a.zip")},
                {QStringLiteral("sourceUrl"), QStringLiteral("https://files.example/a.zip")},
                {QStringLiteral("targetPath"), QStringLiteral("/tmp/a.zip")}});
        // And one paused, which the engine will have forgotten as much.
        core.downloads()->observe(
            core.downloads()->topic(),
            QVariantMap{
                {QStringLiteral("msg"), QStringLiteral("dl-start")},
                {QStringLiteral("id"), 2},
                {QStringLiteral("displayName"), QStringLiteral("b.zip")},
                {QStringLiteral("sourceUrl"), QStringLiteral("https://files.example/b.zip")}});
        core.downloads()->observe(core.downloads()->topic(),
                                  QVariantMap{{QStringLiteral("msg"), QStringLiteral("dl-cancel")},
                                              {QStringLiteral("id"), 2}});
    }
    m_core.reset(new Core(m_dir->path(), m_dir->path() + QStringLiteral("/salama.conf"),
                          m_dir->path() + QStringLiteral("/Downloads/Salama")));
    m_core->tabs()->newTab(QLatin1String(FirstPage));
    m_core->settings()->setTutorialShown(true);
    QVERIFY(loadWindow());
    forgetStartupMessages();

    openMenuItem(QStringLiteral("downloadsMenuButton"));
    // Paused in an earlier run, it can only start over: it says Stopped, not Paused.
    QObject *paused = byRow(findAll(QStringLiteral("downloadDelegate"))).first();
    const auto partOf = [](QObject *row, const char *name) {
        return findObjects(row, QLatin1String(name)).first();
    };
    QCOMPARE(partOf(paused, "downloadStatus")->property("text").toString(),
             QStringLiteral("Stopped"));
    QCOMPARE(partOf(paused, "resumeDownloadMenu")->property("text").toString(),
             QStringLiteral("Download again"));
    QCOMPARE(partOf(paused, "downloadAction")
                 ->property("icon")
                 .value<QObject *>()
                 ->property("source")
                 .toUrl(),
             QUrl(QStringLiteral("image://theme/icon-m-refresh")));
    QObject *row = byRow(findAll(QStringLiteral("downloadDelegate"))).last();
    QCOMPARE(
        findObjects(row, QStringLiteral("downloadStatus")).first()->property("text").toString(),
        QStringLiteral("Failed"));
    QObject *again = findObjects(row, QStringLiteral("resumeDownloadMenu")).first();
    QCOMPARE(again->property("text").toString(), QStringLiteral("Download again"));
    click(again);
    QObject *scope = find(QStringLiteral("viewArea"));
    const QVariantList told = evaluate(scope, QStringLiteral("WebEngine.notifications")).toList();
    QCOMPARE(told.count(), 1);
    QCOMPARE(told.first().toMap().value(QStringLiteral("topic")).toString(),
             QStringLiteral("embedui:download"));
    QCOMPARE(told.first().toMap().value(QStringLiteral("value")).toMap(),
             QVariantMap({{QStringLiteral("msg"), QStringLiteral("addDownload")},
                          {QStringLiteral("from"), QStringLiteral("https://files.example/a.zip")},
                          {QStringLiteral("to"), QStringLiteral("/tmp/a.zip")}}));
    QCOMPARE(m_core->downloads()->count(), 1);
}

// The browsing page says what the downloads are doing above the bar, without anyone
// going to the list (docs/DECISIONS/0038-download-status.md).
void tst_qmlload::downloadBanner()
{
    QObject *scope = find(QStringLiteral("viewArea"));
    QObject *banner = find(QStringLiteral("downloadBanner"));
    QObject *bar = find(QStringLiteral("navigationBar"));
    const auto engineSays = [this, scope](const QString &message) {
        evaluate(scope, QStringLiteral("WebEngine.recvObserve('embed:download', %1)").arg(message));
    };
    const auto part = [banner](const char *name) {
        return findObjects(banner, QLatin1String(name)).first();
    };
    const auto text = [&part](const char *name) { return part(name)->property("text").toString(); };
    QVERIFY(!banner->property("shown").toBool());
    QCOMPARE(banner->property("opacity").toReal(), 0.0);

    // One download: its name and how far along it is.
    engineSays(QStringLiteral("{msg: 'dl-start', id: 1, displayName: 'report.pdf',"
                              " sourceUrl: 'https://files.example/report.pdf',"
                              " targetPath: '/tmp/report.pdf', mimeType: 'application/pdf',"
                              " size: 2048}"));
    engineSays(QStringLiteral("{msg: 'dl-progress', id: 1, percent: 40}"));
    QVERIFY(banner->property("shown").toBool());
    QCOMPARE(text("bannerTitle"), QStringLiteral("report.pdf"));
    QCOMPARE(text("bannerDetail"), QStringLiteral("819 B of 2.0 kB · 40%"));
    QCOMPARE(part("downloadBannerProgress")->property("value").toReal(), 0.4);
    // It fades in, on the bar, wherever the bar is, the banners lying one above the
    // other and this one the lowest.
    QTRY_COMPARE(banner->property("opacity").toReal(), 1.0);
    QObject *banners = find(QStringLiteral("barBanners"));
    QCOMPARE(banners->property("y").toReal() + banners->property("height").toReal(),
             bar->property("y").toReal());
    QCOMPARE(banner->property("y").toReal() + banner->property("height").toReal(),
             banners->property("height").toReal());
    QVERIFY(part("bannerDetail")->property("visible").toBool());
    // The whole width, on the sheets' ground, as the banner for a link opened behind is.
    QCOMPARE(banner->property("width").toReal(), bar->property("width").toReal());
    QCOMPARE(findObjects(banner, QStringLiteral("sheetBackground")).count(), 1);
    // One thing to do at its end, Show, and no button besides: what is done to a download
    // is done in the list.
    QCOMPARE(text("bannerActionLabel"), QStringLiteral("Show"));
    for (QObject *item : findObjects(banner, QString())) {
        QVERIFY2(!QByteArray(item->metaObject()->className()).startsWith("IconButton"),
                 item->metaObject()->className());
    }
    const QColor secondary =
        evaluate(scope, QStringLiteral("Theme.secondaryColor")).value<QColor>();
    const QColor error = evaluate(scope, QStringLiteral("Theme.errorColor")).value<QColor>();
    QCOMPARE(part("bannerDetail")->property("color").value<QColor>(), secondary);
    engineSays(QStringLiteral("{msg: 'dl-cancel', id: 1}"));
    QCOMPARE(text("bannerDetail"), QStringLiteral("Paused · 40%"));
    engineSays(QStringLiteral("{msg: 'dl-fail', id: 1}"));
    QCOMPARE(text("bannerDetail"), QStringLiteral("Failed"));
    QCOMPARE(part("bannerDetail")->property("color").value<QColor>(), error);
    QCOMPARE(part("downloadBannerProgress")->property("progressColor").value<QColor>(), error);
    engineSays(QStringLiteral("{msg: 'dl-start', id: 1}"));

    // More: how many and how far together, and no line under it -- not their names.
    engineSays(QStringLiteral("{msg: 'dl-start', id: 2, displayName: 'b.iso', size: 0}"));
    engineSays(QStringLiteral("{msg: 'dl-progress', id: 2, percent: 20}"));
    QCOMPARE(text("bannerTitle"), QStringLiteral("2 download(s) · 30%"));
    QCOMPARE(text("bannerDetail"), QString());
    QVERIFY(!part("bannerDetail")->property("visible").toBool());
    engineSays(QStringLiteral("{msg: 'dl-start', id: 3, displayName: 'c.zip', size: 0}"));
    engineSays(QStringLiteral("{msg: 'dl-fail', id: 3}"));
    engineSays(QStringLiteral("{msg: 'dl-cancel', id: 2}"));
    QCOMPARE(text("bannerTitle"), QStringLiteral("3 download(s) · 30%"));
    QVERIFY(!part("bannerDetail")->property("visible").toBool());
    QCOMPARE(part("downloadBannerProgress")->property("progressColor").value<QColor>(),
             evaluate(scope, QStringLiteral("Theme.highlightColor")).value<QColor>());

    // Out of the way while the address is edited and while the grid is out.
    tapBar(QStringLiteral("address"));
    QVERIFY(!banner->property("shown").toBool());
    evaluate(bar, QStringLiteral("endEditing()"));
    QVERIFY(banner->property("shown").toBool());
    pullUpToTabs();
    QVERIFY(!banner->property("shown").toBool());
    pullDownToBrowser();
    QVERIFY(banner->property("shown").toBool());

    // A tap opens the list, and so does Show.
    evaluate(banner, QStringLiteral("activate()"));
    QCOMPARE(currentPage()->objectName(), QStringLiteral("downloadsPage"));
    popPage();
    click(part("bannerAction"));
    QCOMPARE(currentPage()->objectName(), QStringLiteral("downloadsPage"));
    popPage();

    // One arriving is said for a moment; a tap then still opens the list, never the file.
    engineSays(QStringLiteral("{msg: 'dl-done', id: 1}"));
    QVERIFY(banner->property("flashing").toBool());
    QCOMPARE(text("bannerTitle"), QStringLiteral("report.pdf"));
    QCOMPARE(text("bannerDetail"), QStringLiteral("Downloaded"));
    QCOMPARE(part("downloadBannerProgress")->property("value").toReal(), 1.0);
    const UrlCatcher files(QStringLiteral("file"));
    evaluate(banner, QStringLiteral("activate()"));
    QVERIFY(files.opened.isEmpty());
    QCOMPARE(currentPage()->objectName(), QStringLiteral("downloadsPage"));
    popPage();
    // Then it goes back to the rest.
    part("downloadBannerFlash")->setProperty("running", false);
    QCOMPARE(text("bannerTitle"), QStringLiteral("2 download(s) · 20%"));

    // Swiped away, it goes, until something changes.
    evaluate(banner, QStringLiteral("dismiss()"));
    QVERIFY(!banner->property("shown").toBool());
    engineSays(QStringLiteral("{msg: 'dl-progress', id: 2, percent: 70}"));
    QVERIFY(!banner->property("shown").toBool());
    engineSays(QStringLiteral("{msg: 'dl-start', id: 2}"));
    QVERIFY(banner->property("shown").toBool());
    QCOMPARE(text("bannerTitle"), QStringLiteral("b.iso"));

    // An arrival swiped away goes at once.
    engineSays(QStringLiteral("{msg: 'dl-done', id: 2}"));
    QVERIFY(banner->property("flashing").toBool());
    evaluate(banner, QStringLiteral("dismiss()"));
    QVERIFY(!banner->property("flashing").toBool());
    QVERIFY(!banner->property("shown").toBool());
}

// The banner under a real finger: dragged sideways far enough it goes, not far enough it
// stays, and a tap opens the list.
void tst_qmlload::downloadBannerUnderAFinger()
{
    QObject *banner = find(QStringLiteral("downloadBanner"));
    evaluate(find(QStringLiteral("viewArea")),
             QStringLiteral("WebEngine.recvObserve('embed:download', {msg: 'dl-start', id: 1,"
                            " displayName: 'a.pdf', size: 0})"));
    auto *root = qobject_cast<QQuickItem *>(m_window.data());
    FingerWindow host(root);
    QQuickWindow &window = *host.window();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QTRY_COMPARE(banner->property("opacity").toReal(), 1.0);
    QObject *card = findObjects(banner, QStringLiteral("bannerBar")).first();
    const QPoint middle = centreOf(card);
    const int width = card->property("width").toInt();
    // Kept clear of Show, at the bar's end.
    const QPoint grip = middle - QPoint(width / 4, 0);

    drag(&window, grip, grip + QPoint(width / 6, 0));
    QVERIFY(banner->property("shown").toBool());
    QCOMPARE(findObjects(banner, QStringLiteral("bannerHandle")).first()->property("x").toReal(),
             0.0);
    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, grip);
    QCOMPARE(currentPage()->objectName(), QStringLiteral("downloadsPage"));
    popPage();
    QCOMPARE(currentPage()->objectName(), QStringLiteral("browserPage"));

    drag(&window, grip, grip + QPoint(width / 2, 0));
    QVERIFY(!banner->property("shown").toBool());
    QCOMPARE(m_core->downloads()->trayCount(), 0);
}

namespace {

// What ContextMenuHandler.js sends for a press held on the page: on a link, a picture, or
// a picture that is a link, by the names the engine's message has
// (docs/DECISIONS/0046-link-menu.md).
QVariantMap heldOn(const QString &link, const QString &title = QString(),
                   const QString &image = QString())
{
    QStringList types;
    if (!image.isEmpty()) {
        types << QStringLiteral("image");
    }
    if (!link.isEmpty()) {
        types << QStringLiteral("link");
    }
    return {
        {QStringLiteral("types"), types},
        {QStringLiteral("linkURL"), link},
        {QStringLiteral("linkTitle"), title},
        {QStringLiteral("linkProtocol"), QUrl(link).scheme()},
        {QStringLiteral("mediaURL"), image},
        {QStringLiteral("contentType"), image.isEmpty() ? QString() : QStringLiteral("image/jpeg")},
        {QStringLiteral("xPos"), 40},
        {QStringLiteral("yPos"), 300}};
}

void holdOn(QObject *view, const QVariantMap &message)
{
    QMetaObject::invokeMethod(view, "recvAsyncMessage",
                              Q_ARG(QString, QStringLiteral("Content:ContextMenu")),
                              Q_ARG(QVariant, QVariant(message)));
}

bool shownIn(QObject *item)
{
    return item != nullptr && item->property("visible").toBool();
}

} // namespace

// A press held on a link or a picture brings the link sheet up, and a press on anything
// else is left to the platform; the platform's own menu for the press shows nothing
// (docs/DECISIONS/0046-link-menu.md).
void tst_qmlload::linkMenuOnALongPress()
{
    ScriptErrors errors;
    TabModel *tabs = m_core->tabs();
    QObject *menu = find(QStringLiteral("linkMenu"));
    QObject *view = currentWebView();
    QVERIFY(menu != nullptr);
    QVERIFY(!menu->property("open").toBool());
    QVERIFY(menu->property("modal").toBool());
    QCOMPARE(menu->property("dock").toInt(), 2); // Dock.Bottom

    // The platform's opener makes its menu from what the view's provider names: a stand-in
    // here, a file that loads, takes what the opener sets on a menu and is never up.
    QCOMPARE(evaluate(view, QStringLiteral("popupProvider.contextMenu.type")).toString(),
             QStringLiteral("item"));
    const QString standIn =
        evaluate(view, QStringLiteral("popupProvider.contextMenu.component")).toString();
    QVERIFY2(standIn.endsWith(QLatin1String("/qml/components/PlatformMenuStandIn.qml")),
             qPrintable(standIn));
    QQmlComponent standInComponent(m_engine.data(), QUrl(standIn));
    QScopedPointer<QObject> madeStandIn(standInComponent.create());
    QVERIFY2(!madeStandIn.isNull(), qPrintable(standInComponent.errorString()));
    for (const char *name : {"linkHref", "linkTitle", "linkProtocol", "imageSrc", "contentType",
                             "viewId", "downloadsEnabled", "pageStack", "tabModel"}) {
        QVERIFY2(madeStandIn->metaObject()->indexOfProperty(name) >= 0, name);
    }
    QVERIFY(QMetaObject::invokeMethod(madeStandIn.data(), "show"));
    QVERIFY(!madeStandIn->property("active").toBool());
    QVERIFY(!madeStandIn->property("visible").toBool());

    // Text, a script dressed as a link, and other messages are not presses on a link.
    holdOn(view, {{QStringLiteral("types"), QStringList{QStringLiteral("content-text")}}});
    QVERIFY(!menu->property("open").toBool());
    holdOn(view, heldOn(QStringLiteral("javascript:void(0)"), QStringLiteral("More")));
    QVERIFY(!menu->property("open").toBool());
    QMetaObject::invokeMethod(
        view, "recvAsyncMessage", Q_ARG(QString, QStringLiteral("embed:find")),
        Q_ARG(QVariant, QVariant(heldOn(QStringLiteral("https://a.example/")))));
    QVERIFY(!menu->property("open").toBool());

    // A link: the sheet, its head naming the link and where it goes.
    holdOn(view, heldOn(QStringLiteral("https://www.trails.example/walks/ridge-loop"),
                        QStringLiteral(" The ridge\n loop ")));
    QVERIFY(menu->property("open").toBool());
    QCOMPARE(menu->property("view").value<QObject *>(), view);
    const auto text = [this](const char *name) {
        return find(QLatin1String(name))->property("text").toString();
    };
    QCOMPARE(text("linkMenuTitle"), QStringLiteral("The ridge loop"));
    QCOMPARE(text("linkMenuAddress"), QStringLiteral("trails.example/walks/ridge-loop"));
    QVERIFY(shownIn(find(QStringLiteral("linkMenuAddress"))));
    QCOMPARE(text("linkMenuInitial"), QStringLiteral("T"));
    QVERIFY(shownIn(find(QStringLiteral("linkMenuInitial"))));
    QVERIFY(!shownIn(find(QStringLiteral("linkMenuAppIcon"))));
    // The menu's sheet: its ground and its handle; and over the page above it, laid on
    // the page rather than carried by the sheet, a dim, under the sheet.
    QCOMPARE(findObjects(menu, QStringLiteral("sheetBackground")).count(), 1);
    QVERIFY(find(QStringLiteral("linkMenuDragHandle")) != nullptr);
    auto *overlay = qobject_cast<QQuickItem *>(find(QStringLiteral("linkMenuOverlay")));
    QCOMPARE(overlay->parentItem(), qobject_cast<QQuickItem *>(menu)->parentItem());
    QVERIFY(overlay->z() < menu->property("z").toReal());
    QVERIFY(overlay->property("shown").toBool());
    QTRY_VERIFY(shownIn(find(QStringLiteral("linkMenuDim"))));
    // The page's actions, on discs, a quarter of the sheet each; no picture's, no other
    // application's.
    QVERIFY(shownIn(find(QStringLiteral("linkPageRow"))));
    QVERIFY(!shownIn(find(QStringLiteral("linkAppRow"))));
    QVERIFY(!shownIn(find(QStringLiteral("linkImageRow"))));
    QVERIFY(!shownIn(
        qobject_cast<QQuickItem *>(find(QStringLiteral("linkMenuSeparator")))->parentItem()));
    const QList<QPair<QString, QString>> actions{
        {QStringLiteral("newTabLinkButton"), QStringLiteral("New tab")},
        {QStringLiteral("backgroundTabLinkButton"), QStringLiteral("Background tab")},
        {QStringLiteral("shareLinkButton"), QStringLiteral("Share")},
        {QStringLiteral("saveLinkButton"), QStringLiteral("Save link")},
    };
    for (const auto &action : actions) {
        QObject *button = find(action.first);
        QVERIFY2(button != nullptr, qPrintable(action.first));
        QCOMPARE(button->property("text").toString(), action.second);
        QVERIFY2(button->property("round").toBool(), qPrintable(action.first));
        QVERIFY2(!button->property("iconSource").toString().isEmpty(), qPrintable(action.first));
        QCOMPARE(qobject_cast<QQuickItem *>(button)->parentItem()->objectName(),
                 QStringLiteral("linkPageRow"));
        QCOMPARE(button->property("width").toReal(), menu->property("width").toReal() / 4);
    }

    // A link without text of its own is named by where it goes, once.
    holdOn(view, heldOn(QStringLiteral("https://trails.example/")));
    QCOMPARE(text("linkMenuTitle"), QStringLiteral("trails.example"));
    QVERIFY(!shownIn(find(QStringLiteral("linkMenuAddress"))));
    evaluate(menu, QStringLiteral("hide()"));
    QVERIFY(!overlay->property("shown").toBool());

    // A press on a page behind the one in front is not for this sheet, and another page
    // in front puts the sheet away.
    tabs->newTab(QStringLiteral("https://two.example/"));
    QObject *front = currentWebView();
    QVERIFY(front != view);
    holdOn(view, heldOn(QStringLiteral("https://trails.example/")));
    QVERIFY(!menu->property("open").toBool());
    holdOn(front, heldOn(QStringLiteral("https://trails.example/")));
    QVERIFY(menu->property("open").toBool());
    QCOMPARE(menu->property("view").value<QObject *>(), front);
    tabs->activateTab(0);
    QVERIFY(!menu->property("open").toBool());
    QCOMPARE(errors.all(), QString());
}

// What the link sheet's actions do for a link to a page: a new tab in front, one behind
// with a banner saying where it went, the share sheet, a download, the clipboard.
void tst_qmlload::linkMenuActions()
{
    ScriptErrors errors;
    TabModel *tabs = m_core->tabs();
    QObject *menu = find(QStringLiteral("linkMenu"));
    QObject *view = currentWebView();
    QObject *scope = find(QStringLiteral("viewArea"));
    const int front = tabs->activeTabId();
    const QString ridge = QStringLiteral("https://trails.example/walks/ridge-loop");
    const QString ridgeTitle = QStringLiteral("The ridge loop");

    // New tab: the link in a tab of its own, in front, after a picture of the page left.
    const int grabs = view->property("grabCount").toInt();
    holdOn(view, heldOn(ridge, ridgeTitle));
    click(find(QStringLiteral("newTabLinkButton")));
    QVERIFY(!menu->property("open").toBool());
    QCOMPARE(tabs->count(), 2);
    QCOMPARE(tabs->activeUrl(), ridge);
    QVERIFY(view->property("grabCount").toInt() > grabs);
    tabs->activateTabById(front);

    // Background tab: the link in a tab behind, named by its text; the page in front stays,
    // and a banner on the bar says where the link went.
    QObject *banner = find(QStringLiteral("tabBanner"));
    const auto bannerText = [banner](const char *name) {
        return findObjects(banner, QLatin1String(name)).first()->property("text").toString();
    };
    QVERIFY(!banner->property("shown").toBool());
    holdOn(view, heldOn(ridge, ridgeTitle));
    click(find(QStringLiteral("backgroundTabLinkButton")));
    QVERIFY(!menu->property("open").toBool());
    QCOMPARE(tabs->count(), 3);
    QCOMPARE(tabs->activeTabId(), front);
    QCOMPARE(currentWebView(), view);
    const Salama::Tab behind = tabs->tabs().last();
    QCOMPARE(behind.url, ridge);
    QCOMPARE(behind.title, ridgeTitle);
    QVERIFY(banner->property("shown").toBool());
    QCOMPARE(bannerText("bannerTitle"), QStringLiteral("Opened in a new tab"));
    QCOMPARE(bannerText("bannerDetail"), ridgeTitle);
    QCOMPARE(bannerText("bannerActionLabel"), QStringLiteral("Show"));
    // Its page is not loaded until it is shown, as a restored tab's is not.
    for (QObject *loader : findAll(QStringLiteral("webViewLoader"))) {
        if (loader->property("tabId").toInt() == behind.id) {
            QVERIFY(!loader->property("active").toBool());
        }
    }
    // Show brings it to the front, and the banner goes.
    click(findObjects(banner, QStringLiteral("bannerAction")).first());
    QCOMPARE(tabs->activeTabId(), behind.id);
    QCOMPARE(currentWebView()->property("url").toString(), ridge);
    QVERIFY(!banner->property("shown").toBool());
    tabs->activateTabById(front);
    // Left alone, it goes after a few seconds; swiped, at once.
    holdOn(view, heldOn(ridge, ridgeTitle));
    click(find(QStringLiteral("backgroundTabLinkButton")));
    QVERIFY(banner->property("shown").toBool());
    findObjects(banner, QStringLiteral("tabBannerTimer")).first()->setProperty("running", false);
    QVERIFY(!banner->property("shown").toBool());
    holdOn(view, heldOn(ridge, ridgeTitle));
    click(find(QStringLiteral("backgroundTabLinkButton")));
    QVERIFY(banner->property("shown").toBool());
    QMetaObject::invokeMethod(banner, "dismissed");
    QVERIFY(!banner->property("shown").toBool());
    // In a group with a name, the name is said too; a link without text is named by
    // where it goes.
    const int reading = tabs->addGroup(QStringLiteral("Reading"));
    tabs->newTab(QStringLiteral("https://trails.example/"));
    QObject *readingView = currentWebView();
    holdOn(readingView, heldOn(ridge, ridgeTitle));
    click(find(QStringLiteral("backgroundTabLinkButton")));
    QCOMPARE(tabs->tabs().last().groupId, reading);
    QCOMPARE(bannerText("bannerDetail"), QStringLiteral("The ridge loop · in Reading"));
    holdOn(readingView, heldOn(QStringLiteral("https://trails.example/maps")));
    click(find(QStringLiteral("backgroundTabLinkButton")));
    QCOMPARE(bannerText("bannerDetail"), QStringLiteral("trails.example/maps · in Reading"));
    QCOMPARE(tabs->tabs().last().title, QString());

    // Share: the link, as the menu shares the page.
    QObject *share = find(QStringLiteral("linkShareAction"));
    holdOn(readingView, heldOn(ridge, ridgeTitle));
    click(find(QStringLiteral("shareLinkButton")));
    QVERIFY(!menu->property("open").toBool());
    QCOMPARE(share->property("triggerCount").toInt(), 1);
    QCOMPARE(share->property("mimeType").toString(), QStringLiteral("text/x-url"));
    const QVariantMap resource = share->property("resources").toList().first().toMap();
    QCOMPARE(resource.value(QStringLiteral("status")).toString(), ridge);
    QCOMPARE(resource.value(QStringLiteral("linkTitle")).toString(), ridgeTitle);

    // Save link: the engine is asked for it, into the downloads folder, under its own name.
    evaluate(scope, QStringLiteral("WebEngine.notifications = []"));
    const QString map = QStringLiteral("https://files.example/maps/ridge-loop.pdf?v=2");
    holdOn(readingView, heldOn(map, QStringLiteral("The map")));
    click(find(QStringLiteral("saveLinkButton")));
    QVERIFY(!menu->property("open").toBool());
    const QVariantList sent = evaluate(scope, QStringLiteral("WebEngine.notifications")).toList();
    QCOMPARE(sent.count(), 1);
    QCOMPARE(sent.first().toMap().value(QStringLiteral("topic")).toString(),
             QStringLiteral("embedui:download"));
    QCOMPARE(
        sent.first().toMap().value(QStringLiteral("value")).toMap(),
        QVariantMap(
            {{QStringLiteral("msg"), QStringLiteral("addDownload")},
             {QStringLiteral("from"), map},
             {QStringLiteral("to"),
              QDir(m_core->downloads()->directory()).filePath(QStringLiteral("ridge-loop.pdf"))}}));

    // The head's copy button: the link on the clipboard, and a word that it is.
    QObject *notice = find(QStringLiteral("linkCopiedNotice"));
    holdOn(readingView, heldOn(ridge, ridgeTitle));
    click(find(QStringLiteral("copyLinkButton")));
    QVERIFY(!menu->property("open").toBool());
    QCOMPARE(evaluate(scope, QStringLiteral("Clipboard.text")).toString(), ridge);
    QCOMPARE(notice->property("shownCount").toInt(), 1);
    QCOMPARE(notice->property("shownText").toString(), QStringLiteral("Link copied"));
    QCOMPARE(errors.all(), QString());
}

// A link another application takes: that application's action, and Share, and no tab
// or preview; the head shows and copies the mailbox or the number without its scheme.
void tst_qmlload::linkMenuForOtherApps()
{
    ScriptErrors errors;
    QObject *menu = find(QStringLiteral("linkMenu"));
    QObject *view = currentWebView();
    QObject *scope = find(QStringLiteral("viewArea"));
    QObject *app = find(QStringLiteral("appLinkButton"));
    const QList<QStringList> links{
        {QStringLiteral("mailto:walks@trails.example"), QStringLiteral("Write email"),
         QStringLiteral("image://theme/icon-m-mail"), QStringLiteral("walks@trails.example")},
        {QStringLiteral("tel:+358401234567"), QStringLiteral("Call"),
         QStringLiteral("image://theme/icon-m-call"), QStringLiteral("+358401234567")},
        {QStringLiteral("sms:+358401234567"), QStringLiteral("Send message"),
         QStringLiteral("image://theme/icon-m-sms"), QStringLiteral("+358401234567")},
        {QStringLiteral("geo:60.17,24.94"), QStringLiteral("Show on map"),
         QStringLiteral("image://theme/icon-m-location"), QStringLiteral("60.17,24.94")},
    };
    for (const QStringList &link : links) {
        holdOn(view, heldOn(link.at(0), QStringLiteral("Write to us")));
        QVERIFY2(menu->property("open").toBool(), qPrintable(link.at(0)));
        QVERIFY(shownIn(find(QStringLiteral("linkAppRow"))));
        QVERIFY(!shownIn(find(QStringLiteral("linkPageRow"))));
        QVERIFY(!shownIn(find(QStringLiteral("linkImageRow"))));
        QVERIFY(!shownIn(find(QStringLiteral("linkPreview"))));
        QCOMPARE(app->property("text").toString(), link.at(1));
        QCOMPARE(app->property("iconSource").toString(), link.at(2));
        QVERIFY(shownIn(find(QStringLiteral("linkMenuAppIcon"))));
        QCOMPARE(find(QStringLiteral("linkMenuAppIcon"))->property("source").toString(),
                 link.at(2));
        QCOMPARE(find(QStringLiteral("linkMenuAddress"))->property("text").toString(), link.at(3));
        QCOMPARE(find(QStringLiteral("shareAppLinkButton"))->property("text").toString(),
                 QStringLiteral("Share"));
        evaluate(menu, QStringLiteral("hide()"));
    }

    // The action hands the link to its application; no tab is made for it.
    const UrlCatcher mail(QStringLiteral("mailto"));
    holdOn(view, heldOn(QStringLiteral("mailto:walks@trails.example")));
    click(app);
    QVERIFY(!menu->property("open").toBool());
    QCOMPARE(mail.opened, QList<QUrl>{QUrl(QStringLiteral("mailto:walks@trails.example"))});
    QCOMPARE(m_core->tabs()->count(), 1);

    // Copied, the mailbox alone.
    holdOn(view, heldOn(QStringLiteral("mailto:walks@trails.example")));
    click(find(QStringLiteral("copyLinkButton")));
    QCOMPARE(evaluate(scope, QStringLiteral("Clipboard.text")).toString(),
             QStringLiteral("walks@trails.example"));
    QCOMPARE(find(QStringLiteral("linkCopiedNotice"))->property("shownText").toString(),
             QStringLiteral("Copied"));

    // Shared, the link as it is.
    holdOn(view, heldOn(QStringLiteral("tel:+358401234567")));
    click(find(QStringLiteral("shareAppLinkButton")));
    QCOMPARE(find(QStringLiteral("linkShareAction"))
                 ->property("resources")
                 .toList()
                 .first()
                 .toMap()
                 .value(QStringLiteral("status"))
                 .toString(),
             QStringLiteral("tel:+358401234567"));
    QCOMPARE(errors.all(), QString());
}

// A picture: lifted out of the page above the sheet, where it can be pinched closer, with
// its own row of actions after the link's, and no page preview.
void tst_qmlload::linkMenuForPictures()
{
    ScriptErrors errors;
    TabModel *tabs = m_core->tabs();
    QObject *menu = find(QStringLiteral("linkMenu"));
    QObject *view = currentWebView();
    QObject *scope = find(QStringLiteral("viewArea"));
    const QString photo = QStringLiteral("https://cdn.example/photos/ridge.jpg");
    const QString mapLink = QStringLiteral("https://trails.example/maps/ridge");

    // A picture that is a link: both rows, the menu's line between them, no preview.
    holdOn(view, heldOn(mapLink, QString(), photo));
    QVERIFY(menu->property("open").toBool());
    QVERIFY(shownIn(find(QStringLiteral("linkPageRow"))));
    QVERIFY(shownIn(find(QStringLiteral("linkImageRow"))));
    QVERIFY(shownIn(
        qobject_cast<QQuickItem *>(find(QStringLiteral("linkMenuSeparator")))->parentItem()));
    QVERIFY(!shownIn(find(QStringLiteral("linkPreview"))));
    QVERIFY(!menu->property("previewShown").toBool());
    QVERIFY(view->property("active").toBool());
    // The head shows the picture on its tile and the link under it.
    QCOMPARE(find(QStringLiteral("linkMenuThumbnail"))->property("source").toString(), photo);
    QCOMPARE(find(QStringLiteral("linkMenuTitle"))->property("text").toString(),
             QStringLiteral("trails.example/maps/ridge"));
    // Lifted above the sheet: the room from under the cutout to the sheet, the picture
    // as wide as the screen and pinched closer from its middle.
    QObject *page = find(QStringLiteral("browserPage"));
    auto *area = qobject_cast<QQuickItem *>(find(QStringLiteral("linkMenuPictureArea")));
    QObject *picture = find(QStringLiteral("linkMenuPicture"));
    QCOMPARE(picture->property("source").toString(), photo);
    QCOMPARE(area->y(), page->property("pageCutoutInset").toReal());
    QCOMPARE(area->y() + area->height(),
             page->property("height").toReal() - menu->property("height").toReal());
    QCOMPARE(picture->property("width").toReal(), page->property("width").toReal());
    QCOMPARE(picture->property("fillMode").toInt(), 1); // Image.PreserveAspectFit
    QObject *pinch = find(QStringLiteral("linkMenuPinch"));
    QCOMPARE(evaluate(pinch, QStringLiteral("pinch.target === parent.children[0]")).toBool(), true);
    QCOMPARE(evaluate(pinch, QStringLiteral("pinch.maximumScale")).toReal(), 4.0);
    QCOMPARE(evaluate(pinch, QStringLiteral("pinch.minimumScale")).toReal(), 1.0);
    // A picture pinched closer starts the next one at its own size.
    picture->setProperty("scale", 2.5);
    holdOn(view, heldOn(QString(), QString(), QStringLiteral("https://cdn.example/b.jpg")));
    QCOMPARE(picture->property("scale").toReal(), 1.0);

    // A picture alone: its row only, named by its address.
    QVERIFY(!shownIn(find(QStringLiteral("linkPageRow"))));
    QVERIFY(shownIn(find(QStringLiteral("linkImageRow"))));
    QVERIFY(!shownIn(
        qobject_cast<QQuickItem *>(find(QStringLiteral("linkMenuSeparator")))->parentItem()));
    QCOMPARE(find(QStringLiteral("linkMenuTitle"))->property("text").toString(),
             QStringLiteral("cdn.example/b.jpg"));
    const QList<QPair<QString, QString>> actions{
        {QStringLiteral("openImageButton"), QStringLiteral("Open image")},
        {QStringLiteral("saveImageButton"), QStringLiteral("Save image")},
        {QStringLiteral("copyImageLinkButton"), QStringLiteral("Copy image link")},
    };
    for (const auto &action : actions) {
        QObject *button = find(action.first);
        QCOMPARE(button->property("text").toString(), action.second);
        QVERIFY(button->property("round").toBool());
        QVERIFY(!button->property("iconSource").toString().isEmpty());
    }
    // Copied from the head, the picture's address.
    click(find(QStringLiteral("copyLinkButton")));
    QCOMPARE(evaluate(scope, QStringLiteral("Clipboard.text")).toString(),
             QStringLiteral("https://cdn.example/b.jpg"));
    QCOMPARE(find(QStringLiteral("linkCopiedNotice"))->property("shownText").toString(),
             QStringLiteral("Image link copied"));

    // Open image: the picture alone, in a tab in front.
    holdOn(view, heldOn(mapLink, QString(), photo));
    click(find(QStringLiteral("openImageButton")));
    QVERIFY(!menu->property("open").toBool());
    QCOMPARE(tabs->count(), 2);
    QCOMPARE(tabs->activeUrl(), photo);
    view = currentWebView();
    // Save image: downloaded under its own name.
    evaluate(scope, QStringLiteral("WebEngine.notifications = []"));
    holdOn(view, heldOn(mapLink, QString(), photo));
    click(find(QStringLiteral("saveImageButton")));
    const QVariantList sent = evaluate(scope, QStringLiteral("WebEngine.notifications")).toList();
    QCOMPARE(sent.count(), 1);
    const QVariantMap asked = sent.first().toMap().value(QStringLiteral("value")).toMap();
    QCOMPARE(asked.value(QStringLiteral("from")).toString(), photo);
    QCOMPARE(asked.value(QStringLiteral("to")).toString(),
             QDir(m_core->downloads()->directory()).filePath(QStringLiteral("ridge.jpg")));
    // Copy image link: its address, said so.
    holdOn(view, heldOn(mapLink, QString(), photo));
    click(find(QStringLiteral("copyImageLinkButton")));
    QVERIFY(!menu->property("open").toBool());
    QCOMPARE(evaluate(scope, QStringLiteral("Clipboard.text")).toString(), photo);
    QCOMPARE(find(QStringLiteral("linkCopiedNotice"))->property("shownText").toString(),
             QStringLiteral("Image link copied"));
    // Gone with the sheet.
    QVERIFY(!find(QStringLiteral("linkMenuOverlay"))->property("shown").toBool());
    QCOMPARE(errors.all(), QString());
}

// The page a link leads to, previewed in the sheet as Safari's link preview is: shown for
// every link until hidden, and hidden for every link until shown again. The engine draws
// one picture, so the page in front is put aside, a still of it in its place, while the
// preview is drawn.
void tst_qmlload::linkPreview()
{
    ScriptErrors errors;
    TabModel *tabs = m_core->tabs();
    QObject *menu = find(QStringLiteral("linkMenu"));
    QObject *view = currentWebView();
    QObject *page = find(QStringLiteral("browserPage"));
    QVERIFY(m_core->settings()->linkPreview());
    QVERIFY(view->property("active").toBool());
    const QString ridge = QStringLiteral("https://trails.example/walks/ridge-loop");

    holdOn(view, heldOn(ridge, QStringLiteral("The ridge loop")));
    QObject *toggle = find(QStringLiteral("linkPreviewToggleLabel"));
    QObject *loader = find(QStringLiteral("linkPreviewLoader"));
    QObject *still = find(QStringLiteral("linkMenuStill"));
    QVERIFY(shownIn(find(QStringLiteral("linkPreview"))));
    QVERIFY(shownIn(find(QStringLiteral("linkPreviewFrame"))));
    QCOMPARE(toggle->property("text").toString(), QStringLiteral("Hide preview"));
    QVERIFY(menu->property("previewShown").toBool());
    // The page the link leads to, in the frame.
    QVERIFY(loader->property("active").toBool());
    auto *preview = loader->property("item").value<QObject *>();
    QVERIFY(preview != nullptr);
    QCOMPARE(preview->objectName(), QStringLiteral("linkPreviewView"));
    QCOMPARE(preview->property("url").toString(), ridge);
    // The page in front put aside, and a still of it where it was.
    QVERIFY(!view->property("active").toBool());
    QVERIFY(!view->property("visible").toBool());
    QTRY_VERIFY(shownIn(still));
    QCOMPARE(still->property("source").toString(),
             QStringLiteral("image://grab/%1").arg(view->property("grabCount").toInt()));
    QCOMPARE(still->property("y").toReal(), page->property("pageCutoutInset").toReal());
    QCOMPARE(still->property("height").toReal(), view->property("height").toReal());
    QCOMPARE(still->property("width").toReal(), view->property("width").toReal());
    // Taken at the page's own size: it stands where the page was.
    QCOMPARE(view->property("lastGrabSize").toSizeF(),
             QSizeF(view->property("width").toReal(), view->property("height").toReal()));

    // A tap on the preview opens the link where the page was, and the page is back.
    evaluate(find(QStringLiteral("linkPreviewTap")), QStringLiteral("clicked(null)"));
    QVERIFY(!menu->property("open").toBool());
    QVERIFY(!menu->property("previewShown").toBool());
    QVERIFY(!loader->property("active").toBool());
    QCOMPARE(view->property("url").toString(), ridge);
    QCOMPARE(tabs->count(), 1);
    QVERIFY(view->property("active").toBool());
    QVERIFY(view->property("visible").toBool());
    QVERIFY(!shownIn(still));

    // Hidden, it stays hidden, for every link and across restarts: it is a setting.
    holdOn(view, heldOn(QStringLiteral("https://trails.example/other"), QStringLiteral("Other")));
    QVERIFY(menu->property("previewShown").toBool());
    click(find(QStringLiteral("linkPreviewToggle")));
    QVERIFY(menu->property("open").toBool());
    QVERIFY(!m_core->settings()->linkPreview());
    QCOMPARE(toggle->property("text").toString(), QStringLiteral("Show preview"));
    QVERIFY(!menu->property("previewShown").toBool());
    QVERIFY(!loader->property("active").toBool());
    QVERIFY(!shownIn(find(QStringLiteral("linkPreviewFrame"))));
    QVERIFY(view->property("active").toBool());
    QVERIFY(view->property("visible").toBool());
    evaluate(menu, QStringLiteral("hide()"));
    holdOn(view, heldOn(ridge, QStringLiteral("The ridge loop")));
    QVERIFY(shownIn(find(QStringLiteral("linkPreview"))));
    QVERIFY(!menu->property("previewShown").toBool());
    click(find(QStringLiteral("linkPreviewToggle")));
    QVERIFY(m_core->settings()->linkPreview());
    QVERIFY(menu->property("previewShown").toBool());
    QVERIFY(loader->property("active").toBool());

    // Put away, the preview goes with the sheet, and its still.
    evaluate(menu, QStringLiteral("hide()"));
    QVERIFY(!menu->property("previewShown").toBool());
    QVERIFY(!loader->property("active").toBool());
    QVERIFY(menu->property("pageStill").isNull() ||
            menu->property("pageStill").value<QObject *>() == nullptr);
    QVERIFY(view->property("active").toBool());

    // A page that cannot be pictured is not put aside: nothing would stand in its place.
    view->setProperty("grabFails", true);
    holdOn(view, heldOn(ridge, QStringLiteral("The ridge loop")));
    QVERIFY(!menu->property("previewShown").toBool());
    QVERIFY(view->property("active").toBool());
    evaluate(menu, QStringLiteral("hide()"));
    view->setProperty("grabFails", false);

    // Not offered while the page in front plays: put aside, it would be paused.
    tabs->setMediaState(tabs->activeTabId(), TabModel::MediaPlaying);
    holdOn(view, heldOn(ridge, QStringLiteral("The ridge loop")));
    QVERIFY(!shownIn(find(QStringLiteral("linkPreview"))));
    QVERIFY(!menu->property("previewShown").toBool());
    QVERIFY(view->property("active").toBool());
    evaluate(menu, QStringLiteral("hide()"));
    tabs->setMediaState(tabs->activeTabId(), TabModel::NoMedia);
    QCOMPARE(errors.all(), QString());
}

// The page ends where the banners on the bar begin, with one of them up or two, and has
// its room back when they go: a banner never lies over the foot of a page.
void tst_qmlload::bannersEndThePage()
{
    QObject *page = find(QStringLiteral("browserPage"));
    QObject *bar = find(QStringLiteral("navigationBar"));
    QObject *viewArea = find(QStringLiteral("viewArea"));
    QObject *banners = find(QStringLiteral("barBanners"));
    QObject *downloads = find(QStringLiteral("downloadBanner"));
    QObject *opened = find(QStringLiteral("tabBanner"));
    QObject *webView = currentWebView();
    const qreal pageHeight = page->property("height").toReal();
    const qreal inset = page->property("pageCutoutInset").toReal();
    const qreal fullBar = bar->property("height").toReal();
    const auto pageEnd = [viewArea]() {
        return viewArea->property("y").toReal() + viewArea->property("height").toReal();
    };
    QCOMPARE(banners->property("height").toReal(), 0.0);
    QCOMPARE(pageEnd(), pageHeight - fullBar);

    evaluate(viewArea, QStringLiteral("WebEngine.recvObserve('embed:download', {msg: 'dl-start',"
                                      " id: 1, displayName: 'a.pdf', size: 0})"));
    QTRY_VERIFY(downloads->property("visible").toBool());
    const qreal one = downloads->property("height").toReal();
    QVERIFY(one > 0);
    QCOMPARE(banners->property("height").toReal(), one);
    QCOMPARE(pageEnd(), banners->property("y").toReal());
    QCOMPARE(pageEnd(), pageHeight - fullBar - one);
    QCOMPARE(webView->property("height").toReal(), pageHeight - fullBar - one - inset);

    const int behind =
        m_core->tabs()->newTabBehind(QStringLiteral("https://b.example/"), QStringLiteral("B"));
    evaluate(banners, QStringLiteral("tabOpened(%1, 'B')").arg(behind));
    QTRY_VERIFY(opened->property("visible").toBool());
    QCOMPARE(banners->property("height").toReal(), 2 * one);
    QCOMPARE(pageEnd(), banners->property("y").toReal());
    QCOMPARE(banners->property("y").toReal() + 2 * one, bar->property("y").toReal());

    // Gone, the page has its room back.
    evaluate(opened, QStringLiteral("dismiss()"));
    evaluate(downloads, QStringLiteral("dismiss()"));
    QTRY_VERIFY(!opened->property("visible").toBool());
    QTRY_VERIFY(!downloads->property("visible").toBool());
    QCOMPARE(banners->property("height").toReal(), 0.0);
    QCOMPARE(pageEnd(), pageHeight - fullBar);
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
                 ->data(m_core->bookmarks()->index(1, 0), roleId(BookmarkModel::Role::Url))
                 .toString(),
             QStringLiteral("https://b2.example/x"));
    QCOMPARE(m_core->bookmarks()
                 ->data(m_core->bookmarks()->index(1, 0), roleId(BookmarkModel::Role::Title))
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

// Settings is a main page with a way each to a page of its own for the start page,
// search, the reader view, the cover, tracking protection, notifications and the
// history, under the headings Browsing, Appearance, Privacy and Help, and the two
// settings that take a line, the website colours and the cutout, last under Appearance
// (docs/DECISIONS/0028-settings-pages.md). Each way in is a theme icon, a name and under
// it how the subject is set, and pushes its page over the main one.
void tst_qmlload::settingsPage()
{
    QObject *page = openMenuItem(QStringLiteral("settingsMenuButton"));
    QCOMPARE(page->objectName(), QStringLiteral("settingsPage"));
    // Opened, it asks the engine for the sites' notification permissions, which the line
    // under Notifications counts.
    QCOMPARE(evaluate(find(QStringLiteral("viewArea")),
                      QStringLiteral("WebEngine.notifications[WebEngine.notifications.length - 1]"
                                     ".value.msg"))
                 .toString(),
             QStringLiteral("get-all"));

    const QStringList expected{
        QStringLiteral("#Browsing"),
        QStringLiteral("startPageSettingsEntry"),
        QStringLiteral("searchSettingsEntry"),
        QStringLiteral("#Appearance"),
        QStringLiteral("readerSettingsEntry"),
        QStringLiteral("coverSettingsEntry"),
        QStringLiteral("websiteColorsCombo"),
        QStringLiteral("notchGuardCombo"),
        QStringLiteral("fixedToolbarSwitch"),
        QStringLiteral("#Privacy"),
        QStringLiteral("trackingSettingsEntry"),
        QStringLiteral("doNotTrackSwitch"),
        QStringLiteral("javascriptSwitch"),
        QStringLiteral("sitePermissionsSettingsEntry"),
        QStringLiteral("historySettingsEntry"),
        QStringLiteral("#Help"),
        QStringLiteral("tutorialSettingsEntry"),
    };
    QCOMPARE(columnOf(find(QStringLiteral("searchSettingsEntry"))), expected);

    // Each way in, with the theme icon a Jolla application gives the same subject --
    // the cover's is the tab count's, not the display's, which sailfish-browser has
    // for what is the screen cutout here -- and how the subject is set now, as the
    // choices are named on its page.
    struct Entry
    {
        QString name;
        QString icon;
        QString page;
        QString value;
    };
    const QList<Entry> entries{
        {QStringLiteral("startPageSettingsEntry"), QStringLiteral("icon-m-home"),
         QStringLiteral("startPageSettingsPage"), QStringLiteral("Your sites")},
        {QStringLiteral("searchSettingsEntry"), QStringLiteral("icon-m-search"),
         QStringLiteral("searchSettingsPage"), QStringLiteral("Qwant")},
        {QStringLiteral("readerSettingsEntry"), QStringLiteral("icon-m-file-formatted"),
         QStringLiteral("readerSettingsPage"), QStringLiteral("Ambience · Sans serif · 100 %")},
        {QStringLiteral("coverSettingsEntry"), QStringLiteral("icon-m-tabs"),
         QStringLiteral("coverSettingsPage"), QStringLiteral("Search")},
        {QStringLiteral("trackingSettingsEntry"), QStringLiteral("icon-m-device-lock"),
         QStringLiteral("trackingSettingsPage"), QStringLiteral("Standard")},
        {QStringLiteral("sitePermissionsSettingsEntry"),
         QStringLiteral("icon-m-browser-permissions"), QStringLiteral("sitePermissionsPage"),
         QStringLiteral("No exceptions")},
        {QStringLiteral("historySettingsEntry"), QStringLiteral("icon-m-history"),
         QStringLiteral("historySettingsPage"), QStringLiteral("Remembered")},
        {QStringLiteral("tutorialSettingsEntry"), QStringLiteral("icon-m-gesture"),
         QStringLiteral("tutorialPage"), QString()},
    };
    const qreal itemSize = evaluate(page, QStringLiteral("Theme.itemSizeMedium")).toReal();
    const qreal iconSize = evaluate(page, QStringLiteral("Theme.iconSizeMedium")).toReal();
    for (const Entry &entry : entries) {
        QObject *item = find(entry.name);
        QVERIFY2(item != nullptr, qPrintable(entry.name));
        QCOMPARE(item->property("iconSource").toString(),
                 QStringLiteral("image://theme/") + entry.icon);
        QVERIFY2(!item->property("text").toString().isEmpty(), qPrintable(entry.name));
        QCOMPARE(item->property("value").toString(), entry.value);
        QObject *value = findObjects(item, QStringLiteral("settingsEntryValue")).first();
        QCOMPARE(value->property("text").toString(), entry.value);
        QCOMPARE(value->property("visible").toBool(), !entry.value.isEmpty());
        QCOMPARE(item->property("height").toReal(), itemSize);
        QObject *icon = findObjects(item, QStringLiteral("settingsEntryIcon")).first();
        QCOMPARE(icon->property("width").toReal(), iconSize);
        QCOMPARE(findObjects(item, QStringLiteral("settingsEntryName")).first()->property("text"),
                 item->property("text"));
        click(item);
        QCOMPARE(currentPage()->objectName(), entry.page);
        QCOMPARE(pageStack()->property("depth").toInt(), 3);
        popPage();
        QCOMPARE(currentPage(), page);
    }

    // The two settings made in place stand in the same column of icons, each with the
    // one sailfish-browser gives the same subject -- its colour scheme's, and its notch
    // guard's -- at the page's margin, the control moved in past it, and lit with it.
    const qreal margin = evaluate(page, QStringLiteral("Theme.horizontalPageMargin")).toReal();
    const qreal gap = evaluate(page, QStringLiteral("Theme.paddingMedium")).toReal();
    struct InPlace
    {
        QString name;
        QString iconItem;
        QString icon;
    };
    const QList<InPlace> inPlace{
        {QStringLiteral("websiteColorsCombo"), QStringLiteral("settingsComboBoxIcon"),
         QStringLiteral("icon-m-night")},
        {QStringLiteral("notchGuardCombo"), QStringLiteral("settingsComboBoxIcon"),
         QStringLiteral("icon-m-display")},
    };
    for (const InPlace &setting : inPlace) {
        QObject *control = find(setting.name);
        QObject *icon = findObjects(control, setting.iconItem).first();
        QCOMPARE(icon->property("source").toUrl(),
                 QUrl(QStringLiteral("image://theme/") + setting.icon));
        QCOMPARE(icon->property("x").toReal(), margin);
        QCOMPARE(control->property("leftMargin").toReal(),
                 margin + icon->property("width").toReal() + gap);
        QVERIFY(!icon->property("highlighted").toBool());
        control->setProperty("highlighted", true);
        QVERIFY(icon->property("highlighted").toBool());
        control->setProperty("highlighted", false);
    }

    // Lit while it is pressed, as Silica's rows are: the name in the highlight colour,
    // and the value under it in the secondary highlight until then.
    QObject *search = find(QStringLiteral("searchSettingsEntry"));
    QObject *searchName = findObjects(search, QStringLiteral("settingsEntryName")).first();
    QObject *searchValue = findObjects(search, QStringLiteral("settingsEntryValue")).first();
    QCOMPARE(searchName->property("color"), evaluate(page, QStringLiteral("Theme.primaryColor")));
    QCOMPARE(searchValue->property("color"),
             evaluate(page, QStringLiteral("Theme.secondaryHighlightColor")));
    QCOMPARE(searchValue->property("font").value<QFont>().pixelSize(),
             evaluate(page, QStringLiteral("Theme.fontSizeExtraSmall")).toInt());
    search->setProperty("down", true);
    QVERIFY(findObjects(search, QStringLiteral("settingsEntryIcon"))
                .first()
                ->property("highlighted")
                .toBool());
    QCOMPARE(searchName->property("color"), evaluate(page, QStringLiteral("Theme.highlightColor")));
    search->setProperty("down", false);

    // Each value follows its setting wherever it is written from.
    auto valueOf = [this](const QString &entry) {
        return find(entry)->property("value").toString();
    };
    m_core->startPageSettings()->setBlank(true);
    QCOMPARE(valueOf(QStringLiteral("startPageSettingsEntry")), QStringLiteral("Blank page"));
    m_core->searchSettings()->setEngineIndex(1);
    QCOMPARE(valueOf(QStringLiteral("searchSettingsEntry")),
             m_core->searchEngines()->engineNames().at(1));
    m_core->readerSettings()->setColors(ReaderSettings::Sepia);
    m_core->readerSettings()->setTypeface(ReaderSettings::Serif);
    m_core->readerSettings()->setTextSize(ReaderSettings::TextSizeMax);
    QCOMPARE(valueOf(QStringLiteral("readerSettingsEntry")),
             QStringLiteral("Sepia · Serif · 140 %"));
    m_core->coverSettings()->setQuickAction(CoverSettings::QuickActionNone);
    QCOMPARE(valueOf(QStringLiteral("coverSettingsEntry")), QStringLiteral("No quick action"));
    const int bookmark = m_core->bookmarks()->add(QStringLiteral("https://yle.fi/uutiset"),
                                                  QStringLiteral("Yle Uutiset"));
    m_core->coverSettings()->setQuickActionBookmark(
        bookmark, QStringLiteral("https://yle.fi/uutiset"), QStringLiteral("Yle Uutiset"));
    m_core->coverSettings()->setQuickAction(CoverSettings::QuickActionBookmark);
    QCOMPARE(valueOf(QStringLiteral("coverSettingsEntry")), QStringLiteral("Yle Uutiset"));
    m_core->privacySettings()->setTrackingProtection(PrivacySettings::TrackingProtectionStrict);
    QCOMPARE(valueOf(QStringLiteral("trackingSettingsEntry")), QStringLiteral("Strict"));
    // Site permissions counts the sites decided for, whichever permission it was: a
    // notification answer is one, and kept in step with the notifications' own list.
    NotificationPermissions *sites = m_core->notificationPermissions();
    sites->setAllowed(QStringLiteral("https://mastodon.social"), true);
    QCOMPARE(valueOf(QStringLiteral("sitePermissionsSettingsEntry")),
             QStringLiteral("1 site(s) with exceptions"));
    sites->setAllowed(QStringLiteral("https://app.element.io"), true);
    sites->setAllowed(QStringLiteral("https://www.iltalehti.fi"), false);
    QCOMPARE(valueOf(QStringLiteral("sitePermissionsSettingsEntry")),
             QStringLiteral("3 site(s) with exceptions"));
    sites->setAllowed(QStringLiteral("https://mastodon.social"), false);
    QCOMPARE(valueOf(QStringLiteral("sitePermissionsSettingsEntry")),
             QStringLiteral("3 site(s) with exceptions"));
    m_core->privacySettings()->setClearHistoryOnClose(true);
    QCOMPARE(valueOf(QStringLiteral("historySettingsEntry")),
             QStringLiteral("Cleared when closed"));
    m_core->privacySettings()->setRememberHistory(false);
    QCOMPARE(valueOf(QStringLiteral("historySettingsEntry")), QStringLiteral("Not remembered"));

    // The website colours, Automatic until changed; each choice reaches the engine at
    // once, through the browsing page, as whether the page is drawn dark. The stub's
    // ambience is a dark one.
    QObject *colors = find(QStringLiteral("websiteColorsCombo"));
    QCOMPARE(colors->property("currentIndex").toInt(), int(Settings::WebsiteColorsAutomatic));
    QObject *pageScope = find(QStringLiteral("viewArea"));
    const auto lastDark = [this, pageScope]() {
        const QVariantList given =
            evaluate(pageScope, QStringLiteral("WebEngineSettings.preferences")).toList();
        for (int i = given.count() - 1; i >= 0; --i) {
            if (given.at(i).toMap().value(QStringLiteral("key")) ==
                QStringLiteral("ui.systemUsesDarkTheme")) {
                return given.at(i).toMap().value(QStringLiteral("value")).toInt();
            }
        }
        return -1;
    };
    QCOMPARE(lastDark(), 1);
    colors->setProperty("currentIndex", int(Settings::WebsiteColorsLight));
    QCOMPARE(m_core->settings()->websiteColors(), int(Settings::WebsiteColorsLight));
    QCOMPARE(lastDark(), 0);
    colors->setProperty("currentIndex", int(Settings::WebsiteColorsDark));
    QCOMPARE(lastDark(), 1);
    colors->setProperty("currentIndex", int(Settings::WebsiteColorsAutomatic));
    QCOMPARE(m_core->settings()->websiteColors(), int(Settings::WebsiteColorsAutomatic));
}

// Appearance and Privacy's rows from sailfish-browser, in its words, and what each does
// to the browsing page and the engine (docs/DECISIONS/0043-notch-guard-modes.md,
// 0044-sailfish-browser-settings.md).
void tst_qmlload::sailfishBrowserSettings()
{
    QObject *page = openMenuItem(QStringLiteral("settingsMenuButton"));
    QObject *colors = find(QStringLiteral("websiteColorsCombo"));
    // A row has an icon or a switch, never both: each switch's light stands centred on
    // the column of icons, where sailfish-browser centres its own.
    const qreal switchMargin =
        evaluate(page, QStringLiteral("Theme.horizontalPageMargin + Theme.paddingLarge"
                                      " + Math.round((Theme.iconSizeMedium"
                                      " - Theme.itemSizeExtraSmall) / 2)"))
            .toReal();
    for (const QString &name :
         {QStringLiteral("fixedToolbarSwitch"), QStringLiteral("doNotTrackSwitch"),
          QStringLiteral("javascriptSwitch")}) {
        QObject *control = find(name);
        QVERIFY2(findObjects(control, QStringLiteral("settingsSwitchIcon")).isEmpty(),
                 qPrintable(name));
        QCOMPARE(control->property("leftMargin").toReal(), switchMargin);
        QVERIFY2(!control->property("description").toString().isEmpty(), qPrintable(name));
    }

    // The colour scheme and the notch guard in sailfish-browser's words, which say what
    // each is for.
    QCOMPARE(colors->property("label").toString(), QStringLiteral("Preferred color scheme"));
    QCOMPARE(colors->property("description").toString(),
             QStringLiteral("The website style to use when available"));
    QCOMPARE(evaluate(page, QStringLiteral("names.websiteColors(Settings.WebsiteColorsAutomatic)"))
                 .toString(),
             QStringLiteral("Match ambience"));

    // The notch guard: Automatic until changed, and the browsing page answers each mode.
    // Automatic keeps a page below the cutout unless it asked for the whole screen;
    // Forced keeps every page below it; Disabled none. The grid's head row keeps out of
    // it unless the guard is disabled (docs/DECISIONS/0013-screen-cutout.md).
    QObject *guard = find(QStringLiteral("notchGuardCombo"));
    QCOMPARE(guard->property("label").toString(), QStringLiteral("Notch guard"));
    QVERIFY(guard->property("description")
                .toString()
                .startsWith(QStringLiteral("Keeps website content away from the screen notch.")));
    QCOMPARE(guard->property("currentIndex").toInt(), int(Settings::NotchGuardAutomatic));
    QObject *browser = find(QStringLiteral("browserPage"));
    QObject *webView = currentWebView();
    const qreal cutout = browser->property("cutoutHeight").toReal();
    QVERIFY(cutout > 0);
    QCOMPARE(browser->property("pageCutoutInset").toReal(), cutout);
    auto *viewport = webView->property("viewport").value<QObject *>();
    viewport->setProperty("coversCutout", true);
    QCOMPARE(browser->property("pageCutoutInset").toReal(), qreal(0));
    // Such a page is told where the cutout is, through the platform's safe area, which
    // a page kept below it is not.
    QVERIFY(webView->property("safeAreaTop").toReal() > 0);
    QCOMPARE(find(QStringLiteral("tabsView"))->property("cutoutHeight").toReal(), cutout);
    guard->setProperty("currentIndex", int(Settings::NotchGuardForced));
    QCOMPARE(m_core->settings()->notchGuard(), int(Settings::NotchGuardForced));
    QCOMPARE(browser->property("pageCutoutInset").toReal(), cutout);
    QCOMPARE(webView->property("safeAreaTop").toReal(), qreal(0));
    guard->setProperty("currentIndex", int(Settings::NotchGuardDisabled));
    QVERIFY(!m_core->settings()->cutoutGuard());
    QCOMPARE(browser->property("pageCutoutInset").toReal(), qreal(0));
    QCOMPARE(browser->property("cutoutInset").toReal(), qreal(0));
    QCOMPARE(find(QStringLiteral("tabsView"))->property("cutoutHeight").toReal(), qreal(0));
    viewport->setProperty("coversCutout", false);
    guard->setProperty("currentIndex", int(Settings::NotchGuardAutomatic));
    QCOMPARE(browser->property("pageCutoutInset").toReal(), cutout);

    // Fixed toolbar: the bar stays whole while a page is scrolled.
    QObject *toolbar = find(QStringLiteral("fixedToolbarSwitch"));
    QVERIFY(!toolbar->property("checked").toBool());
    toolbar->setProperty("checked", true);
    QVERIFY(m_core->settings()->fixedToolbar());
    webView->setProperty("chrome", false);
    QVERIFY(!browser->property("barCompact").toBool());
    toolbar->setProperty("checked", false);
    QVERIFY(browser->property("barCompact").toBool());
    webView->setProperty("chrome", true);

    // Do not track and JavaScript reach the engine as they change, through the browsing
    // page; JavaScript's line says what switching it off costs.
    QObject *pageScope = find(QStringLiteral("viewArea"));
    auto lastPreference = [&](const QString &name) {
        const QVariantList given =
            evaluate(pageScope, QStringLiteral("WebEngineSettings.preferences")).toList();
        for (int i = given.count() - 1; i >= 0; --i) {
            if (given.at(i).toMap().value(QStringLiteral("key")).toString() == name) {
                return given.at(i).toMap().value(QStringLiteral("value"));
            }
        }
        return QVariant();
    };
    QCOMPARE(lastPreference(QStringLiteral("privacy.donottrackheader.enabled")), QVariant(false));
    QCOMPARE(lastPreference(QStringLiteral("javascript.enabled")), QVariant(true));
    QObject *doNotTrack = find(QStringLiteral("doNotTrackSwitch"));
    doNotTrack->setProperty("checked", true);
    QVERIFY(m_core->privacySettings()->doNotTrack());
    QCOMPARE(lastPreference(QStringLiteral("privacy.donottrackheader.enabled")), QVariant(true));
    QObject *javascript = find(QStringLiteral("javascriptSwitch"));
    QCOMPARE(javascript->property("description").toString(),
             QStringLiteral("Allowed (recommended)"));
    javascript->setProperty("checked", false);
    QVERIFY(!m_core->privacySettings()->javascript());
    QCOMPARE(lastPreference(QStringLiteral("javascript.enabled")), QVariant(false));
    QCOMPARE(javascript->property("description").toString(),
             QStringLiteral("Blocked, some sites may not work correctly"));
}

// Start page: what it shows, the sections or a blank page, both on the page at once and
// the one chosen lit; then the sections, each a switch that is on until it is turned off
// and dimmed while the page is blank; then, under a heading, a picture of the screen as a
// new tab shows it, which follows them.
// There is no home page to set: the start page is the home page
// (docs/DECISIONS/0032-start-page.md).
void tst_qmlload::startPageSettingsPage()
{
    StartPageSettings *settings = m_core->startPageSettings();
    openMenuItem(QStringLiteral("settingsMenuButton"));
    click(find(QStringLiteral("startPageSettingsEntry")));
    QCOMPARE(currentPage()->objectName(), QStringLiteral("startPageSettingsPage"));
    QVERIFY(find(QStringLiteral("homePageField")) == nullptr);
    QObject *sites = find(QStringLiteral("startPageSitesSwitch"));
    QObject *blank = find(QStringLiteral("startPageBlankSwitch"));
    const QStringList sections{QStringLiteral("startPageTopSitesSwitch"),
                               QStringLiteral("startPageBookmarksSwitch"),
                               QStringLiteral("startPageRecentSwitch")};
    const QStringList layout =
        QStringList{QStringLiteral("startPageSitesSwitch"), QStringLiteral("startPageBlankSwitch"),
                    QStringLiteral("#Sections")} +
        sections + QStringList{QStringLiteral("#Preview"), QStringLiteral("startPagePreview")};
    QCOMPARE(columnOf(sites), layout);

    // A choice, not two switches: neither checks itself, and the one set is lit.
    QVERIFY(!sites->property("automaticCheck").toBool());
    QVERIFY(!blank->property("automaticCheck").toBool());
    QCOMPARE(sites->property("text").toString(), QStringLiteral("Your sites"));
    QCOMPARE(blank->property("text").toString(), QStringLiteral("Blank page"));
    QVERIFY(sites->property("checked").toBool());
    QVERIFY(!blank->property("checked").toBool());
    for (const QString &name : sections) {
        QVERIFY2(find(name)->property("checked").toBool(), qPrintable(name));
        QVERIFY2(find(name)->property("enabled").toBool(), qPrintable(name));
        QVERIFY2(find(name)->property("description").toString().isEmpty(), qPrintable(name));
    }

    // The picture is the screen at half its size, the page's own, with the bar along its
    // foot. It shows each section switched on -- one with nothing in it yet as where its
    // tiles and rows go -- and nothing past them.
    const auto shown = [this](const QString &part) {
        return find(part)->property("visible").toBool();
    };
    m_core->history()->clear();
    QObject *frame = find(QStringLiteral("startPagePreviewFrame"));
    QCOMPARE(frame->property("width").toReal(), currentPage()->property("width").toReal() / 2);
    QCOMPARE(frame->property("height").toReal(), currentPage()->property("height").toReal() / 2);
    QVERIFY(shown(QStringLiteral("startPagePreviewBar")));
    QVERIFY(shown(QStringLiteral("startPagePreviewTopSites")));
    QVERIFY(shown(QStringLiteral("startPagePreviewBookmarks")));
    QVERIFY(shown(QStringLiteral("startPagePreviewRecent")));
    QCOMPARE(findAll(QStringLiteral("startPagePreviewSpareTile")).count(), 8);
    QCOMPARE(findAll(QStringLiteral("startPagePreviewSpareRow")).count(), 3);
    QVERIFY(findAll(QStringLiteral("startPagePreviewTile")).isEmpty());

    // What the reader has is drawn as the start page draws it, in place of the spare
    // tiles and rows: a site visited is a tile and a row, and a bookmark a tile under its
    // title.
    m_core->history()->visit(QStringLiteral("https://example.org/"), QStringLiteral("Example"));
    QCOMPARE(findAll(QStringLiteral("startPagePreviewSpareTile")).count(), 4);
    QVERIFY(findAll(QStringLiteral("startPagePreviewSpareRow")).isEmpty());
    const QList<QObject *> rows = findAll(QStringLiteral("startPagePreviewRow"));
    QCOMPARE(rows.count(), 1);
    QCOMPARE(rows.first()->property("title").toString(), QStringLiteral("Example"));
    m_core->bookmarks()->add(QStringLiteral("https://sailfishos.org/"),
                             QStringLiteral("Sailfish OS"));
    QVERIFY(findAll(QStringLiteral("startPagePreviewSpareTile")).isEmpty());
    const QList<QObject *> tiles = findAll(QStringLiteral("startPagePreviewTile"));
    QCOMPARE(tiles.count(), 2);
    QCOMPARE(textOf(tiles.last(), "siteTileName"), QStringLiteral("Sailfish OS"));
    find(QStringLiteral("startPageTopSitesSwitch"))->setProperty("checked", false);
    QVERIFY(!settings->topSites());
    QVERIFY(!shown(QStringLiteral("startPagePreviewTopSites")));
    find(QStringLiteral("startPageBookmarksSwitch"))->setProperty("checked", false);
    QVERIFY(!settings->bookmarks());
    find(QStringLiteral("startPageRecentSwitch"))->setProperty("checked", false);
    QVERIFY(!settings->recent());
    QVERIFY(!shown(QStringLiteral("startPagePreviewBookmarks")));
    QVERIFY(!shown(QStringLiteral("startPagePreviewRecent")));
    QVERIFY(shown(QStringLiteral("startPagePreviewBar")));
    find(QStringLiteral("startPageRecentSwitch"))->setProperty("checked", true);
    QVERIFY(settings->recent());
    QVERIFY(shown(QStringLiteral("startPagePreviewRecent")));

    // Blank: the sections are dimmed, and keep their switches for when it is not; the
    // picture is empty.
    click(blank);
    QVERIFY(settings->blank());
    QVERIFY(blank->property("checked").toBool());
    QVERIFY(!sites->property("checked").toBool());
    for (const QString &name : sections) {
        QVERIFY2(!find(name)->property("enabled").toBool(), qPrintable(name));
    }
    QVERIFY(find(QStringLiteral("startPageRecentSwitch"))->property("checked").toBool());
    QVERIFY(!shown(QStringLiteral("startPagePreviewRecent")));
    QVERIFY(shown(QStringLiteral("startPagePreviewBar")));
    click(blank);
    QVERIFY(settings->blank());
    click(sites);
    QVERIFY(!settings->blank());
    QVERIFY(shown(QStringLiteral("startPagePreviewRecent")));
}

// Search: the engine the address bar searches with, every one on the page and the one
// chosen lit, and the sources its suggestions are drawn from, each a switch that is on
// until it is turned off. The way in names the engine.
void tst_qmlload::searchSettingsPage()
{
    SearchSettings *settings = m_core->searchSettings();
    openMenuItem(QStringLiteral("settingsMenuButton"));
    QObject *entry = find(QStringLiteral("searchSettingsEntry"));
    const QStringList engines = m_core->searchEngines()->engineNames();
    click(entry);
    QCOMPARE(currentPage()->objectName(), QStringLiteral("searchSettingsPage"));

    const QList<QObject *> choices = findAll(QStringLiteral("searchEngineChoice"));
    QCOMPARE(choices.count(), engines.count());
    for (int i = 0; i < choices.count(); ++i) {
        QCOMPARE(choices.at(i)->property("text").toString(), engines.at(i));
        QVERIFY(!choices.at(i)->property("automaticCheck").toBool());
        QCOMPARE(choices.at(i)->property("checked").toBool(), i == settings->engineIndex());
    }
    const int other = settings->engineIndex() == 1 ? 0 : 1;
    click(choices.at(other));
    QCOMPARE(settings->engineIndex(), other);
    for (int i = 0; i < choices.count(); ++i) {
        QCOMPARE(choices.at(i)->property("checked").toBool(), i == other);
    }

    // A switch for each source, in the order the address bar lists them, each writing
    // its own setting and no other.
    using Flag = bool (SearchSettings::*)() const;
    const QList<QPair<QString, Flag>> sources{
        {QStringLiteral("omnibarTabsSwitch"), &SearchSettings::omnibarTabs},
        {QStringLiteral("omnibarBookmarksSwitch"), &SearchSettings::omnibarBookmarks},
        {QStringLiteral("omnibarHistorySwitch"), &SearchSettings::omnibarHistory},
        {QStringLiteral("omnibarDownloadsSwitch"), &SearchSettings::omnibarDownloads},
    };
    // Each engine is a row, and the engines found while browsing are a section of their
    // own, there and hidden while there are none; the pull-down menu, which would remove
    // them, is hidden with nothing to remove.
    QStringList layout{QStringLiteral("#Search engine")};
    for (int i = 0; i < engines.count(); ++i) {
        layout.append(QStringLiteral("searchEngineRow"));
    }
    layout += QStringList{
        QStringLiteral("foundSearchEngines"),   QStringLiteral("#Address bar suggestions"),
        QStringLiteral("omnibarTabsSwitch"),    QStringLiteral("omnibarBookmarksSwitch"),
        QStringLiteral("omnibarHistorySwitch"), QStringLiteral("omnibarDownloadsSwitch"),
    };
    QCOMPARE(columnOf(findAll(QStringLiteral("searchEngineRow")).first()), layout);
    QVERIFY(!find(QStringLiteral("foundSearchEngines"))->property("visible").toBool());
    QVERIFY(!find(QStringLiteral("searchSettingsPulley"))->property("visible").toBool());
    for (QObject *row : findAll(QStringLiteral("searchEngineRow"))) {
        QVERIFY(findObjects(row, QStringLiteral("searchEngineChoice"))
                    .first()
                    ->property("description")
                    .toString()
                    .isEmpty());
    }
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

namespace {

// What ContentLinkHandler.jsm sends for a page that has a search of its own: the title and
// address of the description, and the page's own address.
void offerSearch(QObject *view, const QString &title, const QString &href, const QString &page,
                 const QString &name = QStringLiteral("Link:AddSearch"))
{
    const QVariantMap engine{{QStringLiteral("title"), title}, {QStringLiteral("href"), href}};
    const QVariantMap data{{QStringLiteral("engine"), engine}, {QStringLiteral("url"), page}};
    QMetaObject::invokeMethod(view, "recvAsyncMessage", Q_ARG(QString, name),
                              Q_ARG(QVariant, data));
}

QStringList foundTitles(const SearchEngines *list)
{
    QStringList found;
    for (const QVariant &entry : list->foundEngines()) {
        found.append(entry.toMap().value(QStringLiteral("title")).toString());
    }
    return found;
}

// A site's descriptions: two that are, and an error page where a third should be.
QMap<QString, QByteArray> descriptions()
{
    return {{QStringLiteral("/find.xml"),
             QByteArray("<OpenSearchDescription><ShortName>Find</ShortName>"
                        "<Url type=\"text/html\" template=\"https://find.example/search?q="
                        "{searchTerms}\"/></OpenSearchDescription>")},
            {QStringLiteral("/third.xml"),
             QByteArray("<OpenSearchDescription><ShortName>Third</ShortName>"
                        "<Url type=\"text/html\" template=\"https://third.example/?q="
                        "{searchTerms}\"/></OpenSearchDescription>")},
            {QStringLiteral("/broken.xml"), QByteArray("<html><body>Sign in")}};
}

QObject *choiceIn(QObject *row)
{
    return findObjects(row, QStringLiteral("searchEngineChoice")).first();
}

} // namespace

// Search engines found while browsing (docs/DECISIONS/0041-search-engines-found.md): a
// page that offers one is heard on every view and the offer kept, once, and only on the
// message that says so; Settings > Search lists what is kept.
void tst_qmlload::searchEnginesFound()
{
    SearchEngines *list = m_core->searchEngines();
    QObject *view = currentWebView();
    QVERIFY(view->property("messageListeners")
                .toStringList()
                .contains(QStringLiteral("Link:AddSearch")));

    offerSearch(view, QStringLiteral("Find"), QStringLiteral("https://find.example/find.xml"),
                QStringLiteral("https://www.find.example/page"));
    offerSearch(view, QStringLiteral("Broken"), QStringLiteral("https://broken.example/o.xml"),
                QStringLiteral("https://broken.example/"));
    offerSearch(view, QStringLiteral("Find"), QStringLiteral("https://find.example/find.xml"),
                QStringLiteral("https://www.find.example/other"));
    offerSearch(view, QStringLiteral("Qwant"), QStringLiteral("https://qwant.example/o.xml"),
                QStringLiteral("https://qwant.example/"));
    offerSearch(view, QStringLiteral("Quiet"), QStringLiteral("https://quiet.example/o.xml"),
                QStringLiteral("https://quiet.example/"), QStringLiteral("embed:find"));
    QCOMPARE(foundTitles(list), (QStringList{QStringLiteral("Find"), QStringLiteral("Broken")}));
    QCOMPARE(list->foundEngines().first().toMap().value(QStringLiteral("host")).toString(),
             QStringLiteral("find.example"));
    QCOMPARE(list->engineNames().count(), 3);

    openMenuItem(QStringLiteral("settingsMenuButton"));
    click(find(QStringLiteral("searchSettingsEntry")));
    QCOMPARE(currentPage()->objectName(), QStringLiteral("searchSettingsPage"));

    // A section of its own, with what to do with it, and a row each: the add icon, the
    // name, and under it the site and what a tap does.
    QVERIFY(find(QStringLiteral("foundSearchEngines"))->property("visible").toBool());
    QCOMPARE(find(QStringLiteral("foundSearchEnginesHint"))->property("text").toString(),
             QStringLiteral("Sites can offer their search. Tap one to add it and search with it."));
    QVERIFY(find(QStringLiteral("searchSettingsPulley"))->property("visible").toBool());
    const QList<QObject *> found = findAll(QStringLiteral("foundSearchEngine"));
    QCOMPARE(found.count(), 2);
    QCOMPARE(textIn(found.first(), QStringLiteral("foundSearchEngineName")),
             QStringLiteral("Find"));
    QCOMPARE(textIn(found.first(), QStringLiteral("foundSearchEngineHost")),
             QStringLiteral("find.example · Tap to add"));
    QCOMPARE(findObjects(found.first(), QStringLiteral("foundSearchEngineIcon"))
                 .first()
                 ->property("source")
                 .toString(),
             QStringLiteral("image://theme/icon-m-add"));
    QCOMPARE(findAll(QStringLiteral("searchEngineRow")).count(), 3);

    // Forgotten from its menu, which takes the section away with the last of them.
    click(findObjects(found.last(), QStringLiteral("foundSearchEngineForget")).first());
    QCOMPARE(foundTitles(list), QStringList{QStringLiteral("Find")});
    QCOMPARE(findAll(QStringLiteral("foundSearchEngine")).count(), 1);
    click(findObjects(findAll(QStringLiteral("foundSearchEngine")).first(),
                      QStringLiteral("foundSearchEngineForget"))
              .first());
    QVERIFY(list->foundEngines().isEmpty());
    QVERIFY(findAll(QStringLiteral("foundSearchEngine")).isEmpty());
    QVERIFY(!find(QStringLiteral("foundSearchEngines"))->property("visible").toBool());
    QVERIFY(!find(QStringLiteral("searchSettingsPulley"))->property("visible").toBool());
}

// A tap on an engine found fetches its description with the page's own XMLHttpRequest,
// from a server on the loopback, and reads it: the engine is added, chosen, no longer on
// offer, and said to be; or it is not, and stays.
void tst_qmlload::searchEnginesAdd()
{
    SearchSettings *search = m_core->searchSettings();
    SearchEngines *list = m_core->searchEngines();
    QObject *view = currentWebView();
    DescriptionServer server(descriptions());
    QVERIFY(server.isListening());
    offerSearch(view, QStringLiteral("Find"), server.url(QStringLiteral("/find.xml")),
                QStringLiteral("https://www.find.example/page"));
    offerSearch(view, QStringLiteral("Broken"), server.url(QStringLiteral("/broken.xml")),
                QStringLiteral("https://broken.example/"));
    openMenuItem(QStringLiteral("settingsMenuButton"));
    click(find(QStringLiteral("searchSettingsEntry")));

    QObject *notice = find(QStringLiteral("searchEngineNotice"));
    QCOMPARE(notice->property("shownCount").toInt(), 0);
    click(findAll(QStringLiteral("foundSearchEngine")).first());
    QTRY_COMPARE(notice->property("shownCount").toInt(), 1);
    QCOMPARE(notice->property("shownText").toString(), QStringLiteral("Find search added"));
    QCOMPARE(server.requests(), 1);
    QCOMPARE(list->engineNames().last(), QStringLiteral("Find"));
    QCOMPARE(search->engineIndex(), 3);
    QCOMPARE(search->searchUrl(QStringLiteral("a b")),
             QStringLiteral("https://find.example/search?q=a%20b"));
    QCOMPARE(foundTitles(list), QStringList{QStringLiteral("Broken")});
    QCOMPARE(findAll(QStringLiteral("foundSearchEngine")).count(), 1);

    // Listed with the others, built-in first, the site it came from under it, and lit as
    // the one in use; the built-in ones have no such line.
    const QList<QObject *> rows = findAll(QStringLiteral("searchEngineRow"));
    QCOMPARE(rows.count(), 4);
    QCOMPARE(choiceIn(rows.last())->property("text").toString(), QStringLiteral("Find"));
    QCOMPARE(choiceIn(rows.last())->property("description").toString(),
             QStringLiteral("Added from find.example"));
    QVERIFY(choiceIn(rows.last())->property("checked").toBool());
    QVERIFY(!choiceIn(rows.first())->property("checked").toBool());
    QVERIFY(choiceIn(rows.first())->property("description").toString().isEmpty());

    // One that is not a description is not added, and stays to be tried or forgotten.
    const QList<QObject *> found = findAll(QStringLiteral("foundSearchEngine"));
    click(found.first());
    QTRY_COMPARE(notice->property("shownCount").toInt(), 2);
    QCOMPARE(notice->property("shownText").toString(), QStringLiteral("Could not add Broken"));
    QCOMPARE(foundTitles(list), QStringList{QStringLiteral("Broken")});
    QCOMPARE(list->engineNames().count(), 4);
    QCOMPARE(search->engineIndex(), 3);

    // A second tap while the first is on its way is not a second fetch.
    const int requests = server.requests();
    click(found.first());
    click(found.first());
    QTRY_COMPARE(notice->property("shownCount").toInt(), 3);
    QCOMPARE(server.requests(), requests + 1);
    QTest::qWait(50);
    QCOMPARE(notice->property("shownCount").toInt(), 3);
}

// An added engine is removed from a menu opened by pressing and holding it, or with every
// other from the pull-down menu, after its remorse; the first built-in engine is the one
// in use if the one removed was.
void tst_qmlload::searchEnginesRemove()
{
    SearchSettings *search = m_core->searchSettings();
    SearchEngines *list = m_core->searchEngines();
    QObject *view = currentWebView();
    DescriptionServer server(descriptions());
    QVERIFY(server.isListening());
    offerSearch(view, QStringLiteral("Find"), server.url(QStringLiteral("/find.xml")),
                QStringLiteral("https://find.example/"));
    openMenuItem(QStringLiteral("settingsMenuButton"));
    click(find(QStringLiteral("searchSettingsEntry")));
    click(findAll(QStringLiteral("foundSearchEngine")).first());
    QTRY_COMPARE(list->addedCount(), 1);
    QObject *remove = find(QStringLiteral("removeAddedEnginesMenu"));
    QVERIFY(remove->property("visible").toBool());

    // A built-in engine has no menu to open, an added one has.
    QList<QObject *> rows = findAll(QStringLiteral("searchEngineRow"));
    QMetaObject::invokeMethod(choiceIn(rows.first()), "pressAndHold");
    QVERIFY(!rows.first()->property("menuOpen").toBool());
    QMetaObject::invokeMethod(choiceIn(rows.last()), "pressAndHold");
    QVERIFY(rows.last()->property("menuOpen").toBool());
    click(findObjects(rows.last(), QStringLiteral("searchEngineRemove")).first());
    QCOMPARE(list->engineNames().count(), 3);
    QCOMPARE(search->engineIndex(), 0);
    rows = findAll(QStringLiteral("searchEngineRow"));
    QCOMPARE(rows.count(), 3);
    QVERIFY(choiceIn(rows.first())->property("checked").toBool());
    QVERIFY(!remove->property("visible").toBool());

    // What was added and what was found, gone together.
    offerSearch(view, QStringLiteral("Third"), server.url(QStringLiteral("/third.xml")),
                QStringLiteral("https://third.example/"));
    offerSearch(view, QStringLiteral("Find"), server.url(QStringLiteral("/find.xml")),
                QStringLiteral("https://find.example/"));
    QVERIFY(remove->property("visible").toBool());
    click(findAll(QStringLiteral("foundSearchEngine")).first());
    QTRY_COMPARE(list->addedCount(), 1);
    QCOMPARE(list->engineNames().last(), QStringLiteral("Third"));
    QCOMPARE(foundTitles(list), QStringList{QStringLiteral("Find")});
    const int remorses = evaluate(currentPage(), QStringLiteral("Remorse.popupCount")).toInt();
    click(remove);
    QCOMPARE(evaluate(currentPage(), QStringLiteral("Remorse.popupCount")).toInt(), remorses + 1);
    QCOMPARE(evaluate(currentPage(), QStringLiteral("Remorse.popupText")).toString(),
             QStringLiteral("Removing added search engines"));
    QCOMPARE(list->engineNames().count(), 3);
    QVERIFY(list->foundEngines().isEmpty());
    QCOMPARE(search->engineIndex(), 0);
    QVERIFY(!remove->property("visible").toBool());
    QVERIFY(!find(QStringLiteral("foundSearchEngines"))->property("visible").toBool());

    // The main page's line follows the engine in use, an added one as any.
    offerSearch(view, QStringLiteral("Third"), server.url(QStringLiteral("/third.xml")),
                QStringLiteral("https://third.example/"));
    click(findAll(QStringLiteral("foundSearchEngine")).first());
    QTRY_COMPARE(list->engineNames().last(), QStringLiteral("Third"));
    popPage();
    QCOMPARE(
        textIn(find(QStringLiteral("searchSettingsEntry")), QStringLiteral("settingsEntryValue")),
        QStringLiteral("Third"));
}

// The reader view's look: each colour a square painted as the reader view will be and
// each typeface a tile written in it, the one set lit, Firefox's middle text size written
// as the whole of itself, and the way in says all three in a line.
void tst_qmlload::readerSettingsPage()
{
    openMenuItem(QStringLiteral("settingsMenuButton"));
    QObject *entry = find(QStringLiteral("readerSettingsEntry"));
    click(entry);
    QCOMPARE(currentPage()->objectName(), QStringLiteral("readerSettingsPage"));

    // Under the choices, a few lines of an article as the reader view will set them, in
    // the ambience's own look to begin with -- its colours and typeface, the heading
    // first and at the end of the line -- and at the reader view's own size.
    QObject *preview = find(QStringLiteral("readerPreview"));
    QObject *domain = find(QStringLiteral("readerPreviewDomain"));
    QObject *heading = find(QStringLiteral("readerPreviewHeading"));
    QObject *text = find(QStringLiteral("readerPreviewText"));
    const qreal zoom =
        Settings::pageZoom(evaluate(preview, QStringLiteral("Theme.pixelRatio")).toReal());
    const auto fontOf = [](QObject *label) { return label->property("font").value<QFont>(); };
    const auto sizedFor = [zoom, &fontOf](QObject *label, int step) {
        return qAbs(fontOf(label).pixelSize() - Reader::fontSizeFor(step) * zoom) <= 1;
    };
    const auto theme = [this, preview](const char *name) {
        return evaluate(preview, QStringLiteral("Theme.") + QLatin1String(name)).value<QColor>();
    };
    QCOMPARE(columnOf(preview).last(), QStringLiteral("readerPreview"));
    QVERIFY(preview->property("ambience").toBool());
    QCOMPARE(preview->property("color").value<QColor>(), theme("highlightDimmerColor"));
    QCOMPARE(text->property("color").value<QColor>(), theme("primaryColor"));
    QCOMPARE(heading->property("color").value<QColor>(), theme("highlightColor"));
    QCOMPARE(domain->property("color").value<QColor>(), theme("secondaryHighlightColor"));
    QCOMPARE(heading->property("horizontalAlignment").toInt(), int(Qt::AlignRight));
    QVERIFY(heading->property("y").toReal() < domain->property("y").toReal());
    QCOMPARE(fontOf(text).family(),
             evaluate(preview, QStringLiteral("Theme.fontFamily")).toString());
    QVERIFY(sizedFor(text, ReaderSettings::TextSizeDefault));

    // Five squares, in the order a reader reads them, each carrying its stored value.
    const QList<QObject *> swatches = findAll(QStringLiteral("readerColorsChoice"));
    const QList<int> order{ReaderSettings::Automatic, ReaderSettings::Ambience,
                           ReaderSettings::Light, ReaderSettings::Sepia, ReaderSettings::Dark};
    const QStringList names{QStringLiteral("Automatic"), QStringLiteral("Ambience"),
                            QStringLiteral("Light"), QStringLiteral("Sepia"),
                            QStringLiteral("Dark")};
    QCOMPARE(swatches.count(), order.count());
    for (int i = 0; i < swatches.count(); ++i) {
        QCOMPARE(swatches.at(i)->property("colors").toInt(), order.at(i));
        QCOMPARE(swatches.at(i)->property("text").toString(), names.at(i));
        QCOMPARE(swatches.at(i)->property("selected").toBool(),
                 order.at(i) == int(ReaderSettings::Ambience));
    }
    QCOMPARE(m_core->readerSettings()->colors(), int(ReaderSettings::Ambience));
    // Each painted as its theme: Sepia's square is the style sheet's sepia.
    QObject *sepiaSquare =
        findObjects(swatches.at(3), QStringLiteral("readerSwatchSquare")).first();
    QCOMPARE(sepiaSquare->property("color").value<QColor>(),
             Reader::backgroundOf(QStringLiteral("sepia")));

    click(swatches.at(3));
    QCOMPARE(m_core->readerSettings()->colors(), int(ReaderSettings::Sepia));
    QVERIFY(swatches.at(3)->property("selected").toBool());
    QVERIFY(!swatches.at(1)->property("selected").toBool());
    QVERIFY(!preview->property("ambience").toBool());
    QCOMPARE(preview->property("color").value<QColor>(),
             Reader::backgroundOf(QStringLiteral("sepia")));
    QCOMPARE(text->property("color").value<QColor>(), Reader::textColorOf(QStringLiteral("sepia")));
    QCOMPARE(domain->property("color").value<QColor>(),
             Reader::linkColorOf(QStringLiteral("sepia")));
    // Firefox's order again: the site, then the heading.
    QVERIFY(heading->property("y").toReal() > domain->property("y").toReal());
    QCOMPARE(fontOf(text).family(), QStringLiteral("sans-serif"));
    // Automatic is the ambience's light or dark: the stub's is dark.
    click(swatches.at(0));
    QCOMPARE(m_core->readerSettings()->colors(), int(ReaderSettings::Automatic));
    QCOMPARE(preview->property("color").value<QColor>(),
             Reader::backgroundOf(QStringLiteral("dark")));
    click(swatches.at(3));

    // The typefaces, each a tile written in itself.
    const QList<QObject *> typefaces = findAll(QStringLiteral("readerTypefaceChoice"));
    QCOMPARE(typefaces.count(), 2);
    QCOMPARE(textIn(typefaces.at(0), QStringLiteral("readerTypefaceName")),
             QStringLiteral("Sans serif"));
    QCOMPARE(textIn(typefaces.at(1), QStringLiteral("readerTypefaceName")),
             QStringLiteral("Serif"));
    QVERIFY(typefaces.at(0)->property("selected").toBool());
    QCOMPARE(m_core->readerSettings()->typeface(), int(ReaderSettings::SansSerif));
    click(typefaces.at(1));
    QCOMPARE(m_core->readerSettings()->typeface(), int(ReaderSettings::Serif));
    QVERIFY(typefaces.at(1)->property("selected").toBool());
    QVERIFY(!typefaces.at(0)->property("selected").toBool());
    // The article in the typeface chosen, the site's name in the sans-serif still.
    QCOMPARE(fontOf(text).family(), QStringLiteral("serif"));
    QCOMPARE(fontOf(domain).family(), QStringLiteral("sans-serif"));
    QObject *readerSize = find(QStringLiteral("readerTextSizeSlider"));
    QCOMPARE(readerSize->property("minimumValue").toInt(), int(ReaderSettings::TextSizeMin));
    QCOMPARE(readerSize->property("maximumValue").toInt(), int(ReaderSettings::TextSizeMax));
    QCOMPARE(readerSize->property("value").toInt(), int(ReaderSettings::TextSizeDefault));
    QCOMPARE(readerSize->property("valueText").toString(), QStringLiteral("100 %"));
    readerSize->setProperty("value", 9);
    QCOMPARE(m_core->readerSettings()->textSize(), 9);
    QCOMPARE(readerSize->property("valueText").toString(), QStringLiteral("140 %"));
    QVERIFY(sizedFor(text, 9));
    readerSize->setProperty("value", 1);
    QCOMPARE(readerSize->property("valueText").toString(), QStringLiteral("60 %"));
    QVERIFY(sizedFor(text, 1));

    // The picture follows the setting wherever it is written from, and so do the
    // squares.
    m_core->readerSettings()->setColors(ReaderSettings::Light);
    QCOMPARE(preview->property("color").value<QColor>(),
             Reader::backgroundOf(QStringLiteral("light")));
    QVERIFY(swatches.at(2)->property("selected").toBool());
    m_core->readerSettings()->setColors(ReaderSettings::Dark);
    QCOMPARE(domain->property("color").value<QColor>(),
             Reader::linkColorOf(QStringLiteral("dark")));
    // In the ambience's look a serif article keeps its serif, and the heading is set in
    // it too.
    m_core->readerSettings()->setColors(ReaderSettings::Ambience);
    QCOMPARE(fontOf(text).family(), QStringLiteral("serif"));
    QCOMPARE(fontOf(heading).family(), QStringLiteral("serif"));
}

// Tracking protection: its three levels on the page at once, each saying what it does,
// the one set lit; a change reaches the engine at once. The way in names the level.
void tst_qmlload::trackingSettingsPage()
{
    openMenuItem(QStringLiteral("settingsMenuButton"));
    QObject *entry = find(QStringLiteral("trackingSettingsEntry"));
    click(entry);
    QObject *page = currentPage();
    QCOMPARE(page->objectName(), QStringLiteral("trackingSettingsPage"));

    // Standard until it is changed here, and a change reaches the engine at once, every
    // preference of the new level after the old ones. The browsing page writes them, so
    // they are read through something of BrowserPage.qml's own.
    QObject *pageScope = find(QStringLiteral("viewArea"));
    const QList<QObject *> levels = findAll(QStringLiteral("trackingProtectionChoice"));
    QCOMPARE(levels.count(), 3);
    const QStringList names{QStringLiteral("Off"), QStringLiteral("Standard"),
                            QStringLiteral("Strict")};
    QStringList descriptions;
    for (int level = 0; level < levels.count(); ++level) {
        QObject *choice = levels.at(level);
        QCOMPARE(choice->property("text").toString(), names.at(level));
        QVERIFY(!choice->property("automaticCheck").toBool());
        QCOMPARE(choice->property("checked").toBool(),
                 level == int(PrivacySettings::TrackingProtectionStandard));
        const QString description = choice->property("description").toString();
        QVERIFY2(!description.isEmpty(), qPrintable(names.at(level)));
        QVERIFY(!descriptions.contains(description));
        descriptions.append(description);
    }
    const int given =
        evaluate(pageScope, QStringLiteral("WebEngineSettings.preferences.length")).toInt();
    click(levels.at(PrivacySettings::TrackingProtectionStrict));
    QCOMPARE(m_core->privacySettings()->trackingProtection(),
             int(PrivacySettings::TrackingProtectionStrict));
    QVERIFY(levels.at(PrivacySettings::TrackingProtectionStrict)->property("checked").toBool());
    QVERIFY(!levels.at(PrivacySettings::TrackingProtectionStandard)->property("checked").toBool());
    const QVariantList strict = EngineMessages::trackingProtectionPreferences(
        PrivacySettings::TrackingProtectionStrict, SitePermissionSettings::CookiesBlockCrossSite);
    const QVariantList preferences =
        evaluate(pageScope, QStringLiteral("WebEngineSettings.preferences")).toList();
    QCOMPARE(preferences.count(), given + strict.count());
    for (int i = 0; i < strict.count(); ++i) {
        QCOMPARE(preferences.at(given + i).toMap().value(QStringLiteral("key")),
                 strict.at(i).toMap().value(QStringLiteral("name")));
        QCOMPARE(preferences.at(given + i).toMap().value(QStringLiteral("value")),
                 strict.at(i).toMap().value(QStringLiteral("value")));
    }
    click(levels.at(PrivacySettings::TrackingProtectionOff));
    QCOMPARE(m_core->privacySettings()->trackingProtection(),
             int(PrivacySettings::TrackingProtectionOff));
    QCOMPARE(evaluate(pageScope, QStringLiteral("WebEngineSettings.preferences.length")).toInt(),
             given + 2 * strict.count());
    // The same level again tells the engine nothing.
    click(levels.at(PrivacySettings::TrackingProtectionOff));
    QCOMPARE(evaluate(pageScope, QStringLiteral("WebEngineSettings.preferences.length")).toInt(),
             given + 2 * strict.count());

    // Tracking protection is all there is here: clearing has a page of its own.
    // Under the levels, once, what none of them can promise.
    const QString choice = QStringLiteral("trackingProtectionChoice");
    QCOMPARE(columnOf(levels.first()),
             (QStringList{choice, choice, choice, QStringLiteral("trackingProtectionLimits")}));
    QVERIFY(find(QStringLiteral("trackingProtectionLimits"))
                ->property("text")
                .toString()
                .contains(QStringLiteral("some trackers may still get through")));
}

// History: whether pages are kept, and whether they go as the browser closes, each a
// switch writing its setting; what is kept, counted; and the button to clear browsing
// data, which asks first. The way in says whether and for how long the history is kept.
void tst_qmlload::historySettingsPage()
{
    PrivacySettings *settings = m_core->privacySettings();
    TabModel *tabs = m_core->tabs();
    openMenuItem(QStringLiteral("settingsMenuButton"));
    // What Settings asked of the engine as it opened (settingsPage()) is not this page's.
    forgetStartupMessages();
    QObject *entry = find(QStringLiteral("historySettingsEntry"));
    click(entry);
    QObject *page = currentPage();
    QCOMPARE(page->objectName(), QStringLiteral("historySettingsPage"));
    QObject *remember = find(QStringLiteral("rememberHistorySwitch"));
    QObject *onClose = find(QStringLiteral("clearHistoryOnCloseSwitch"));
    QCOMPARE(columnOf(remember),
             (QStringList{QStringLiteral("rememberHistorySwitch"),
                          QStringLiteral("clearHistoryOnCloseSwitch"),
                          QStringLiteral("#Kept on this phone"), QStringLiteral("keptHistory"),
                          QStringLiteral("keptDownloads"), QStringLiteral("keptClosedTabs"),
                          QStringLiteral("keptOpenTabs"), QStringLiteral("clearDataButton")}));
    // Remembering needs no line; clearing on close says what goes with it.
    QVERIFY(remember->property("description").toString().isEmpty());
    QCOMPARE(onClose->property("description").toString(),
             QStringLiteral("With it, the list of downloads and the recently closed tabs"));

    // What is kept, each counted as it changes: the one page the tests start on, no
    // downloads, nothing closed, the one tab in the one group.
    const auto kept = [this](const QString &detail) {
        return find(detail)->property("value").toString();
    };
    QCOMPARE(find(QStringLiteral("keptHistory"))->property("label").toString(),
             QStringLiteral("History"));
    QCOMPARE(kept(QStringLiteral("keptHistory")), QStringLiteral("1 page(s)"));
    QCOMPARE(kept(QStringLiteral("keptDownloads")), QStringLiteral("0 file(s)"));
    QCOMPARE(kept(QStringLiteral("keptClosedTabs")), QStringLiteral("0 tab(s)"));
    QCOMPARE(kept(QStringLiteral("keptOpenTabs")), QStringLiteral("1 tab(s)"));
    tabs->newTab(QStringLiteral("https://closed.example/"));
    tabs->closeTab(tabs->activeTabIndex());
    QCOMPARE(kept(QStringLiteral("keptHistory")), QStringLiteral("2 page(s)"));
    QCOMPARE(kept(QStringLiteral("keptClosedTabs")), QStringLiteral("1 tab(s)"));
    tabs->groupModel()->addGroup(QStringLiteral("Work"));
    QCOMPARE(kept(QStringLiteral("keptOpenTabs")), QStringLiteral("1, in 2 group(s)"));

    // Kept to begin with; switched off, a page visited is not, and what was kept stays.
    QVERIFY(remember->property("checked").toBool());
    tabs->newTab(QStringLiteral("https://kept.example/"));
    const int visits = m_core->history()->count();
    QVERIFY(visits > 0);
    remember->setProperty("checked", false);
    QVERIFY(!settings->rememberHistory());
    tabs->newTab(QStringLiteral("https://unkept.example/"));
    QCOMPARE(m_core->history()->count(), visits);
    remember->setProperty("checked", true);
    QVERIFY(settings->rememberHistory());

    // Cleared as the browser closes only once that is switched on; the line says so.
    QVERIFY(!onClose->property("checked").toBool());
    m_core->clearOnClose();
    QCOMPARE(m_core->history()->count(), visits);
    onClose->setProperty("checked", true);
    QVERIFY(settings->clearHistoryOnClose());
    m_core->clearOnClose();
    QCOMPARE(m_core->history()->count(), 0);
    onClose->setProperty("checked", false);

    // Clearing browsing data is a way in of its own, to a dialog that asks which kinds
    // (clearDataDialog()); backed out of, it clears nothing.
    tabs->newTab(QStringLiteral("https://again.example/"));
    const int again = m_core->history()->count();
    QVERIFY(again > 0);
    const int remorses = evaluate(page, QStringLiteral("Remorse.popupCount")).toInt();
    QObject *clear = find(QStringLiteral("clearDataButton"));
    QCOMPARE(clear->property("text").toString(), QStringLiteral("Clear browsing data"));
    click(clear);
    QCOMPARE(currentPage()->objectName(), QStringLiteral("clearDataDialog"));
    popPage();
    QCOMPARE(currentPage(), page);
    QCOMPARE(evaluate(page, QStringLiteral("Remorse.popupCount")).toInt(), remorses);
    QCOMPARE(evaluate(page, QStringLiteral("WebEngine.notifications.length")).toInt(), 0);
    QCOMPARE(m_core->history()->count(), again);
}

// Clear browsing data asks how far back and which kinds in a dialog -- everything, the
// open tabs off to begin with, the rest on, and Clear dimmed while none is -- and what
// it is accepted with is cleared under one remorse on the history page, each kind as
// its own button used to clear it.
void tst_qmlload::clearDataDialog()
{
    TabModel *tabs = m_core->tabs();
    tabs->newTab(QStringLiteral("https://two.example/"));
    QVERIFY(m_core->history()->count() > 0);
    openMenuItem(QStringLiteral("settingsMenuButton"));
    // What Settings asked of the engine as it opened (settingsPage()) is not this page's.
    forgetStartupMessages();
    click(find(QStringLiteral("historySettingsEntry")));
    QObject *privacy = currentPage();
    QCOMPARE(privacy->objectName(), QStringLiteral("historySettingsPage"));
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
        click(find(QStringLiteral("clearDataButton")));
        QObject *dialog = currentPage();
        QCOMPARE(dialog->objectName(), QStringLiteral("clearDataDialog"));
        for (int i = 0; i < switches.count(); ++i) {
            find(switches.at(i))->setProperty("checked", on.at(i));
        }
        QMetaObject::invokeMethod(dialog, "accept");
        popPage();
    };

    // The dialog as it opens, in Firefox's order: all but the open tabs on.
    click(find(QStringLiteral("clearDataButton")));
    QObject *dialog = currentPage();
    QCOMPARE(dialog->objectName(), QStringLiteral("clearDataDialog"));
    const QList<bool> initially{false, true, true, true};
    QCOMPARE(columnOf(find(switches.first())),
             QStringList{QStringLiteral("clearRangeCombo")} + switches);
    QObject *range = find(QStringLiteral("clearRangeCombo"));
    QCOMPARE(range->property("currentIndex").toInt(), int(Salama::HistoryModel::ClearEverything));
    QVERIFY(range->property("description").toString().isEmpty());
    range->setProperty("currentIndex", int(Salama::HistoryModel::ClearLastHour));
    QVERIFY(!range->property("description").toString().isEmpty());
    range->setProperty("currentIndex", int(Salama::HistoryModel::ClearEverything));
    for (int i = 0; i < switches.count(); ++i) {
        QObject *toggle = find(switches.at(i));
        QVERIFY2(!toggle->property("text").toString().isEmpty(), qPrintable(switches.at(i)));
        QCOMPARE(toggle->property("checked").toBool(), initially.at(i));
    }
    QVERIFY(dialog->property("canAccept").toBool());

    // Under each kind that can be counted, how much of it goes; under the cookies, what
    // their going does; the cache is said by its name.
    const auto said = [this](const QString &name) {
        return find(name)->property("description").toString();
    };
    QCOMPARE(said(QStringLiteral("clearTabsSwitch")), QStringLiteral("2 tab(s), in every group"));
    QCOMPARE(said(QStringLiteral("clearHistorySwitch")), QStringLiteral("2 page(s)"));
    QCOMPARE(said(QStringLiteral("clearSiteDataSwitch")),
             QStringLiteral("Signs you out of most sites"));
    QVERIFY(said(QStringLiteral("clearCacheSwitch")).isEmpty());
    // The closed tabs go with the whole history, and are counted while it is.
    tabs->closeTab(tabs->activeTabIndex());
    QCOMPARE(said(QStringLiteral("clearTabsSwitch")), QStringLiteral("1 tab(s), in every group"));
    QCOMPARE(said(QStringLiteral("clearHistorySwitch")),
             QStringLiteral("2 page(s) and 1 closed tab(s)"));
    QCOMPARE(dialog->property("closedTabs").toInt(), 1);
    range->setProperty("currentIndex", int(Salama::HistoryModel::ClearLastHour));
    QCOMPARE(dialog->property("closedTabs").toInt(), 0);
    QCOMPARE(said(QStringLiteral("clearHistorySwitch")), QStringLiteral("2 page(s)"));
    range->setProperty("currentIndex", int(Salama::HistoryModel::ClearEverything));
    QCOMPARE(evaluate(dialog, QStringLiteral("historyText(3, 18, 6)")).toString(),
             QStringLiteral("3 page(s), 18 download(s) and 6 closed tab(s)"));
    QCOMPARE(evaluate(dialog, QStringLiteral("historyText(0, 0, 0)")).toString(),
             QStringLiteral("Nothing from this time"));
    tabs->newTab(QStringLiteral("https://two.example/"));

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

    // The open tabs alone: every one closed, and the browsing page opens the start page
    // in their place.
    clear({true, false, false, false});
    QCOMPARE(remorses(), 4);
    QCOMPARE(tabs->count(), 1);
    QVERIFY(tabs->activeUrl().isEmpty());
    QVERIFY(m_core->history()->count() >= visits);
    QCOMPARE(sent(), 2);
    QCOMPARE(currentPage(), privacy);

    // The history of the last hour alone: an old visit stays, the recent ones go, and
    // so do the downloads of that hour; the recently closed tabs stay, going only with
    // the whole history.
    {
        QSqlQuery old(m_core->storage().database());
        QVERIFY(old.exec(QStringLiteral("INSERT INTO browser_history (url, title, date) "
                                        "VALUES ('https://old.example/', 'Old', 5)")));
        m_core->history()->setSearchTerm(QString());
        m_core->history()->setSearchTerm(QStringLiteral("old.example"));
        QCOMPARE(m_core->history()->count(), 1);
        m_core->history()->setSearchTerm(QString());
    }
    tabs->newTab(QStringLiteral("https://recent.example/"));
    const int closedBefore = tabs->closedTabs()->count();
    QVERIFY(closedBefore > 0);
    click(find(QStringLiteral("clearDataButton")));
    find(QStringLiteral("clearRangeCombo"))
        ->setProperty("currentIndex", int(Salama::HistoryModel::ClearLastHour));
    for (int i = 0; i < switches.count(); ++i) {
        find(switches.at(i))->setProperty("checked", i == 1);
    }
    QMetaObject::invokeMethod(currentPage(), "accept");
    popPage();
    QCOMPARE(remorses(), 5);
    QCOMPARE(m_core->history()->count(), 1);
    QCOMPARE(
        m_core->history()
            ->data(m_core->history()->index(0, 0), Salama::roleId(Salama::HistoryModel::Role::Url))
            .toString(),
        QStringLiteral("https://old.example/"));
    QCOMPARE(tabs->closedTabs()->count(), closedBefore);

    // All four together are still one remorse. The tabs go first, so the history
    // cleared after them does not keep the page that took their place.
    tabs->newTab(QStringLiteral("https://four.example/"));
    clear({true, true, true, true});
    QCOMPARE(remorses(), 6);
    QCOMPARE(tabs->count(), 1);
    QVERIFY(tabs->activeUrl().isEmpty());
    QCOMPARE(m_core->history()->count(), 0);
    QCOMPARE(tabs->closedTabs()->count(), 0);
    QCOMPARE(sent(), 4);
    QCOMPARE(notification(2, QStringLiteral("value")), QStringLiteral("cookies-and-site-data"));
    QCOMPARE(notification(3, QStringLiteral("value")), QStringLiteral("cache"));
}

// The cover's quick action, a row for each with the glyph it wears, under a picture of
// the cover with the action on it. The way in says what the action is, in the words the
// choices are offered in.
void tst_qmlload::coverSettingsPage()
{
    openMenuItem(QStringLiteral("settingsMenuButton"));
    QObject *entry = find(QStringLiteral("coverSettingsEntry"));
    click(entry);
    QObject *page = currentPage();
    QCOMPARE(page->objectName(), QStringLiteral("coverSettingsPage"));

    QObject *preview = find(QStringLiteral("coverPreview"));
    const QStringList layout{
        QStringLiteral("coverPreview"),         QStringLiteral("#Quick action"),
        QStringLiteral("quickActionExplained"), QStringLiteral("quickAction-none"),
        QStringLiteral("quickAction-search"),   QStringLiteral("quickAction-bookmarks"),
        QStringLiteral("quickAction-bookmark"), QStringLiteral("quickAction-downloads"),
        QStringLiteral("quickAction-history"),  QStringLiteral("quickActionIconPicker"),
    };
    QCOMPARE(columnOf(preview), layout);

    // The picture is a real cover's shape, two thirds its size and centred, the cover
    // with nothing to say: the halftone alone, whole.
    const qreal coverWidth = evaluate(page, QStringLiteral("Theme.coverSizeLarge.width")).toReal();
    const qreal coverHeight =
        evaluate(page, QStringLiteral("Theme.coverSizeLarge.height")).toReal();
    auto *previewItem = qobject_cast<QQuickItem *>(preview);
    QCOMPARE(previewItem->width(), coverWidth * 2 / 3);
    QCOMPARE(previewItem->x() + previewItem->width() / 2, previewItem->parentItem()->width() / 2);
    auto *picture =
        qobject_cast<QQuickItem *>(findObjects(preview, QStringLiteral("previewPicture")).first());
    QCOMPARE(picture->height(), qreal(qRound(picture->width() * coverHeight / coverWidth)));
    QObject *halftone = findObjects(preview, QStringLiteral("previewHalftone")).first();
    QVERIFY(halftone->property("visible").toBool());
    QCOMPARE(halftone->property("strength").toReal(), 1.0);

    // Why there is one action, in the voice of a hint rather than a control: small, in
    // the secondary highlight, and the words of a sentence rather than rich text.
    QObject *explained = find(QStringLiteral("quickActionExplained"));
    QCOMPARE(explained->property("text").toString(),
             QStringLiteral("The cover on the home screen offers one action. While a tab "
                            "plays, its mute button sits beside it."));
    QCOMPARE(explained->property("textFormat").toInt(), int(Qt::PlainText));
    QCOMPARE(explained->property("wrapMode").toInt(),
             evaluate(explained, QStringLiteral("Text.Wrap")).toInt());
    QCOMPARE(explained->property("font").value<QFont>().pixelSize(),
             evaluate(page, QStringLiteral("Theme.fontSizeExtraSmall")).toInt());
    QCOMPARE(explained->property("color"),
             evaluate(page, QStringLiteral("Theme.secondaryHighlightColor")));

    // The rows: the one set lit, a small item tall, with no line under it but the
    // bookmark's once one is picked.
    QObject *search = find(QStringLiteral("quickAction-search"));
    QVERIFY(search->property("chosen").toBool());
    QCOMPARE(search->property("height"), evaluate(page, QStringLiteral("Theme.itemSizeSmall")));
    QCOMPARE(findObjects(search, QStringLiteral("quickActionName")).first()->property("color"),
             evaluate(page, QStringLiteral("Theme.highlightColor")));
    QObject *history = find(QStringLiteral("quickAction-history"));
    QVERIFY(!history->property("chosen").toBool());
    QCOMPARE(findObjects(history, QStringLiteral("quickActionName")).first()->property("color"),
             evaluate(page, QStringLiteral("Theme.primaryColor")));
    QVERIFY(!findObjects(find(QStringLiteral("quickAction-bookmark")),
                         QStringLiteral("quickActionDetail"))
                 .first()
                 ->property("visible")
                 .toBool());
}

namespace {

// The cover's own items, found by name.
QObject *coverPart(QObject *coverItem, const QString &name)
{
    return coverItem->findChild<QObject *>(name);
}

bool shown(QObject *coverItem, const QString &name)
{
    return coverPart(coverItem, name)->property("visible").toBool();
}

// Which of the cover's views is up, as the reader sees it: where they were, the
// downloads, what plays, or none -- the halftone alone.
QString coverView(QObject *coverItem)
{
    QStringList up;
    for (const QString &view : {QStringLiteral("coverPlace"), QStringLiteral("coverDownloads"),
                                QStringLiteral("coverMedia")}) {
        if (shown(coverItem, view)) {
            up.append(view);
        }
    }
    return up.join(QLatin1Char(' '));
}

qreal halftoneStrength(QObject *coverItem)
{
    return coverPart(coverItem, QStringLiteral("coverHalftone"))->property("strength").toReal();
}

} // namespace

// At rest, the cover says where the reader was: the site and title of the tab in front,
// over the faint halftone, the bolt in the cover's middle
// (docs/DECISIONS/0037-cover-is-where-you-were.md). Its quick action is a search until
// another is chosen.
void tst_qmlload::cover()
{
    auto *coverItem = m_window->property("coverItem").value<QObject *>();
    QVERIFY(coverItem != nullptr);
    // As large as the home screen draws it, so there is a layout to measure.
    coverItem->setProperty("width", 234);
    coverItem->setProperty("height", 374);
    TabModel *tabs = m_core->tabs();
    const int front = tabs->activeTabId();

    QCOMPARE(coverView(coverItem), QStringLiteral("coverPlace"));
    QVERIFY(shown(coverItem, QStringLiteral("coverHalftone")));
    QVERIFY(halftoneStrength(coverItem) < 0.2);
    const auto text = [coverItem](const QString &name) {
        return coverPart(coverItem, name)->property("text").toString();
    };
    QCOMPARE(text(QStringLiteral("coverPlaceHost")), QStringLiteral("qwant.com"));
    // A page with no title yet is named by its site.
    QCOMPARE(text(QStringLiteral("coverPlaceTitle")), QStringLiteral("qwant.com"));
    // Nothing under the title: it has the room down to the actions.
    QVERIFY(coverPart(coverItem, QStringLiteral("coverPlaceCount")) == nullptr);
    QVERIFY(coverPart(coverItem, QStringLiteral("coverPlaceGroup")) == nullptr);
    auto *place = qobject_cast<QQuickItem *>(coverPart(coverItem, QStringLiteral("coverPlace")));
    auto *title =
        qobject_cast<QQuickItem *>(coverPart(coverItem, QStringLiteral("coverPlaceTitle")));
    QCOMPARE(title->y() + title->height(), place->height());
    // No icon known: the site's first letter.
    QCOMPARE(text(QStringLiteral("coverPlaceLetter")), QStringLiteral("Q"));
    QVERIFY(shown(coverItem, QStringLiteral("coverPlaceLetter")));

    // It follows the tab in front: its title, its site.
    tabs->updateTitle(front, QStringLiteral("Catatumbo lightning"));
    QCOMPARE(text(QStringLiteral("coverPlaceTitle")), QStringLiteral("Catatumbo lightning"));
    const int second = tabs->newTab(QStringLiteral("https://yle.fi/uutiset"));
    QCOMPARE(text(QStringLiteral("coverPlaceHost")), QStringLiteral("yle.fi"));
    tabs->closeTabById(second);

    // The halftone is a picture the cover has on disk, installed beside the QML. It fills
    // the cover and is cut to it top and bottom alike, being taller than a cover: the
    // bolt, in the picture's middle, is in the cover's.
    auto *halftone =
        qobject_cast<QQuickItem *>(coverPart(coverItem, QStringLiteral("coverHalftone")));
    QCOMPARE(halftone->width(), 234.0);
    QCOMPARE(halftone->height(), 374.0);
    QObject *dots = coverPart(coverItem, QStringLiteral("coverHalftoneDots"));
    const QUrl source = dots->property("source").toUrl();
    QVERIFY(source.toString().endsWith(QLatin1String("art/cover/halftone.png")));
    QVERIFY2(QFile::exists(source.toLocalFile()), qPrintable(source.toString()));
    QCOMPARE(dots->property("fillMode").toInt(),
             evaluate(dots, QStringLiteral("Image.PreserveAspectCrop")).toInt());
    QCOMPARE(dots->property("verticalAlignment").toInt(),
             evaluate(dots, QStringLiteral("Image.AlignVCenter")).toInt());
    const QSize drawn = QImage(source.toLocalFile()).size();
    const qreal coverAspect = evaluate(dots, QStringLiteral("Theme.coverSizeLarge.height"
                                                            " / Theme.coverSizeLarge.width"))
                                  .toReal();
    QVERIFY(qreal(drawn.height()) / drawn.width() > coverAspect);

    // The quick action is a search until another is chosen: the window raised, and the
    // address bar opened for a new tab, which is made once something is chosen and
    // counted from then on.
    QMetaObject::invokeMethod(coverPart(coverItem, QStringLiteral("quickCoverAction")),
                              "triggered");
    QCOMPARE(m_window->property("activateCount").toInt(), 1);
    QVERIFY(find(QStringLiteral("navigationBar"))->property("editing").toBool());
    QVERIFY(find(QStringLiteral("navigationBar"))->property("forNewTab").toBool());
    QCOMPARE(tabs->count(), 1);
    QObject *field = find(QStringLiteral("addressField"));
    field->setProperty("text", QStringLiteral("example.org"));
    enterKey(field);
    QCOMPARE(tabs->count(), 2);
}

// With nowhere to say the reader was -- no tab open, or the one in front on the start
// page -- the halftone alone, whole; the quick action stays.
void tst_qmlload::coverWithNothingToSay()
{
    auto *coverItem = m_window->property("coverItem").value<QObject *>();
    QVERIFY(coverItem != nullptr);
    TabModel *tabs = m_core->tabs();

    tabs->newTab(QString());
    QCOMPARE(coverView(coverItem), QString());
    QVERIFY(shown(coverItem, QStringLiteral("coverHalftone")));
    QCOMPARE(halftoneStrength(coverItem), 1.0);
    QVERIFY(
        coverPart(coverItem, QStringLiteral("quickCoverActions"))->property("enabled").toBool());

    tabs->closeAllTabs();
    QTRY_COMPARE(tabs->activeUrl(), QString());
    QCOMPARE(coverView(coverItem), QString());
    QCOMPARE(halftoneStrength(coverItem), 1.0);
}

// While something downloads, how far the downloads have come, in steps of five; before
// what plays, when both happen at once.
void tst_qmlload::coverShowsDownloads()
{
    auto *coverItem = m_window->property("coverItem").value<QObject *>();
    QVERIFY(coverItem != nullptr);
    Core *core = m_core.data();
    const auto progress = [core](int id, int percent) {
        observeDownload(core, {{QStringLiteral("msg"), QStringLiteral("dl-progress")},
                               {QStringLiteral("id"), id},
                               {QStringLiteral("percent"), percent}});
    };
    const auto start = [core](int id, const QString &name) {
        observeDownload(
            core, {{QStringLiteral("msg"), QStringLiteral("dl-start")},
                   {QStringLiteral("id"), id},
                   {QStringLiteral("displayName"), name},
                   {QStringLiteral("sourceUrl"), QStringLiteral("https://files.example/") + name},
                   {QStringLiteral("targetPath"), QStringLiteral("/tmp/") + name},
                   {QStringLiteral("mimeType"), QStringLiteral("application/pdf")},
                   {QStringLiteral("size"), 2048}});
    };
    QObject *ring = coverPart(coverItem, QStringLiteral("coverDownloadsRing"));
    const auto percent = [coverItem]() {
        return coverPart(coverItem, QStringLiteral("coverDownloadsPercent"))
            ->property("text")
            .toString();
    };

    start(1, QStringLiteral("map.pdf"));
    progress(1, 43);
    QCOMPARE(coverView(coverItem), QStringLiteral("coverDownloads"));
    QVERIFY(halftoneStrength(coverItem) < 0.2);
    QCOMPARE(percent(), QStringLiteral("40"));
    QCOMPARE(ring->property("value").toReal(), 0.4);
    QCOMPARE(
        coverPart(coverItem, QStringLiteral("coverDownloadsCount"))->property("text").toString(),
        QStringLiteral("1 file(s)"));
    // Within a step, nothing on the cover changes.
    QSignalSpy redrawn(ring, SIGNAL(valueChanged()));
    progress(1, 44);
    QCOMPARE(redrawn.count(), 0);
    progress(1, 45);
    QCOMPARE(redrawn.count(), 1);
    QCOMPARE(percent(), QStringLiteral("45"));

    // Two together; and playing meanwhile, the downloads stay, the mute beside the action.
    start(2, QStringLiteral("iso.pdf"));
    QCOMPARE(
        coverPart(coverItem, QStringLiteral("coverDownloadsCount"))->property("text").toString(),
        QStringLiteral("2 file(s)"));
    m_core->tabs()->setMediaState(m_core->tabs()->activeTabId(), TabModel::MediaPlaying);
    QCOMPARE(coverView(coverItem), QStringLiteral("coverDownloads"));
    QVERIFY(
        coverPart(coverItem, QStringLiteral("mediaCoverActions"))->property("enabled").toBool());

    // Done, what plays comes back; and with nothing playing, where the reader was.
    observeDownload(core, {{QStringLiteral("msg"), QStringLiteral("dl-done")},
                           {QStringLiteral("id"), 1},
                           {QStringLiteral("targetPath"), QStringLiteral("/tmp/map.pdf")}});
    observeDownload(
        core, {{QStringLiteral("msg"), QStringLiteral("dl-fail")}, {QStringLiteral("id"), 2}});
    QCOMPARE(coverView(coverItem), QStringLiteral("coverMedia"));
    m_core->tabs()->setMediaState(m_core->tabs()->activeTabId(), TabModel::NoMedia);
    QCOMPARE(coverView(coverItem), QStringLiteral("coverPlace"));
}

// While the tab in front plays, what plays: with a picture, the picture and under it
// what the page calls it; without one, the site large over the faint halftone. Muted,
// it reads paused and the picture dims.
void tst_qmlload::coverShowsWhatPlays()
{
    auto *coverItem = m_window->property("coverItem").value<QObject *>();
    QVERIFY(coverItem != nullptr);
    // As large as the home screen draws it, so there is a layout to measure.
    coverItem->setProperty("width", 234);
    coverItem->setProperty("height", 374);
    TabModel *tabs = m_core->tabs();
    const int front = tabs->activeTabId();
    tabs->updateTitle(front, QStringLiteral("Yle Areena"));
    const auto text = [coverItem](const QString &name) {
        return coverPart(coverItem, name)->property("text").toString();
    };

    // Nothing said of it: the plain view, the page's own title.
    tabs->setMediaState(front, TabModel::MediaPlaying);
    QCOMPARE(coverView(coverItem), QStringLiteral("coverMedia"));
    QVERIFY(shown(coverItem, QStringLiteral("coverMediaPlain")));
    QVERIFY(!shown(coverItem, QStringLiteral("coverMediaFrame")));
    QVERIFY(shown(coverItem, QStringLiteral("coverHalftone")));
    QVERIFY(halftoneStrength(coverItem) < 0.2);
    QCOMPARE(text(QStringLiteral("coverMediaPlainState")), QStringLiteral("Playing"));
    QCOMPARE(text(QStringLiteral("coverMediaPlainHost")), QStringLiteral("qwant.com"));
    QCOMPARE(text(QStringLiteral("coverMediaPlainTitle")), QStringLiteral("Yle Areena"));
    QVERIFY(!shown(coverItem, QStringLiteral("coverMediaPlainArtist")));

    // What the page says, with no picture: its words, still plain.
    TabModel::MediaMetadata said{QStringLiteral("Symphony No. 5"), QStringLiteral("Beethoven"),
                                 QString()};
    tabs->setMediaMetadata(front, said);
    QCOMPARE(text(QStringLiteral("coverMediaPlainTitle")), QStringLiteral("Symphony No. 5"));
    QCOMPARE(text(QStringLiteral("coverMediaPlainArtist")), QStringLiteral("Beethoven"));

    // With a picture: the picture, and the halftone gives it the room.
    said.artwork =
        QUrl::fromLocalFile(QStringLiteral(SALAMA_SOURCE_DIR "/art/logo.png")).toString();
    tabs->setMediaMetadata(front, said);
    QTRY_VERIFY(shown(coverItem, QStringLiteral("coverMediaFrame")));
    QVERIFY(!shown(coverItem, QStringLiteral("coverMediaPlain")));
    QVERIFY(!shown(coverItem, QStringLiteral("coverHalftone")));
    QCOMPARE(text(QStringLiteral("coverMediaTitle")), QStringLiteral("Symphony No. 5"));
    QCOMPARE(text(QStringLiteral("coverMediaArtist")), QStringLiteral("Beethoven"));
    QCOMPARE(text(QStringLiteral("coverMediaState")), QStringLiteral("Playing · qwant.com"));
    QObject *frame = coverPart(coverItem, QStringLiteral("coverMediaFrame"));
    // Square, as the picture is, and clear of the actions with the words under it;
    // fetched at the widest it is shown at.
    QObject *mediaView = coverPart(coverItem, QStringLiteral("coverMedia"));
    QObject *caption = coverPart(coverItem, QStringLiteral("coverMediaCaption"));
    QCOMPARE(frame->property("height"), frame->property("width"));
    QVERIFY(frame->property("width").toReal() > 0);
    QVERIFY(caption->property("y").toReal() + caption->property("height").toReal() <=
            mediaView->property("height").toReal());
    QCOMPARE(coverPart(coverItem, QStringLiteral("coverMediaArtwork"))
                 ->property("sourceSize")
                 .toSize()
                 .width(),
             qRound(mediaView->property("width").toReal()));
    QCOMPARE(frame->property("opacity").toReal(), 1.0);

    // Muted: paused, dimmed, the speaker struck through.
    tabs->setMuted(front, true);
    QCOMPARE(text(QStringLiteral("coverMediaState")), QStringLiteral("Paused · qwant.com"));
    QVERIFY(frame->property("opacity").toReal() < 1.0);
    QVERIFY(coverPart(coverItem, QStringLiteral("muteCoverAction"))
                ->property("iconSource")
                .toUrl()
                .toString()
                .contains(QLatin1String("speaker-mute")));

    // Stopped, where the reader was.
    tabs->setMuted(front, false);
    tabs->setMediaState(front, TabModel::NoMedia);
    QCOMPARE(coverView(coverItem), QStringLiteral("coverPlace"));
}

namespace {

// Whether a cover action's picture is the file named, drawn for the stub's small icon
// size and dark ambience, and there to be read by the home screen.
bool drawnFrom(const QUrl &picture, const QString &file)
{
    return picture.toString().endsWith(QStringLiteral("art/cover/") + file) &&
           QFile::exists(picture.toLocalFile());
}

// The glyphs a picture of the cover draws its actions in, left to right.
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
    CoverSettings *settings = m_core->coverSettings();
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
    settings->setQuickAction(CoverSettings::QuickActionBookmarks);
    tap();
    QCOMPARE(currentPage()->objectName(), QStringLiteral("bookmarksPage"));
    QCOMPARE(pageStack()->property("depth").toInt(), 2);
    QVERIFY(!bar->property("editing").toBool());
    popPage();
    settings->setQuickAction(CoverSettings::QuickActionDownloads);
    tapBar(QStringLiteral("menu"));
    QVERIFY(menu->property("open").toBool());
    tap();
    QCOMPARE(currentPage()->objectName(), QStringLiteral("downloadsPage"));
    QVERIFY(!menu->property("open").toBool());
    popPage();
    settings->setQuickAction(CoverSettings::QuickActionHistory);
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
    settings->setQuickAction(CoverSettings::QuickActionBookmark);
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
    QCOMPARE(settings->quickAction(), int(CoverSettings::QuickActionBookmark));
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
    CoverSettings *settings = m_core->coverSettings();
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
    const QList<QPair<CoverSettings::QuickAction, QString>> glyphs{
        {CoverSettings::QuickActionBookmarks, QStringLiteral("bookmarks")},
        {CoverSettings::QuickActionDownloads, QStringLiteral("downloads")},
        {CoverSettings::QuickActionHistory, QStringLiteral("history")},
        {CoverSettings::QuickActionBookmark, QStringLiteral("globe")},
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
    settings->setQuickAction(CoverSettings::QuickActionNone);
    QCOMPARE(enabled(), QStringList{QStringLiteral("muteCoverActions")});
    QVERIFY(drawnFrom(picture(QStringLiteral("loneMuteCoverAction")),
                      QStringLiteral("speaker-mute-32-white.png")));
    QMetaObject::invokeMethod(
        coverItem->findChild<QObject *>(QStringLiteral("loneMuteCoverAction")), "triggered");
    QVERIFY(!tabs->isMuted(tabs->activeTabId()));

    // Neither: nothing on the home screen.
    QCOMPARE(enabled(), QStringList());
    settings->setQuickAction(CoverSettings::QuickActionSearch);
    QCOMPARE(enabled(), QStringList{QStringLiteral("quickCoverActions")});
    QVERIFY2(errors.all().isEmpty(), qPrintable(errors.all()));
}

// The choice of the quick action on the cover's settings page: each row writes it, is
// lit while it is the one set, wears the glyph it has on the cover, and the pictures of
// the cover draw it. "Open a bookmark" asks which, and backing out has chosen nothing;
// its row then names the bookmark, and under the rows are the glyphs it can wear, drawn
// from the files the cover hands the home screen.
void tst_qmlload::quickActionChoice()
{
    ScriptErrors errors;
    CoverSettings *settings = m_core->coverSettings();
    BookmarkModel *bookmarks = m_core->bookmarks();
    const int wiki = bookmarks->add(QStringLiteral("https://forest.example/wiki"),
                                    QStringLiteral("Forest wiki"));
    const int sea = bookmarks->add(QStringLiteral("https://sea.example/"), QStringLiteral("Sea"));
    openMenuItem(QStringLiteral("settingsMenuButton"));
    click(find(QStringLiteral("coverSettingsEntry")));
    QObject *page = currentPage();
    QObject *icons = find(QStringLiteral("quickActionIconPicker"));
    QObject *preview = find(QStringLiteral("coverPreview"));
    QObject *bookmarkItem = find(QStringLiteral("quickAction-bookmark"));
    QObject *detail = findObjects(bookmarkItem, QStringLiteral("quickActionDetail")).first();
    const auto glyphOf = [](QObject *row) {
        return findObjects(row, QStringLiteral("quickActionGlyph")).first();
    };

    // A search until another is chosen: in the middle of the picture's strip, drawn from
    // the cover's own files, and its row wearing the same.
    QCOMPARE(previewGlyphs(preview), QStringList{QStringLiteral("search")});
    QVERIFY(drawnFrom(
        findObjects(preview, QStringLiteral("previewAction")).first()->property("source").toUrl(),
        QStringLiteral("search-32-white.png")));
    QVERIFY(
        drawnFrom(glyphOf(find(QStringLiteral("quickAction-search")))->property("source").toUrl(),
                  QStringLiteral("search-32-white.png")));
    QVERIFY(glyphOf(find(QStringLiteral("quickAction-search")))->property("highlighted").toBool());
    QVERIFY(!icons->property("visible").toBool());

    struct Choice
    {
        QString item;
        CoverSettings::QuickAction action;
        QString name;
        QString glyph;
    };
    const QList<Choice> choices{
        {QStringLiteral("quickAction-none"), CoverSettings::QuickActionNone, QStringLiteral("None"),
         QString()},
        {QStringLiteral("quickAction-bookmarks"), CoverSettings::QuickActionBookmarks,
         QStringLiteral("Bookmarks"), QStringLiteral("bookmarks")},
        {QStringLiteral("quickAction-downloads"), CoverSettings::QuickActionDownloads,
         QStringLiteral("Downloads"), QStringLiteral("downloads")},
        {QStringLiteral("quickAction-history"), CoverSettings::QuickActionHistory,
         QStringLiteral("History"), QStringLiteral("history")},
        {QStringLiteral("quickAction-search"), CoverSettings::QuickActionSearch,
         QStringLiteral("Search"), QStringLiteral("search")},
    };
    for (const Choice &choice : choices) {
        QObject *item = find(choice.item);
        QCOMPARE(textIn(item, QStringLiteral("quickActionName")), choice.name);
        // No action wears nothing: a dot keeps its place.
        QCOMPARE(glyphOf(item)->property("visible").toBool(), !choice.glyph.isEmpty());
        if (!choice.glyph.isEmpty()) {
            QVERIFY(drawnFrom(glyphOf(item)->property("source").toUrl(),
                              choice.glyph + QStringLiteral("-32-white.png")));
        }
        click(item);
        QCOMPARE(settings->quickAction(), int(choice.action));
        QVERIFY(item->property("chosen").toBool());
        QVERIFY(glyphOf(item)->property("highlighted").toBool());
        for (const Choice &other : choices) {
            if (other.item != choice.item) {
                QVERIFY2(!find(other.item)->property("chosen").toBool(), qPrintable(other.item));
            }
        }
        // No action leaves the strip bare.
        QCOMPARE(previewGlyphs(preview),
                 choice.glyph.isEmpty() ? QStringList() : QStringList{choice.glyph});
        QVERIFY(!icons->property("visible").toBool());
    }

    // "Open a bookmark" asks which, and backing out leaves the action as it was.
    QCOMPARE(textIn(bookmarkItem, QStringLiteral("quickActionName")),
             QStringLiteral("Open a bookmark"));
    click(bookmarkItem);
    QCOMPARE(currentPage()->objectName(), QStringLiteral("bookmarkPickerPage"));
    QCOMPARE(findAll(QStringLiteral("bookmarkPickerRow")).count(), 2);
    popPage();
    QCOMPARE(currentPage(), page);
    QCOMPARE(settings->quickAction(), int(CoverSettings::QuickActionSearch));
    QCOMPARE(settings->quickActionBookmark(), 0);
    QVERIFY(!bookmarkItem->property("chosen").toBool());
    QVERIFY(!detail->property("visible").toBool());

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
    QCOMPARE(settings->quickAction(), int(CoverSettings::QuickActionBookmark));
    QCOMPARE(settings->quickActionBookmark(), wiki);
    QCOMPARE(settings->quickActionBookmarkUrl(), QStringLiteral("https://forest.example/wiki"));
    QCOMPARE(settings->quickActionBookmarkTitle(), QStringLiteral("Forest wiki"));
    QVERIFY(bookmarkItem->property("chosen").toBool());
    QVERIFY(detail->property("visible").toBool());
    QCOMPARE(detail->property("text").toString(), QStringLiteral("Forest wiki"));
    QCOMPARE(bookmarkItem->property("height"),
             evaluate(page, QStringLiteral("Theme.itemSizeMedium")));

    // Under the rows, every glyph it can wear, the one it wears lit; a tap on another
    // writes it, and the row, the pictures and the cover follow.
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
    QCOMPARE(previewGlyphs(preview), QStringList{QStringLiteral("globe")});
    click(find(QStringLiteral("quickActionIcon-heart")));
    QCOMPARE(settings->quickActionIcon(), QStringLiteral("heart"));
    QVERIFY(find(QStringLiteral("quickActionIcon-heart"))->property("highlighted").toBool());
    QVERIFY(!find(QStringLiteral("quickActionIcon-globe"))->property("highlighted").toBool());
    QCOMPARE(previewGlyphs(preview), QStringList{QStringLiteral("heart")});
    QVERIFY(drawnFrom(glyphOf(bookmarkItem)->property("source").toUrl(),
                      QStringLiteral("heart-32-white.png")));
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
    QCOMPARE(detail->property("text").toString(), QStringLiteral("Sea"));
    QCOMPARE(settings->quickActionIcon(), QStringLiteral("heart"));

    // Another action chosen, the bookmark's row still names the bookmark picked, for
    // when it is chosen again.
    click(find(QStringLiteral("quickAction-history")));
    QVERIFY(detail->property("visible").toBool());
    QCOMPARE(detail->property("text").toString(), QStringLiteral("Sea"));
    QVERIFY(!icons->property("visible").toBool());

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
    CoverSettings *settings = m_core->coverSettings();
    BookmarkModel *bookmarks = m_core->bookmarks();
    TabModel *tabs = m_core->tabs();
    const QString url = QStringLiteral("https://bee.example/");
    const int front = tabs->activeTabId();
    const int beeTab = tabs->newTab(url);
    const int first = bookmarks->add(url, QStringLiteral("Bee"));
    settings->setQuickActionBookmark(first, url, QStringLiteral("Bee"));
    settings->setQuickAction(CoverSettings::QuickActionBookmark);
    const auto openCoverSettings = [this]() {
        openMenuItem(QStringLiteral("settingsMenuButton"));
        click(find(QStringLiteral("coverSettingsEntry")));
        return findObjects(find(QStringLiteral("quickAction-bookmark")),
                           QStringLiteral("quickActionDetail"))
            .first();
    };
    const auto backToBrowser = [this]() {
        popPage();
        popPage();
    };
    QObject *value = openCoverSettings();
    QCOMPARE(value->property("text").toString(), QStringLiteral("Bee"));

    // Renamed, while the page is open: read again, and what the setting keeps of it with it.
    bookmarks->edit(bookmarks->count() - 1, url, QStringLiteral("Bee hive"));
    QCOMPARE(value->property("text").toString(), QStringLiteral("Bee hive"));
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
    QCOMPARE(settings->quickAction(), int(CoverSettings::QuickActionBookmark));
    value = openCoverSettings();
    QCOMPARE(value->property("text").toString(), bookmarks->titleOf(second));
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
    const auto thumbnail = [this]() {
        return m_core->tabs()
            ->data(m_core->tabs()->index(0, 0), roleId(TabModel::Role::Thumbnail))
            .toString();
    };
    webView->setProperty("loading", true);
    webView->setProperty("loading", false);
    QTRY_VERIFY(!thumbnail().isEmpty());
    const QString onLoad = thumbnail();
    const int grabs = webView->property("grabCount").toInt();

    // Nothing is taken while the application is still the one on screen.
    QObject *page = find(QStringLiteral("browserPage"));
    QMetaObject::invokeMethod(page, "applicationStateChanged",
                              Q_ARG(QVariant, Qt::ApplicationActive));
    QCOMPARE(webView->property("grabCount").toInt(), grabs);

    // Leaving it is the cover's last chance at a current picture of this tab.
    QMetaObject::invokeMethod(page, "applicationStateChanged",
                              Q_ARG(QVariant, Qt::ApplicationInactive));
    QCOMPARE(webView->property("grabCount").toInt(), grabs + 1);
    QTRY_VERIFY(thumbnail() != onLoad);
    QVERIFY(!thumbnail().isEmpty());
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
    // says of downloads; and, by the notifications' part of the page, for the sites'
    // permissions.
    const QStringList topics =
        activity->topics() +
        QStringList{m_core->downloads()->topic(), m_core->notificationPermissions()->topic()};
    QCOMPARE(evaluate(scope, QStringLiteral("WebEngine.observers")).toStringList(), topics);

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
        m_core->tabs()->data(m_core->tabs()->index(1, 0), roleId(TabModel::Role::TabId)).toInt());
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
// A cell of the grid is its picture alone, marked when it is the active tab by the
// wash round it and nothing else.
void tst_qmlload::gridCellsArePicturesAlone()
{
    m_core->tabs()->newTab(QStringLiteral("https://two.example/"));
    pullUpToTabs();
    QList<QObject *> previews = findAll(QStringLiteral("tabPreview"));
    QCOMPARE(previews.count(), 2);
    QObject *cell = previews.at(1);

    // The preview box is rounded. Clipping is rectangular whatever the shape of the
    // item doing it, so the picture is cut to the box's corners by a mask.
    QObject *shot = findObjects(cell, QStringLiteral("tabPreviewShot")).first();
    QVERIFY(shot->property("radius").toReal() > 0);
    // The active one is marked by a thin frame just outside the picture, following its
    // corners, in the highlight background colour -- not a square wash behind it, which
    // is gone -- and the other cells by nothing.
    QVERIFY(findObjects(cell, QStringLiteral("tabPreviewHighlight")).isEmpty());
    auto *frame =
        qobject_cast<QQuickItem *>(findObjects(cell, QStringLiteral("tabPreviewFrame")).first());
    QVERIFY(frame->isVisible());
    QCOMPARE(frame->property("color").value<QColor>().alpha(), 0);
    auto *border = frame->property("border").value<QObject *>();
    const qreal stroke = border->property("width").toReal();
    QCOMPARE(stroke, evaluate(cell, QStringLiteral("Theme._lineWidth")).toReal());
    QVERIFY(stroke < evaluate(cell, QStringLiteral("Theme.paddingSmall")).toReal());
    QCOMPARE(border->property("color").value<QColor>(),
             evaluate(cell, QStringLiteral("Theme.highlightBackgroundColor")).value<QColor>());
    const qreal gap = cell->property("frameGap").toReal();
    QVERIFY(gap > 0);
    const QRectF picture(
        qobject_cast<QQuickItem *>(shot)->mapToScene(QPointF(0, 0)),
        QSizeF(shot->property("width").toReal(), shot->property("height").toReal()));
    const QRectF framed(frame->mapToScene(QPointF(0, 0)), QSizeF(frame->width(), frame->height()));
    QCOMPARE(framed, picture.adjusted(-gap - stroke, -gap - stroke, gap + stroke, gap + stroke));
    QCOMPARE(frame->property("radius").toReal(), shot->property("radius").toReal() + gap + stroke);
    // Inside the cell, short of its edges.
    QVERIFY(gap + stroke < cell->property("inset").toReal());
    QVERIFY(!findObjects(previews.at(0), QStringLiteral("tabPreviewFrame"))
                 .first()
                 ->property("visible")
                 .toBool());
    // The picture sits in from the cell's edges by a little more than a medium padding,
    // and two cells stand twice that apart. Nothing is under it: no favicon and no
    // title, so it runs down to the same inset at the foot.
    const qreal inset = cell->property("inset").toReal();
    QVERIFY(inset > evaluate(cell, QStringLiteral("Theme.paddingMedium")).toReal());
    QCOMPARE(shot->property("x").toReal(), inset);
    QCOMPARE(shot->property("y").toReal(), inset);
    QCOMPARE(shot->property("width").toReal(), cell->property("width").toReal() - 2 * inset);
    QCOMPARE(shot->property("height").toReal(), cell->property("height").toReal() - 2 * inset);
    QVERIFY(findObjects(cell, QStringLiteral("tabTitle")).isEmpty());
    QVERIFY(findObjects(cell, QStringLiteral("tabFavicon")).isEmpty());
    auto *shotLayer = shot->property("layer").value<QObject *>();
    QVERIFY(shotLayer != nullptr);
    QVERIFY(shotLayer->property("enabled").toBool());
    QVERIFY(cell->property("highlighted").toBool());
}

// The rows along the grid's head and foot: their tint opaque, as the navigation bar's
// is, so no cell shows through either.
void tst_qmlload::gridRowsAreOpaque()
{
    QObject *headRow = find(QStringLiteral("gridHeadRow"));
    QObject *footRow = find(QStringLiteral("gridFootRow"));
    QVERIFY(headRow != nullptr);
    QVERIFY(footRow != nullptr);
    const QColor tint = headRow->property("color").value<QColor>();
    QCOMPARE(tint.alphaF(), 1.0);
    QCOMPARE(tint, evaluate(headRow, QStringLiteral("Theme.highlightDimmerColor")).value<QColor>());
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
    media->answer(first, Salama::PageMedia::Command::Query, QStringLiteral("playing"));
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

namespace {

// What the frame script hands the view from its page: the page's message, with the
// origin and the permission Gecko holds.
void relay(QObject *view, const QVariantMap &message,
           const QString &origin = QLatin1String(ChatSite),
           const QString &permission = QStringLiteral("default"))
{
    QVariantMap detail = message;
    detail.insert(QStringLiteral("page"), QStringLiteral("p1"));
    const QVariantMap data{
        {QStringLiteral("origin"), origin},
        {QStringLiteral("permission"), permission},
        {QStringLiteral("detail"),
         QString::fromUtf8(QJsonDocument::fromVariant(detail).toJson(QJsonDocument::Compact))},
    };
    QMetaObject::invokeMethod(view, "recvAsyncMessage",
                              Q_ARG(QString, QStringLiteral("salama:notification")),
                              Q_ARG(QVariant, data));
}

QVariantMap message(const QString &type, int id, const QString &title = QString(),
                    const QString &tag = QString())
{
    QVariantMap message{{QStringLiteral("type"), type}, {QStringLiteral("id"), id}};
    if (!title.isEmpty()) {
        message.insert(QStringLiteral("title"), title);
        message.insert(QStringLiteral("body"), title + QStringLiteral(" and more"));
        message.insert(QStringLiteral("tag"), tag);
    }
    return message;
}

// What the replies run in a view told its page, as "type:id" or "permission:id:state".
QStringList repliesIn(QObject *view)
{
    static const QRegularExpression reply(
        QStringLiteral("^window\\.dispatchEvent\\(new CustomEvent\\('salama-notification-reply', "
                       "\\{ detail: (\".*\") \\}\\)\\); return true;$"));
    QStringList list;
    for (const QString &script : view->property("scripts").toStringList()) {
        const QRegularExpressionMatch match = reply.match(script);
        if (!match.hasMatch()) {
            continue;
        }
        const QString json = QJsonDocument::fromJson(
                                 (QLatin1Char('[') + match.captured(1) + QLatin1Char(']')).toUtf8())
                                 .array()
                                 .at(0)
                                 .toString();
        const QVariantMap said = QJsonDocument::fromJson(json.toUtf8()).toVariant().toMap();
        QString line = said.value(QStringLiteral("type")).toString() + QLatin1Char(':') +
                       QString::number(said.value(QStringLiteral("id")).toInt());
        if (said.contains(QStringLiteral("permission"))) {
            line += QLatin1Char(':') + said.value(QStringLiteral("permission")).toString();
        }
        list.append(line);
    }
    return list;
}

} // namespace

// A page's notifications (docs/DECISIONS/0033-web-notifications.md): the frame script
// and the page's Notification put in each view, what the page shows made a platform
// notification, and a tap, a swipe and the page going.
void tst_qmlload::webNotifications()
{
    WebNotifications *notifications = m_core->webNotifications();
    QObject *view = currentWebView();
    const int tab = m_core->tabs()->activeTabId();

    // Heard once asked for, and the frame script loaded, both as the view is made.
    QVERIFY(
        view->property("messageListeners").toStringList().contains(notifications->messageName()));
    QCOMPARE(view->property("frameScripts").toStringList(),
             QStringList{notifications->relayScriptUrl()});
    // The page's Notification, as a document arrives and again once it has loaded -- once
    // the engine has made the view, which runs no script before.
    const auto installs = [view, notifications]() {
        return view->property("scripts").toStringList().count(notifications->pageScript());
    };
    QCOMPARE(installs(), 0);
    QMetaObject::invokeMethod(view, "viewInitialized");
    view->setProperty("loading", true);
    QCOMPARE(installs(), 0);
    view->setProperty("loading", false);
    const int installed = installs();
    QCOMPARE(installed, 1);
    view->setProperty("loading", true);
    view->setProperty("loading", false);
    QCOMPARE(installs(), installed + 1);
    view->setProperty("url", QStringLiteral("https://www.qwant.com/?q=chat"));
    QCOMPARE(installs(), installed + 2);

    // Shown: a platform notification, the page's title and text, the site under them.
    relay(view, message(QStringLiteral("show"), 1, QStringLiteral("Hello"), QStringLiteral("room")),
          QLatin1String(ChatSite), QStringLiteral("granted"));
    QList<QObject *> shown = findAll(QStringLiteral("webNotification"));
    QCOMPARE(shown.count(), 1);
    QPointer<QObject> hello = shown.first();
    QCOMPARE(hello->property("summary").toString(), QStringLiteral("Hello"));
    QCOMPARE(hello->property("body").toString(), QStringLiteral("Hello and more"));
    QCOMPARE(hello->property("previewSummary").toString(), QStringLiteral("Hello"));
    QCOMPARE(hello->property("previewBody").toString(), QStringLiteral("Hello and more"));
    QCOMPARE(hello->property("subText").toString(), QStringLiteral("chat.example"));
    QCOMPARE(hello->property("icon").toString(), QString());
    QCOMPARE(hello->property("appName").toString(), QStringLiteral("Salama"));
    QVERIFY(hello->property("appIcon").toString().endsWith(
        QLatin1String("/icons/hicolor/172x172/apps/harbour-salama.png")));
    const QVariantList actions = hello->property("remoteActions").toList();
    QCOMPARE(actions.count(), 1);
    QCOMPARE(actions.first().toMap().value(QStringLiteral("name")).toString(),
             QStringLiteral("default"));
    QCOMPARE(hello->property("publishCount").toInt(), 1);
    QCOMPARE(repliesIn(view), QStringList{QStringLiteral("show:1")});

    // One with the same tag in its place.
    relay(view, message(QStringLiteral("show"), 2, QStringLiteral("Again"), QStringLiteral("room")),
          QLatin1String(ChatSite), QStringLiteral("granted"));
    QCOMPARE(findAll(QStringLiteral("webNotification")), QList<QObject *>{hello.data()});
    QCOMPARE(hello->property("summary").toString(), QStringLiteral("Again"));
    QCOMPARE(hello->property("publishCount").toInt(), 2);

    // Tapped with another tab in front: that tab to the front, the browser with it, and
    // the page told, before the notification closes.
    m_core->tabs()->newTab(QStringLiteral("https://two.example/"));
    QVERIFY(currentWebView() != view);
    const int activations = m_window->property("activateCount").toInt();
    QMetaObject::invokeMethod(hello, "clicked");
    // Closed, and gone with the next turn of the event loop.
    QVERIFY(hello->property("isClosed").toBool());
    QCOMPARE(m_core->tabs()->activeTabId(), tab);
    QCOMPARE(currentWebView(), view);
    QCOMPARE(m_window->property("activateCount").toInt(), activations + 1);
    QCOMPARE(repliesIn(view).mid(2),
             (QStringList{QStringLiteral("click:2"), QStringLiteral("close:2")}));
    settle();
    QVERIFY(hello.isNull());
    QVERIFY(notifications->keys().isEmpty());

    // Tapped over Settings: back on the page.
    relay(view, message(QStringLiteral("show"), 3, QStringLiteral("Over")), QLatin1String(ChatSite),
          QStringLiteral("granted"));
    openMenuItem(QStringLiteral("settingsMenuButton"));
    QCOMPARE(currentPage()->objectName(), QStringLiteral("settingsPage"));
    QMetaObject::invokeMethod(findAll(QStringLiteral("webNotification")).first(), "clicked");
    QCOMPARE(currentPage()->objectName(), QStringLiteral("browserPage"));

    // Swiped away: the page told, and nothing left of it.
    relay(view, message(QStringLiteral("show"), 4, QStringLiteral("Swiped")),
          QLatin1String(ChatSite), QStringLiteral("granted"));
    QPointer<QObject> swiped = findAll(QStringLiteral("webNotification")).first();
    QMetaObject::invokeMethod(swiped, "closed", Q_ARG(int, 1));
    QCOMPARE(repliesIn(view).last(), QStringLiteral("close:4"));
    settle();
    QVERIFY(swiped.isNull());

    // Closed by the page: closed on the platform too.
    relay(view, message(QStringLiteral("show"), 5, QStringLiteral("Closed")),
          QLatin1String(ChatSite), QStringLiteral("granted"));
    QPointer<QObject> closed = findAll(QStringLiteral("webNotification")).first();
    relay(view, message(QStringLiteral("close"), 5));
    QVERIFY(closed->property("isClosed").toBool());
    settle();
    QVERIFY(closed.isNull());

    // Not allowed: nothing shown, the page told.
    relay(view, message(QStringLiteral("show"), 6, QStringLiteral("No")));
    QVERIFY(findAll(QStringLiteral("webNotification")).isEmpty());
    QCOMPARE(repliesIn(view).last(), QStringLiteral("error:6"));

    // The page going, and the tab: what they showed goes with them.
    relay(view, message(QStringLiteral("show"), 7, QStringLiteral("Unload")),
          QLatin1String(ChatSite), QStringLiteral("granted"));
    QPointer<QObject> unloaded = findAll(QStringLiteral("webNotification")).first();
    relay(view, {{QStringLiteral("type"), QStringLiteral("unload")}});
    QVERIFY(unloaded->property("isClosed").toBool());
    relay(view, message(QStringLiteral("show"), 8, QStringLiteral("Tab")), QLatin1String(ChatSite),
          QStringLiteral("granted"));
    QPointer<QObject> ofTheTab = findAll(QStringLiteral("webNotification")).first();
    m_core->tabs()->closeTabById(tab);
    settle();
    QVERIFY(ofTheTab.isNull() || ofTheTab->property("isClosed").toBool());
    QVERIFY(notifications->keys().isEmpty());
}

// A page asks, and is asked about as Firefox asks: allow, always block, not now; only
// while it is the page on the screen; and the platform's own refusal taken back.
void tst_qmlload::notificationPermissions()
{
    NotificationPermissions *permissions = m_core->notificationPermissions();
    QObject *view = currentWebView();
    QMetaObject::invokeMethod(view, "viewInitialized");
    QObject *scope = find(QStringLiteral("viewArea"));
    const auto lastToEngine = [this, scope]() {
        const QVariantList sent =
            evaluate(scope, QStringLiteral("WebEngine.notifications")).toList();
        return sent.isEmpty() ? QVariantMap() : sent.last().toMap();
    };
    const auto question = [this]() {
        QObject *dialog = currentPage();
        return dialog->objectName() == QLatin1String("notificationPermissionDialog")
                   ? find(QStringLiteral("notificationPermissionQuestion"))
                         ->property("text")
                         .toString()
                   : QString();
    };

    // Allowed: for good, in the engine's keeping, and the page told -- every request
    // it made meanwhile.
    relay(view, message(QStringLiteral("request"), 1));
    QCOMPARE(question(), QStringLiteral("Allow chat.example to send notifications?"));
    QObject *dialog = currentPage();
    relay(view, message(QStringLiteral("request"), 2));
    QCOMPARE(currentPage(), dialog);
    QCOMPARE(pageStack()->property("depth").toInt(), 2);
    QMetaObject::invokeMethod(dialog, "accept");
    QVERIFY(permissions->isAllowed(QLatin1String(ChatSite)));
    QCOMPARE(lastToEngine().value(QStringLiteral("topic")).toString(),
             QStringLiteral("embedui:perms"));
    const QVariantMap added = lastToEngine().value(QStringLiteral("value")).toMap();
    QCOMPARE(added.value(QStringLiteral("msg")).toString(), QStringLiteral("add"));
    QCOMPARE(added.value(QStringLiteral("uri")).toString(), QLatin1String(ChatSite));
    QCOMPARE(added.value(QStringLiteral("permission")).toInt(), 1);
    QCOMPARE(repliesIn(view), (QStringList{QStringLiteral("permission:1:granted"),
                                           QStringLiteral("permission:2:granted")}));
    popPage();

    // Always block: for good.
    relay(view, message(QStringLiteral("request"), 3), QStringLiteral("https://news.example"));
    QCOMPARE(question(), QStringLiteral("Allow news.example to send notifications?"));
    click(find(QStringLiteral("blockNotificationsButton")));
    QVERIFY(permissions->isBlocked(QStringLiteral("https://news.example")));
    QCOMPARE(repliesIn(view).last(), QStringLiteral("permission:3:denied"));
    popPage();

    // Not now: refused, and nothing kept.
    relay(view, message(QStringLiteral("request"), 4), QStringLiteral("https://shop.example"));
    QMetaObject::invokeMethod(currentPage(), "reject");
    QCOMPARE(repliesIn(view).last(), QStringLiteral("permission:4:denied"));
    QCOMPARE(permissions->rowCount(), 2);
    popPage();

    // A page that goes while it asks takes its question with it.
    relay(view, message(QStringLiteral("request"), 5), QStringLiteral("https://gone.example"));
    QVERIFY(!question().isEmpty());
    relay(view, {{QStringLiteral("type"), QStringLiteral("unload")}});
    QCOMPARE(currentPage()->objectName(), QStringLiteral("browserPage"));
    QCOMPARE(repliesIn(view).count(), 4);

    // Not asked for a page behind the one in front, nor over the grid or another page,
    // nor with the application out of sight: refused this time.
    m_core->tabs()->newTab(QStringLiteral("https://two.example/"));
    QObject *front = currentWebView();
    QMetaObject::invokeMethod(front, "viewInitialized");
    relay(view, message(QStringLiteral("request"), 6), QStringLiteral("https://behind.example"));
    QCOMPARE(currentPage()->objectName(), QStringLiteral("browserPage"));
    QCOMPARE(repliesIn(view).last(), QStringLiteral("permission:6:denied"));
    openMenuItem(QStringLiteral("settingsMenuButton"));
    relay(front, message(QStringLiteral("request"), 7), QStringLiteral("https://over.example"));
    QCOMPARE(currentPage()->objectName(), QStringLiteral("settingsPage"));
    QCOMPARE(repliesIn(front).last(), QStringLiteral("permission:7:denied"));
    popPage();
    pullUpToTabs();
    relay(front, message(QStringLiteral("request"), 8), QStringLiteral("https://grid.example"));
    QCOMPARE(repliesIn(front).last(), QStringLiteral("permission:8:denied"));
    pullDownToBrowser();
    QCOMPARE(permissions->rowCount(), 2);

    // The platform's refusal of a page that asked the engine itself, taken back once it
    // has been sent -- for the page's own site, when it is not one decided on.
    const int sent = evaluate(scope, QStringLiteral("WebEngine.notifications.length")).toInt();
    const QVariantMap refused{
        {QStringLiteral("title"), QStringLiteral("desktopNotification")},
        {QStringLiteral("host"), QStringLiteral("two.example")},
        {QStringLiteral("id"), QStringLiteral("two.example desktop-notification")}};
    QMetaObject::invokeMethod(front, "aboutToOpenPopup",
                              Q_ARG(QVariant, QStringLiteral("embed:permissions")),
                              Q_ARG(QVariant, refused));
    QTRY_COMPARE(evaluate(scope, QStringLiteral("WebEngine.notifications.length")).toInt(),
                 sent + 1);
    QCOMPARE(lastToEngine()
                 .value(QStringLiteral("value"))
                 .toMap()
                 .value(QStringLiteral("msg"))
                 .toString(),
             QStringLiteral("remove"));
    QCOMPARE(lastToEngine()
                 .value(QStringLiteral("value"))
                 .toMap()
                 .value(QStringLiteral("uri"))
                 .toString(),
             QStringLiteral("https://two.example"));

    // A page of a site allowed to notify is not put to sleep out of sight; the rest are.
    QObject *page = find(QStringLiteral("browserPage"));
    QMetaObject::invokeMethod(page, "applicationStateChanged",
                              Q_ARG(QVariant, Qt::ApplicationInactive));
    QTRY_VERIFY(m_core->pageActivity()->asleep());
    QCOMPARE(front->property("calls").toStringList().count(QStringLiteral("suspendView")), 1);
    QCOMPARE(view->property("calls").toStringList().count(QStringLiteral("suspendView")), 1);
    QMetaObject::invokeMethod(page, "applicationStateChanged",
                              Q_ARG(QVariant, Qt::ApplicationActive));
    permissions->setAllowed(QStringLiteral("https://two.example"), true);
    QMetaObject::invokeMethod(page, "applicationStateChanged",
                              Q_ARG(QVariant, Qt::ApplicationInactive));
    QTRY_VERIFY(m_core->pageActivity()->asleep());
    QCOMPARE(front->property("calls").toStringList().count(QStringLiteral("suspendView")), 1);
    QCOMPARE(view->property("calls").toStringList().count(QStringLiteral("suspendView")), 2);
    // Asked while out of sight: refused this time.
    relay(front, message(QStringLiteral("request"), 9), QStringLiteral("https://asleep.example"));
    QCOMPARE(repliesIn(front).last(), QStringLiteral("permission:9:denied"));
    QMetaObject::invokeMethod(page, "applicationStateChanged",
                              Q_ARG(QVariant, Qt::ApplicationActive));
}

// Settings > Notifications: whether sites may ask, and the sites the engine keeps, the
// allowed and the blocked under a heading each, each with a way to change it or forget
// it.
void tst_qmlload::notificationSettingsPage()
{
    NotificationPermissions *permissions = m_core->notificationPermissions();
    PrivacySettings *settings = m_core->privacySettings();
    QObject *scope = find(QStringLiteral("viewArea"));
    const auto lastToEngine = [this, scope]() {
        const QVariantList sent =
            evaluate(scope, QStringLiteral("WebEngine.notifications")).toList();
        return sent.isEmpty() ? QVariantMap()
                              : sent.last().toMap().value(QStringLiteral("value")).toMap();
    };

    // Settings > Site permissions > Notifications, where the Notifications row was.
    openMenuItem(QStringLiteral("settingsMenuButton"));
    click(find(QStringLiteral("sitePermissionsSettingsEntry")));
    QCOMPARE(currentPage()->objectName(), QStringLiteral("sitePermissionsPage"));
    click(find(QStringLiteral("notificationsPermissionRow")));
    QObject *page = currentPage();
    QCOMPARE(page->objectName(), QStringLiteral("notificationSettingsPage"));
    // Opened, it asks the engine for what it keeps.
    QCOMPARE(lastToEngine().value(QStringLiteral("msg")).toString(), QStringLiteral("get-all"));
    QVERIFY(findAll(QStringLiteral("notificationSite")).isEmpty());

    evaluate(
        scope,
        QStringLiteral(
            "WebEngine.recvObserve('embed:perms:all', ["
            " {type: 'desktop-notification', uri: 'https://news.example', capability: 2, "
            "expireType: 0},"
            " {type: 'geolocation', uri: 'https://maps.example', capability: 1, expireType: 0},"
            " {type: 'desktop-notification', uri: 'https://chat.example', capability: 1, "
            "expireType: 0}])"));
    QCOMPARE(permissions->rowCount(), 2);
    const QList<QObject *> rows = byRow(findAll(QStringLiteral("notificationSite")));
    QCOMPARE(rows.count(), 2);
    const auto text = [](QObject *row, const char *name) {
        return findObjects(row, QLatin1String(name)).first()->property("text").toString();
    };
    QCOMPARE(text(rows.at(0), "notificationSiteHost"), QStringLiteral("chat.example"));
    QCOMPARE(text(rows.at(1), "notificationSiteHost"), QStringLiteral("news.example"));
    QCOMPARE(text(rows.at(0), "notificationSiteToggle"), QStringLiteral("Block"));
    QCOMPARE(text(rows.at(1), "notificationSiteToggle"), QStringLiteral("Allow"));
    QCOMPARE(text(rows.at(1), "notificationSiteRemove"), QStringLiteral("Forget this site"));
    // The headings are the list's sections, by whether a site is allowed.
    QObject *list = find(QStringLiteral("notificationSiteList"));
    QCOMPARE(evaluate(list, QStringLiteral("section.property")).toString(),
             QStringLiteral("allowed"));
    QCOMPARE(rows.at(0)->property("allowed").toBool(), true);
    QCOMPARE(rows.at(1)->property("allowed").toBool(), false);
    QStringList headings;
    for (QObject *heading : findAll(QStringLiteral("notificationSiteSection"))) {
        headings.append(heading->property("text").toString());
    }
    headings.sort();
    QCOMPARE(headings, (QStringList{QStringLiteral("Allowed"), QStringLiteral("Blocked")}));
    // Under the list, what forgetting a site does.
    QCOMPARE(find(QStringLiteral("notificationSitesFooter"))->property("text").toString(),
             QStringLiteral("A site you forget asks again the next time it wants to send one."));

    // From the menu: blocked, then removed, and the engine told each time.
    click(findObjects(rows.at(0), QStringLiteral("notificationSiteToggle")).first());
    QVERIFY(permissions->isBlocked(QLatin1String(ChatSite)));
    QCOMPARE(lastToEngine().value(QStringLiteral("msg")).toString(), QStringLiteral("add"));
    QCOMPARE(lastToEngine().value(QStringLiteral("permission")).toInt(), 2);
    click(findObjects(byRow(findAll(QStringLiteral("notificationSite"))).at(1),
                      QStringLiteral("notificationSiteRemove"))
              .first());
    QCOMPARE(lastToEngine().value(QStringLiteral("msg")).toString(), QStringLiteral("remove"));
    QCOMPARE(lastToEngine().value(QStringLiteral("uri")).toString(),
             QStringLiteral("https://news.example"));
    QCOMPARE(findAll(QStringLiteral("notificationSite")).count(), 1);

    // Whether others may ask, said the way round a switch that is on reads: the
    // engine's default for the permission.
    QObject *canAsk = find(QStringLiteral("sitesCanAskSwitch"));
    QVERIFY(canAsk->property("description").toString().isEmpty());
    QVERIFY(canAsk->property("checked").toBool());
    canAsk->setProperty("checked", false);
    QVERIFY(settings->blockNotificationRequests());
    const QVariantMap given =
        evaluate(scope, QStringLiteral("WebEngineSettings.preferences")).toList().last().toMap();
    QCOMPARE(given.value(QStringLiteral("key")).toString(),
             QStringLiteral("permissions.default.desktop-notification"));
    QCOMPARE(given.value(QStringLiteral("value")).toInt(), 2);
    canAsk->setProperty("checked", true);
    QVERIFY(!settings->blockNotificationRequests());

    // Nothing left: the page says what it will list.
    permissions->remove(QLatin1String(ChatSite));
    QVERIFY(findAll(QStringLiteral("notificationSite")).isEmpty());
}

namespace {

// The engine's list of permissions, as qtmozembed hands it over.
QString permissionList(const QList<QStringList> &entries)
{
    QStringList items;
    for (const QStringList &entry : entries) {
        items.append(QStringLiteral("{type: '%1', uri: '%2', capability: %3, expireType: 0}")
                         .arg(entry.at(0), entry.at(1), entry.at(2)));
    }
    return QStringLiteral("WebEngine.recvObserve('embed:perms:all', [%1])")
        .arg(items.join(QLatin1Char(',')));
}

} // namespace

// Settings > Site permissions: a row for each kind with how it is set and how many sites
// are an exception to it, a tap to change the default or go to the exceptions, the
// cookies only while tracking protection is off or a site has an exception to them, and
// the sites tracking protection is off for under a heading of their own. Every choice is
// written as it is made, and reaches the engine (docs/DECISIONS/0039-site-permissions.md).
void tst_qmlload::sitePermissionsPage()
{
    SitePermissionSettings *defaults = m_core->sitePermissionSettings();
    QObject *scope = find(QStringLiteral("viewArea"));
    const auto lastToEngine = [this, scope]() {
        const QVariantList sent =
            evaluate(scope, QStringLiteral("WebEngine.notifications")).toList();
        return sent.isEmpty() ? QVariantMap()
                              : sent.last().toMap().value(QStringLiteral("value")).toMap();
    };
    // What the engine was last given for a preference, or an invalid value if never.
    const auto given = [this, scope](const char *key) {
        const QVariantList all =
            evaluate(scope, QStringLiteral("WebEngineSettings.preferences")).toList();
        for (int i = all.count() - 1; i >= 0; --i) {
            if (all.at(i).toMap().value(QStringLiteral("key")).toString() == QLatin1String(key)) {
                return all.at(i).toMap().value(QStringLiteral("value"));
            }
        }
        return QVariant();
    };
    evaluate(scope,
             QStringLiteral("pageStack.push('%1')")
                 .arg(QUrl::fromLocalFile(QLatin1String(SALAMA_SOURCE_DIR) +
                                          QStringLiteral("/qml/pages/SitePermissionsPage.qml"))
                          .toString()));
    QObject *page = currentPage();
    QCOMPARE(page->objectName(), QStringLiteral("sitePermissionsPage"));
    // Opened, it asks the engine for what it keeps.
    QCOMPARE(lastToEngine().value(QStringLiteral("msg")).toString(), QStringLiteral("get-all"));

    // What the page says of itself, and the rows in the order Settings lists the kinds.
    QCOMPARE(find(QStringLiteral("sitePermissionsHint"))->property("text").toString(),
             QStringLiteral("What sites may do unless you decided otherwise for a site. Tap one "
                            "to change it or see the exceptions."));
    const auto row = [this](const char *name) { return find(QLatin1String(name)); };
    const auto value = [&row](const char *name) {
        return textOf(row(name), "sitePermissionValue");
    };
    const auto description = [&row](const char *name) {
        return textOf(row(name), "sitePermissionDescription");
    };
    QCOMPARE(textOf(row("notificationsPermissionRow"), "sitePermissionName"),
             QStringLiteral("Notifications"));
    QCOMPARE(textOf(row("popupsPermissionRow"), "sitePermissionName"), QStringLiteral("Pop-ups"));
    QCOMPARE(textOf(row("locationPermissionRow"), "sitePermissionName"),
             QStringLiteral("Location"));
    QCOMPARE(textOf(row("cameraPermissionRow"), "sitePermissionName"), QStringLiteral("Camera"));
    QCOMPARE(textOf(row("microphonePermissionRow"), "sitePermissionName"),
             QStringLiteral("Microphone"));
    QCOMPARE(findObjects(row("popupsPermissionRow"), QStringLiteral("sitePermissionIcon"))
                 .first()
                 ->property("source")
                 .toString(),
             QStringLiteral("image://theme/icon-m-browser-popup"));
    QCOMPARE(findObjects(row("notificationsPermissionRow"), QStringLiteral("sitePermissionIcon"))
                 .first()
                 ->property("source")
                 .toString(),
             QStringLiteral("image://theme/icon-m-notifications"));
    QCOMPARE(findObjects(row("locationPermissionRow"), QStringLiteral("sitePermissionIcon"))
                 .first()
                 ->property("source")
                 .toString(),
             QStringLiteral("image://theme/icon-m-browser-location"));

    // As Firefox has them: pop-ups blocked, the rest asked, and nothing an exception.
    QCOMPARE(value("notificationsPermissionRow"), QStringLiteral("Ask"));
    QCOMPARE(value("popupsPermissionRow"), QStringLiteral("Block"));
    QCOMPARE(value("locationPermissionRow"), QStringLiteral("Ask"));
    QCOMPARE(value("cameraPermissionRow"), QStringLiteral("Ask"));
    QCOMPARE(value("microphonePermissionRow"), QStringLiteral("Ask"));
    for (const char *name :
         {"notificationsPermissionRow", "popupsPermissionRow", "locationPermissionRow",
          "cameraPermissionRow", "microphonePermissionRow"}) {
        QCOMPARE(description(name), QStringLiteral("No exceptions"));
    }
    // Cookies are tracking protection's while it is on (Standard), and nothing is turned
    // off for any site.
    QVERIFY(!shownIn(page, "cookiesPermissionRow"));
    QVERIFY(!shownIn(page, "trackingExceptionsSection"));
    QVERIFY(!shownIn(page, "trackingPermissionRow"));

    // The engine's list: the notifications' counts come from their own model, the others
    // from the site permissions.
    evaluate(
        scope,
        permissionList(
            {{QStringLiteral("desktop-notification"), QStringLiteral("https://a.example"),
              QStringLiteral("1")},
             {QStringLiteral("desktop-notification"), QStringLiteral("https://b.example"),
              QStringLiteral("1")},
             {QStringLiteral("desktop-notification"), QStringLiteral("https://c.example"),
              QStringLiteral("2")},
             {QStringLiteral("popup"), QStringLiteral("https://op.example"), QStringLiteral("1")},
             {QStringLiteral("camera"), QStringLiteral("https://cam.example"), QStringLiteral("2")},
             {QStringLiteral("camera"), QStringLiteral("https://cam2.example"),
              QStringLiteral("1")},
             {QStringLiteral("trackingprotection"), QStringLiteral("https://tp.example"),
              QStringLiteral("1")}}));
    QCOMPARE(description("notificationsPermissionRow"),
             QStringLiteral("2 site(s) allowed · 1 blocked"));
    QCOMPARE(description("popupsPermissionRow"), QStringLiteral("1 exception(s)"));
    QCOMPARE(description("cameraPermissionRow"), QStringLiteral("2 exception(s)"));
    QCOMPARE(description("locationPermissionRow"), QStringLiteral("No exceptions"));
    QVERIFY(shownIn(page, "trackingExceptionsSection"));
    QVERIFY(shownIn(page, "trackingPermissionRow"));
    QCOMPARE(value("trackingPermissionRow"), QStringLiteral("Off for 1 site(s)"));
    QCOMPARE(description("trackingPermissionRow"),
             QStringLiteral("Turned off from a site’s details"));

    // The choices of a default: pop-ups allowed or blocked, written as they are made, and
    // the engine given the preference.
    QObject *popups = row("popupsPermissionRow");
    const QList<QObject *> popupChoices =
        findObjects(popups, QStringLiteral("sitePermissionChoice"));
    QCOMPARE(popupChoices.count(), 2);
    QCOMPARE(popupChoices.at(0)->property("text").toString(), QStringLiteral("Allow"));
    QCOMPARE(popupChoices.at(1)->property("text").toString(), QStringLiteral("Block"));
    QCOMPARE(findObjects(popups, QStringLiteral("sitePermissionShowExceptions"))
                 .first()
                 ->property("text")
                 .toString(),
             QStringLiteral("Show exceptions"));
    click(popups);
    QVERIFY(popups->property("menuOpen").toBool());
    click(popupChoices.at(0));
    QVERIFY(defaults->popupsAllowed());
    QCOMPARE(value("popupsPermissionRow"), QStringLiteral("Allow"));
    QCOMPARE(given("dom.disable_open_during_load"), QVariant(false));
    click(popupChoices.at(1));
    QVERIFY(!defaults->popupsAllowed());
    QCOMPARE(given("dom.disable_open_during_load"), QVariant(true));

    // Location, camera and microphone are asked about or refused outright, each alone.
    const QList<QObject *> cameraChoices =
        findObjects(row("cameraPermissionRow"), QStringLiteral("sitePermissionChoice"));
    QCOMPARE(cameraChoices.at(0)->property("text").toString(), QStringLiteral("Ask"));
    QCOMPARE(cameraChoices.at(1)->property("text").toString(), QStringLiteral("Block"));
    click(cameraChoices.at(1));
    QVERIFY(defaults->cameraBlocked());
    QVERIFY(!defaults->locationBlocked());
    QVERIFY(!defaults->microphoneBlocked());
    QCOMPARE(value("cameraPermissionRow"), QStringLiteral("Block"));
    QCOMPARE(value("locationPermissionRow"), QStringLiteral("Ask"));
    QCOMPARE(given("permissions.default.camera"), QVariant(2));
    QCOMPARE(given("permissions.default.microphone"), QVariant(0));
    QCOMPARE(given("permissions.default.geo"), QVariant(0));
    click(findObjects(row("locationPermissionRow"), QStringLiteral("sitePermissionChoice")).at(1));
    click(
        findObjects(row("microphonePermissionRow"), QStringLiteral("sitePermissionChoice")).at(1));
    QVERIFY(defaults->locationBlocked());
    QVERIFY(defaults->microphoneBlocked());
    QCOMPARE(given("permissions.default.geo"), QVariant(2));
    QCOMPARE(given("permissions.default.geolocation"), QVariant(2));
    QCOMPARE(given("permissions.default.microphone"), QVariant(2));
    click(cameraChoices.at(0));
    QVERIFY(!defaults->cameraBlocked());
    QCOMPARE(given("permissions.default.camera"), QVariant(0));
}

// The cookies on Settings > Site permissions are tracking protection's while it is on:
// the row comes with it off, or with a site that has an exception to them, and what is
// chosen there is the engine's cookie behaviour while tracking protection is off. The
// rows with no choices to make are ways on (docs/DECISIONS/0039-site-permissions.md).
void tst_qmlload::sitePermissionsCookiesAndWaysOn()
{
    SitePermissionSettings *defaults = m_core->sitePermissionSettings();
    PrivacySettings *privacy = m_core->privacySettings();
    QObject *scope = find(QStringLiteral("viewArea"));
    evaluate(scope,
             QStringLiteral("pageStack.push('%1')")
                 .arg(QUrl::fromLocalFile(QLatin1String(SALAMA_SOURCE_DIR) +
                                          QStringLiteral("/qml/pages/SitePermissionsPage.qml"))
                          .toString()));
    QObject *page = currentPage();
    QCOMPARE(page->objectName(), QStringLiteral("sitePermissionsPage"));
    const auto row = [this](const char *name) { return find(QLatin1String(name)); };
    const auto value = [&row](const char *name) {
        return textOf(row(name), "sitePermissionValue");
    };
    const auto description = [&row](const char *name) {
        return textOf(row(name), "sitePermissionDescription");
    };

    // The cookies row comes with tracking protection off, with the choices of three, and
    // says why it is there; the choice is what the engine's cookie behaviour is while it
    // is off.
    privacy->setTrackingProtection(PrivacySettings::TrackingProtectionOff);
    QVERIFY(shownIn(page, "cookiesPermissionRow"));
    QCOMPARE(textOf(row("cookiesPermissionRow"), "sitePermissionName"), QStringLiteral("Cookies"));
    QCOMPARE(value("cookiesPermissionRow"), QStringLiteral("Block cross-site"));
    QCOMPARE(description("cookiesPermissionRow"),
             QStringLiteral("Shown while tracking protection is off · No exceptions"));
    QCOMPARE(findObjects(row("cookiesPermissionRow"), QStringLiteral("sitePermissionIcon"))
                 .first()
                 ->property("source")
                 .toString(),
             QStringLiteral("image://theme/icon-m-browser-cookies"));
    const QList<QObject *> cookieChoices =
        findObjects(row("cookiesPermissionRow"), QStringLiteral("sitePermissionChoice"));
    QCOMPARE(cookieChoices.count(), 3);
    QCOMPARE(cookieChoices.at(0)->property("text").toString(), QStringLiteral("Allow all"));
    QCOMPARE(cookieChoices.at(1)->property("text").toString(), QStringLiteral("Block cross-site"));
    QCOMPARE(cookieChoices.at(2)->property("text").toString(), QStringLiteral("Block all"));
    const auto cookieBehavior = [this, scope]() {
        const QVariantList given =
            evaluate(scope, QStringLiteral("WebEngineSettings.preferences")).toList();
        for (int i = given.count() - 1; i >= 0; --i) {
            if (given.at(i).toMap().value(QStringLiteral("key")).toString() ==
                QLatin1String("network.cookie.cookieBehavior")) {
                return given.at(i).toMap().value(QStringLiteral("value")).toInt();
            }
        }
        return -1;
    };
    QCOMPARE(cookieBehavior(), 1);
    click(cookieChoices.at(2));
    QCOMPARE(defaults->cookies(), int(SitePermissionSettings::CookiesBlockAll));
    QCOMPARE(value("cookiesPermissionRow"), QStringLiteral("Block all"));
    QCOMPARE(cookieBehavior(), 2);
    click(cookieChoices.at(0));
    QCOMPARE(cookieBehavior(), 0);
    // Standard is Firefox's: its own cookie behaviour, whatever was chosen.
    privacy->setTrackingProtection(PrivacySettings::TrackingProtectionStandard);
    QCOMPARE(cookieBehavior(), 5);
    QVERIFY(!shownIn(page, "cookiesPermissionRow"));
    // A site with an exception to cookies brings the row back, and its count.
    evaluate(scope, permissionList({{QStringLiteral("cookie"), QStringLiteral("https://c.example"),
                                     QStringLiteral("1")}}));
    QVERIFY(shownIn(page, "cookiesPermissionRow"));
    QCOMPARE(description("cookiesPermissionRow"), QStringLiteral("1 exception(s)"));

    // A tap with no choices to make is a way on: the notifications' page, and the sites
    // tracking protection was turned off for; the menu's last item goes to the
    // exceptions of the row's kind.
    evaluate(scope, permissionList({{QStringLiteral("trackingprotection"),
                                     QStringLiteral("https://tp.example"), QStringLiteral("1")},
                                    {QStringLiteral("popup"), QStringLiteral("https://op.example"),
                                     QStringLiteral("1")}}));
    click(row("notificationsPermissionRow"));
    QCOMPARE(currentPage()->objectName(), QStringLiteral("notificationSettingsPage"));
    popPage();
    click(row("trackingPermissionRow"));
    QCOMPARE(currentPage()->objectName(), QStringLiteral("siteExceptionsPage"));
    QCOMPARE(currentPage()->property("kind").toInt(), int(SitePermissions::TrackingProtection));
    popPage();
    click(findObjects(row("popupsPermissionRow"), QStringLiteral("sitePermissionShowExceptions"))
              .first());
    QCOMPARE(currentPage()->objectName(), QStringLiteral("siteExceptionsPage"));
    QCOMPARE(currentPage()->property("kind").toInt(), int(SitePermissions::Popups));
    popPage();
    QCOMPARE(currentPage()->objectName(), QStringLiteral("sitePermissionsPage"));
}

// The exceptions to one kind of permission: the allowed under a heading and the blocked
// under another, each with a menu to change it or remove it, and the pull-down menu that
// adds a site by its address and removes them all (0039-site-permissions.md).
void tst_qmlload::siteExceptionsPage()
{
    SitePermissions *sites = m_core->sitePermissions();
    QObject *scope = find(QStringLiteral("viewArea"));
    const auto lastToEngine = [this, scope]() {
        const QVariantList sent =
            evaluate(scope, QStringLiteral("WebEngine.notifications")).toList();
        return sent.isEmpty() ? QVariantMap()
                              : sent.last().toMap().value(QStringLiteral("value")).toMap();
    };
    const auto openPage = [this, scope](int kind) {
        evaluate(scope,
                 QStringLiteral("pageStack.push('%1', {kind: %2})")
                     .arg(QUrl::fromLocalFile(QLatin1String(SALAMA_SOURCE_DIR) +
                                              QStringLiteral("/qml/pages/SiteExceptionsPage.qml"))
                              .toString())
                     .arg(kind));
        return currentPage();
    };
    QObject *page = openPage(SitePermissions::Popups);
    QCOMPARE(page->objectName(), QStringLiteral("siteExceptionsPage"));
    QCOMPARE(lastToEngine().value(QStringLiteral("msg")).toString(), QStringLiteral("get-all"));
    // Nothing yet: the page says so, and has no footer to say what removing does.
    QVERIFY(findAll(QStringLiteral("siteException")).isEmpty());
    QVERIFY(!shownIn(page, "siteExceptionsFooter"));
    QVERIFY(!shownIn(page, "removeAllMenuItem"));
    QCOMPARE(textOf(page, "addSiteMenuItem"), QStringLiteral("Add a site"));
    QCOMPARE(textOf(page, "removeAllMenuItem"), QStringLiteral("Remove all exceptions"));

    // Header by kind: what it is for every other site, as default.
    QCOMPARE(evaluate(page, QStringLiteral("siteNames.defaultName(kind)")).toString(),
             QStringLiteral("Block"));

    evaluate(
        scope,
        permissionList(
            {{QStringLiteral("popup"), QStringLiteral("https://vr.example"), QStringLiteral("2")},
             {QStringLiteral("popup"), QStringLiteral("https://op.example"), QStringLiteral("1")},
             {QStringLiteral("cookie"), QStringLiteral("https://other.example"),
              QStringLiteral("1")},
             {QStringLiteral("popup"), QStringLiteral("https://iltalehti.example"),
              QStringLiteral("2")}}));
    QList<QObject *> rows = byRow(findAll(QStringLiteral("siteException")));
    QCOMPARE(rows.count(), 3);
    // Allowed first, and each group by host; the cookies' site is another page's.
    QCOMPARE(textOf(rows.at(0), "siteExceptionHost"), QStringLiteral("op.example"));
    QCOMPARE(textOf(rows.at(1), "siteExceptionHost"), QStringLiteral("iltalehti.example"));
    QCOMPARE(textOf(rows.at(2), "siteExceptionHost"), QStringLiteral("vr.example"));
    // Each row offers the decisions it has not got; pop-ups are not asked about.
    const auto offered = [](QObject *row) {
        QStringList texts;
        for (const char *name : {"siteExceptionAllow", "siteExceptionBlock", "siteExceptionAsk"}) {
            if (shownIn(row, name)) {
                texts.append(textOf(row, name));
            }
        }
        return texts;
    };
    QCOMPARE(offered(rows.at(0)), QStringList{QStringLiteral("Block")});
    QCOMPARE(offered(rows.at(1)), QStringList{QStringLiteral("Allow")});
    QCOMPARE(textOf(rows.at(0), "siteExceptionRemove"), QStringLiteral("Remove"));
    QCOMPARE(rows.at(0)->property("decision").toInt(), int(SitePermissions::Allow));
    QCOMPARE(rows.at(1)->property("decision").toInt(), int(SitePermissions::Block));
    QObject *list = find(QStringLiteral("siteExceptionList"));
    QCOMPARE(evaluate(list, QStringLiteral("section.property")).toString(),
             QStringLiteral("decision"));
    QStringList headings;
    for (QObject *heading : findAll(QStringLiteral("siteExceptionSection"))) {
        headings.append(heading->property("text").toString());
    }
    headings.sort();
    QCOMPARE(headings, (QStringList{QStringLiteral("Allowed"), QStringLiteral("Blocked")}));
    QCOMPARE(find(QStringLiteral("siteExceptionsFooter"))->property("text").toString(),
             QStringLiteral("A site you remove follows the default again."));
    QVERIFY(shownIn(page, "removeAllMenuItem"));

    // Blocked from the menu: the engine told, and the row moves under the other heading.
    click(findObjects(rows.at(0), QStringLiteral("siteExceptionBlock")).first());
    QCOMPARE(sites->decision(SitePermissions::Popups, QStringLiteral("https://op.example")),
             int(SitePermissions::Block));
    QCOMPARE(lastToEngine().value(QStringLiteral("msg")).toString(), QStringLiteral("add"));
    QCOMPARE(lastToEngine().value(QStringLiteral("type")).toString(), QStringLiteral("popup"));
    QCOMPARE(lastToEngine().value(QStringLiteral("permission")).toInt(), 2);
    rows = byRow(findAll(QStringLiteral("siteException")));
    QCOMPARE(rows.count(), 3);
    for (QObject *row : rows) {
        QCOMPARE(row->property("decision").toInt(), int(SitePermissions::Block));
    }
    QCOMPARE(textOf(rows.at(0), "siteExceptionHost"), QStringLiteral("iltalehti.example"));
    // Removed: it follows the default again, and the page of another kind does not see it.
    click(findObjects(rows.at(1), QStringLiteral("siteExceptionRemove")).first());
    QCOMPARE(lastToEngine().value(QStringLiteral("msg")).toString(), QStringLiteral("remove"));
    QCOMPARE(lastToEngine().value(QStringLiteral("uri")).toString(),
             QStringLiteral("https://op.example"));
    QCOMPARE(findAll(QStringLiteral("siteException")).count(), 2);

    // Adding a site: its address has to begin as an origin does, and the choice is
    // allowing or blocking it.
    click(find(QStringLiteral("addSiteMenuItem")));
    QObject *dialog = currentPage();
    QCOMPARE(dialog->objectName(), QStringLiteral("siteExceptionDialog"));
    QObject *address = find(QStringLiteral("siteExceptionAddress"));
    QCOMPARE(address->property("text").toString(), QStringLiteral("https://"));
    QVERIFY(!address->property("errorHighlight").toBool());
    QVERIFY(!dialog->property("canAccept").toBool());
    address->setProperty("text", QStringLiteral("ftp://files.example"));
    QVERIFY(address->property("errorHighlight").toBool());
    QCOMPARE(address->property("label").toString(),
             QStringLiteral("Must begin with http:// or https://"));
    QVERIFY(!dialog->property("canAccept").toBool());
    address->setProperty("text", QStringLiteral("https://New.Example/path"));
    QVERIFY(!address->property("errorHighlight").toBool());
    QCOMPARE(dialog->property("origin").toString(), QStringLiteral("https://new.example"));
    QVERIFY(dialog->property("canAccept").toBool());
    QObject *decision = find(QStringLiteral("siteExceptionDecision"));
    QVERIFY(shownIn(dialog, "siteExceptionDecision"));
    QCOMPARE(decision->property("label").toString(), QStringLiteral("Pop-ups"));
    decision->setProperty("currentIndex", 1);
    QMetaObject::invokeMethod(dialog, "accept");
    QCOMPARE(sites->decision(SitePermissions::Popups, QStringLiteral("https://new.example")),
             int(SitePermissions::Block));
    QCOMPARE(lastToEngine().value(QStringLiteral("uri")).toString(),
             QStringLiteral("https://new.example"));
    QCOMPARE(lastToEngine().value(QStringLiteral("permission")).toInt(), 2);
    popPage();
    QCOMPARE(findAll(QStringLiteral("siteException")).count(), 3);

    // Allowed is the default choice of the dialog, as Silica's combo box starts at its
    // first item; a site of the dialog that is not one is not added.
    click(find(QStringLiteral("addSiteMenuItem")));
    dialog = currentPage();
    find(QStringLiteral("siteExceptionAddress"))
        ->setProperty("text", QStringLiteral("https://ok.example"));
    find(QStringLiteral("siteExceptionDecision"))->setProperty("currentIndex", 0);
    QMetaObject::invokeMethod(dialog, "accept");
    QCOMPARE(sites->decision(SitePermissions::Popups, QStringLiteral("https://ok.example")),
             int(SitePermissions::Allow));
    popPage();
    click(find(QStringLiteral("addSiteMenuItem")));
    dialog = currentPage();
    find(QStringLiteral("siteExceptionAddress"))->setProperty("text", QStringLiteral("nonsense"));
    QMetaObject::invokeMethod(dialog, "accept");
    QCOMPARE(sites->count(SitePermissions::Popups), 4);
    popPage();

    // Removing them all, after the remorse: every site of the kind, and no other kind's.
    QCOMPARE(evaluate(page, QStringLiteral("Remorse.popupCount")).toInt(), 0);
    click(find(QStringLiteral("removeAllMenuItem")));
    QCOMPARE(evaluate(page, QStringLiteral("Remorse.popupCount")).toInt(), 1);
    QCOMPARE(evaluate(page, QStringLiteral("Remorse.popupText")).toString(),
             QStringLiteral("Removing exceptions"));
    QCOMPARE(sites->count(SitePermissions::Popups), 0);
    QCOMPARE(sites->count(SitePermissions::Cookies), 1);
    QVERIFY(findAll(QStringLiteral("siteException")).isEmpty());
    QVERIFY(!shownIn(page, "siteExceptionsFooter"));
    popPage();

    // Tracking protection's list is the sites it is off for: one heading, no blocking,
    // nothing to change but taking a site off the list.
    sites->observe(
        QStringLiteral("embed:perms:all"),
        QVariantList{QVariantMap{{QStringLiteral("type"), QStringLiteral("trackingprotection")},
                                 {QStringLiteral("uri"), QStringLiteral("https://tp.example")},
                                 {QStringLiteral("capability"), 1},
                                 {QStringLiteral("expireType"), 0}}});
    page = openPage(SitePermissions::TrackingProtection);
    rows = findAll(QStringLiteral("siteException"));
    QCOMPARE(rows.count(), 1);
    QVERIFY(offered(rows.first()).isEmpty());
    QVERIFY(shownIn(rows.first(), "siteExceptionRemove"));
    QCOMPARE(textOf(page, "siteExceptionSection"), QStringLiteral("Tracking protection off"));
    click(findObjects(rows.first(), QStringLiteral("siteExceptionRemove")).first());
    QCOMPARE(sites->count(SitePermissions::TrackingProtection), 0);
    QCOMPARE(lastToEngine().value(QStringLiteral("type")).toString(),
             QStringLiteral("trackingprotection"));
    // Adding one is allowing it, with no choice to make.
    click(find(QStringLiteral("addSiteMenuItem")));
    dialog = currentPage();
    QVERIFY(!shownIn(dialog, "siteExceptionDecision"));
    find(QStringLiteral("siteExceptionAddress"))
        ->setProperty("text", QStringLiteral("https://tp2.example"));
    QMetaObject::invokeMethod(dialog, "accept");
    QCOMPARE(
        sites->decision(SitePermissions::TrackingProtection, QStringLiteral("https://tp2.example")),
        int(SitePermissions::Allow));
}

// A kind a page asks for: its exceptions page lists the sites asked each time under a
// heading of their own, and asking is one of each row's choices and the add dialog's
// (docs/DECISIONS/0040-site-details.md).
void tst_qmlload::siteExceptionsAskEachTime()
{
    SitePermissions *sites = m_core->sitePermissions();
    QObject *scope = find(QStringLiteral("viewArea"));
    const auto lastToEngine = [this, scope]() {
        const QVariantList sent =
            evaluate(scope, QStringLiteral("WebEngine.notifications")).toList();
        return sent.isEmpty() ? QVariantMap()
                              : sent.last().toMap().value(QStringLiteral("value")).toMap();
    };
    const auto openPage = [this, scope](int kind) {
        evaluate(scope,
                 QStringLiteral("pageStack.push('%1', {kind: %2})")
                     .arg(QUrl::fromLocalFile(QLatin1String(SALAMA_SOURCE_DIR) +
                                              QStringLiteral("/qml/pages/SiteExceptionsPage.qml"))
                              .toString())
                     .arg(kind));
        return currentPage();
    };
    sites->observe(
        QStringLiteral("embed:perms:all"),
        QVariantList{QVariantMap{{QStringLiteral("type"), QStringLiteral("geo")},
                                 {QStringLiteral("uri"), QStringLiteral("https://ask.example")},
                                 {QStringLiteral("capability"), 3},
                                 {QStringLiteral("expireType"), 0}},
                     QVariantMap{{QStringLiteral("type"), QStringLiteral("geo")},
                                 {QStringLiteral("uri"), QStringLiteral("https://map.example")},
                                 {QStringLiteral("capability"), 1},
                                 {QStringLiteral("expireType"), 0}}});
    openPage(SitePermissions::Location);
    const QList<QObject *> rows = byRow(findAll(QStringLiteral("siteException")));
    QCOMPARE(rows.count(), 2);
    QCOMPARE(textOf(rows.at(0), "siteExceptionHost"), QStringLiteral("map.example"));
    QCOMPARE(textOf(rows.at(1), "siteExceptionHost"), QStringLiteral("ask.example"));
    QCOMPARE(textOf(rows.at(0), "siteExceptionAsk"), QStringLiteral("Always ask"));
    QVERIFY(!shownIn(rows.at(1), "siteExceptionAsk"));
    QVERIFY(shownIn(rows.at(1), "siteExceptionAllow"));
    QVERIFY(shownIn(rows.at(1), "siteExceptionBlock"));
    QStringList askHeadings;
    for (QObject *heading : findAll(QStringLiteral("siteExceptionSection"))) {
        askHeadings.append(heading->property("text").toString());
    }
    askHeadings.sort();
    QCOMPARE(askHeadings, (QStringList{QStringLiteral("Allowed"), QStringLiteral("Always ask")}));
    click(findObjects(rows.at(0), QStringLiteral("siteExceptionAsk")).first());
    QCOMPARE(sites->decision(SitePermissions::Location, QStringLiteral("https://map.example")),
             int(SitePermissions::Ask));
    QCOMPARE(lastToEngine().value(QStringLiteral("permission")).toInt(), 3);
    // Added from the pulley, the third choice is asking each time.
    click(find(QStringLiteral("addSiteMenuItem")));
    QObject *askDialog = currentPage();
    QVERIFY(shownIn(askDialog, "siteExceptionAskChoice"));
    find(QStringLiteral("siteExceptionAddress"))
        ->setProperty("text", QStringLiteral("https://new.example"));
    find(QStringLiteral("siteExceptionDecision"))->setProperty("currentIndex", 2);
    QMetaObject::invokeMethod(askDialog, "accept");
    QCOMPARE(sites->decision(SitePermissions::Location, QStringLiteral("https://new.example")),
             int(SitePermissions::Ask));
    // The dialog, and the page under it.
    evaluate(scope, QStringLiteral("pageStack.pop(null, PageStackAction.Immediate)"));
    evaluate(scope, QStringLiteral("pageStack.pop(null, PageStackAction.Immediate)"));
}

// The head of the menu sheet is the way to the site's details, which the chevron after
// the title says is there; the copy button at its right keeps its own tap, and on the
// start page, where there is no site, there is no way in
// (docs/DECISIONS/0040-site-details.md).
void tst_qmlload::menuHeadOpensSiteDetails()
{
    QObject *menu = find(QStringLiteral("browserMenu"));
    m_core->tabs()->updateTitle(m_core->tabs()->activeTabId(), QStringLiteral("Qwant"));
    tapBar(QStringLiteral("menu"));
    QVERIFY(menu->property("open").toBool());
    QVERIFY(shownIn(menu, "menuHeaderChevron"));
    QCOMPARE(findObjects(menu, QStringLiteral("menuHeaderChevron"))
                 .first()
                 ->property("source")
                 .toString(),
             QStringLiteral("image://theme/icon-m-right"));
    QVERIFY(findObjects(menu, QStringLiteral("menuHeaderDetails"))
                .first()
                ->property("enabled")
                .toBool());

    // The copy button at the right of the head is not a tap on the head.
    click(find(QStringLiteral("copyAddressButton")));
    QCOMPARE(currentPage()->objectName(), QStringLiteral("browserPage"));
    QVERIFY(!menu->property("open").toBool());

    tapBar(QStringLiteral("menu"));
    click(find(QStringLiteral("menuHeaderDetails")));
    QObject *details = currentPage();
    QCOMPARE(details->objectName(), QStringLiteral("siteDetailsPage"));
    QVERIFY(!menu->property("open").toBool());
    // What the sheet's head had: the page, and the view that shows it.
    QCOMPARE(details->property("url").toString(), QLatin1String(FirstPage));
    QCOMPARE(details->property("title").toString(), QStringLiteral("Qwant"));
    QCOMPARE(details->property("view").value<QObject *>(), currentWebView());
    popPage();

    // The start page has no site: nothing to open, and no chevron to say there is.
    m_core->tabs()->newTab(QString());
    tapBar(QStringLiteral("menu"));
    QVERIFY(!shownIn(menu, "menuHeaderChevron"));
    QVERIFY(!findObjects(menu, QStringLiteral("menuHeaderDetails"))
                 .first()
                 ->property("enabled")
                 .toBool());
}

// A site's details: whether the connection is secure and whose certificate says so, and
// what the engine tells of the certificate (docs/DECISIONS/0040-site-details.md).
void tst_qmlload::siteDetailsConnection()
{
    QObject *scope = find(QStringLiteral("viewArea"));
    const auto security = [this]() {
        return currentWebView()->property("security").value<QObject *>();
    };
    const auto open = [this]() {
        tapBar(QStringLiteral("menu"));
        click(find(QStringLiteral("menuHeaderDetails")));
        return currentPage();
    };
    const auto icon = [](QObject *page, const char *property) {
        return findObjects(page, QStringLiteral("siteSecurityIcon")).first()->property(property);
    };
    m_core->tabs()->updateTitle(m_core->tabs()->activeTabId(),
                                QStringLiteral("Qwant, the search engine"));

    // What the engine says of the certificate.
    security()->setProperty("issuerDisplayName", QStringLiteral("Let's Encrypt (R11)"));
    security()->setProperty("subjectDisplayName", QStringLiteral("www.qwant.com"));
    security()->setProperty("expiryDate", QDateTime(QDate(2026, 12, 14), QTime(12, 0)));
    security()->setProperty("protocolVersion", 4);
    security()->setProperty("cipherName", QStringLiteral("TLS_AES_128_GCM_SHA256"));
    QObject *page = open();
    QCOMPARE(page->objectName(), QStringLiteral("siteDetailsPage"));
    // Asked of the engine, as the notifications' page asks.
    const QVariantList sent = evaluate(scope, QStringLiteral("WebEngine.notifications")).toList();
    QCOMPARE(
        sent.last().toMap().value(QStringLiteral("value")).toMap().value(QStringLiteral("msg")),
        QVariant(QStringLiteral("get-all")));
    QCOMPARE(page->property("origin").toString(), QStringLiteral("https://www.qwant.com"));

    // Secure: the padlock, and who vouches for it.
    QCOMPARE(textOf(page, "siteSecurityTitle"), QStringLiteral("Connection is secure"));
    QCOMPARE(textOf(page, "siteSecurityDetail"), QStringLiteral("Verified by Let's Encrypt (R11)"));
    QCOMPARE(icon(page, "source").toString(), QStringLiteral("image://theme/icon-m-device-lock"));
    QVERIFY(!shownIn(page, "siteSecurityWarning"));
    const auto detail = [page](const char *name, const char *property) {
        return findObjects(page, QLatin1String(name)).first()->property(property).toString();
    };
    QVERIFY(shownIn(page, "siteConnectionDetails"));
    QCOMPARE(detail("siteIssuedTo", "label"), QStringLiteral("Issued to"));
    QCOMPARE(detail("siteIssuedTo", "value"), QStringLiteral("www.qwant.com"));
    QCOMPARE(detail("siteVerifiedBy", "label"), QStringLiteral("Verified by"));
    QCOMPARE(detail("siteVerifiedBy", "value"), QStringLiteral("Let's Encrypt (R11)"));
    QCOMPARE(detail("siteValidUntil", "label"), QStringLiteral("Valid until"));
    QCOMPARE(detail("siteValidUntil", "value"), QStringLiteral("14 Dec 2026"));
    QCOMPARE(detail("siteProtocol", "label"), QStringLiteral("Protocol"));
    QCOMPARE(detail("siteProtocol", "value"), QStringLiteral("TLS 1.3"));
    QCOMPARE(detail("siteCipherSuite", "label"), QStringLiteral("Cipher suite"));
    QCOMPARE(detail("siteCipherSuite", "value"), QStringLiteral("TLS_AES_128_GCM_SHA256"));

    // A line the engine has nothing for is not drawn; with nothing at all, nor is the
    // heading.
    security()->setProperty("cipherName", QString());
    security()->setProperty("protocolVersion", -1);
    QVERIFY(!shownIn(page, "siteCipherSuite"));
    QVERIFY(!shownIn(page, "siteProtocol"));
    QVERIFY(shownIn(page, "siteValidUntil"));
    security()->setProperty("protocolVersion", 3);
    QCOMPARE(detail("siteProtocol", "value"), QStringLiteral("TLS 1.2"));
    security()->setProperty("protocolVersion", 0);
    QCOMPARE(detail("siteProtocol", "value"), QStringLiteral("SSL 3.0"));
    security()->setProperty("protocolVersion", -1);
    security()->setProperty("expiryDate", QVariant());
    security()->setProperty("subjectDisplayName", QString());
    security()->setProperty("issuerDisplayName", QString());
    QVERIFY(!shownIn(page, "siteIssuedTo"));
    QVERIFY(!shownIn(page, "siteValidUntil"));
    QVERIFY(!shownIn(page, "siteConnectionDetails"));
    // Verified by nobody: the title alone.
    QVERIFY(!shownIn(page, "siteSecurityDetail"));
    security()->setProperty("issuerDisplayName", QStringLiteral("Let's Encrypt (R11)"));
    security()->setProperty("subjectDisplayName", QStringLiteral("www.qwant.com"));

    // Broken: the warning in the error colour, the reason, and what not to do on the site.
    const QColor error = evaluate(page, QStringLiteral("Theme.errorColor")).value<QColor>();
    security()->setProperty("allGood", false);
    security()->setProperty("notValidAtThisTime", true);
    QCOMPARE(textOf(page, "siteSecurityTitle"), QStringLiteral("Connection is not secure"));
    QCOMPARE(textOf(page, "siteSecurityDetail"),
             QStringLiteral("The certificate has expired or is not yet valid"));
    QCOMPARE(icon(page, "source").toString(), QStringLiteral("image://theme/icon-m-warning"));
    QCOMPARE(icon(page, "color").value<QColor>(), error);
    QCOMPARE(findObjects(page, QStringLiteral("siteSecurityTitle"))
                 .first()
                 ->property("color")
                 .value<QColor>(),
             error);
    QCOMPARE(textOf(page, "siteSecurityWarning"),
             QStringLiteral("Do not enter personal data, passwords, card details on this site"));
    QVERIFY(shownIn(page, "siteSecurityWarning"));
    // The certificate is still described.
    QVERIFY(shownIn(page, "siteConnectionDetails"));
    // The most to the point of the reasons first.
    security()->setProperty("domainMismatch", true);
    QCOMPARE(textOf(page, "siteSecurityDetail"),
             QStringLiteral("The certificate has expired or is not yet valid"));
    security()->setProperty("notValidAtThisTime", false);
    QCOMPARE(textOf(page, "siteSecurityDetail"),
             QStringLiteral("The certificate is for another site"));
    security()->setProperty("domainMismatch", false);
    security()->setProperty("untrusted", true);
    QCOMPARE(textOf(page, "siteSecurityDetail"), QStringLiteral("The certificate is not trusted"));
    // The engine unhappy for none of the reasons it has: not secure, and no reason made up.
    security()->setProperty("untrusted", false);
    QVERIFY(!shownIn(page, "siteSecurityDetail"));
    QCOMPARE(textOf(page, "siteSecurityTitle"), QStringLiteral("Connection is not secure"));
    // No verdict yet is no warning, as the bar's padlock has it.
    security()->setProperty("validState", false);
    QCOMPARE(textOf(page, "siteSecurityTitle"), QStringLiteral("Connection is secure"));
    security()->setProperty("validState", true);
    security()->setProperty("allGood", true);
    QCOMPARE(textOf(page, "siteSecurityTitle"), QStringLiteral("Connection is secure"));
    popPage();

    // Plain http: not secure, and no certificate to describe, whatever the engine says.
    typeAddress(QStringLiteral("http://plain.example/"));
    QCOMPARE(m_core->tabs()->activeUrl(), QStringLiteral("http://plain.example/"));
    page = open();
    QCOMPARE(page->objectName(), QStringLiteral("siteDetailsPage"));
    QCOMPARE(textOf(page, "siteSecurityTitle"), QStringLiteral("Connection is not secure"));
    QVERIFY(!shownIn(page, "siteSecurityDetail"));
    QVERIFY(shownIn(page, "siteSecurityWarning"));
    QCOMPARE(icon(page, "source").toString(), QStringLiteral("image://theme/icon-m-warning"));
    QVERIFY(!shownIn(page, "siteConnectionDetails"));
    // A site all the same: its permissions are drawn, for its own origin.
    QCOMPARE(page->property("origin").toString(), QStringLiteral("http://plain.example"));
    QCOMPARE(findAll(QStringLiteral("siteDecisionRow")).count(), 6);
    popPage();

    // An engine with no security to ask, or a view gone since the page was opened: drawn
    // as the padlock in the bar is, with nothing of the certificate, and nothing amiss.
    typeAddress(QStringLiteral("https://secure.example/"));
    currentWebView()->setProperty("security", QVariant::fromValue<QObject *>(nullptr));
    page = open();
    QCOMPARE(textOf(page, "siteSecurityTitle"), QStringLiteral("Connection is secure"));
    QVERIFY(!shownIn(page, "siteSecurityDetail"));
    QVERIFY(!shownIn(page, "siteConnectionDetails"));
    QVERIFY(!shownIn(page, "siteTrackersBlocked"));
    QVERIFY(!page->property("tlsBroken").toBool());
    page->setProperty("view", QVariant::fromValue<QObject *>(nullptr));
    QCOMPARE(textOf(page, "siteSecurityTitle"), QStringLiteral("Connection is secure"));
    QVERIFY(!shownIn(page, "siteConnectionDetails"));
    popPage();
}

// Tracking protection for the one site: a switch that adds the site to the engine's
// allow list or takes it off, loading the page again; and off in Settings, off for every
// site, and said so (docs/DECISIONS/0040-site-details.md).
void tst_qmlload::siteDetailsTrackingProtection()
{
    SitePermissions *sites = m_core->sitePermissions();
    PrivacySettings *privacy = m_core->privacySettings();
    QObject *scope = find(QStringLiteral("viewArea"));
    const QString site = QStringLiteral("https://www.qwant.com");
    const auto lastToEngine = [this, scope]() {
        const QVariantList sent =
            evaluate(scope, QStringLiteral("WebEngine.notifications")).toList();
        return sent.isEmpty() ? QVariantMap()
                              : sent.last().toMap().value(QStringLiteral("value")).toMap();
    };
    const auto reloads = [this]() {
        return currentWebView()->property("calls").toStringList().count(QStringLiteral("reload"));
    };
    tapBar(QStringLiteral("menu"));
    click(find(QStringLiteral("menuHeaderDetails")));
    QObject *page = currentPage();
    QCOMPARE(page->objectName(), QStringLiteral("siteDetailsPage"));
    QObject *toggle = find(QStringLiteral("siteTrackingSwitch"));
    QCOMPARE(toggle->property("text").toString(), QStringLiteral("Tracking protection"));
    QVERIFY(!toggle->property("automaticCheck").toBool());

    // On, at either level: one line on what to do if the site looks broken.
    QVERIFY(toggle->property("checked").toBool());
    QVERIFY(toggle->property("enabled").toBool());
    const QString onLine =
        QStringLiteral("If something looks broken on this site, try turning this off.");
    QCOMPARE(toggle->property("description").toString(), onLine);
    privacy->setTrackingProtection(PrivacySettings::TrackingProtectionStrict);
    QCOMPARE(toggle->property("description").toString(), onLine);
    privacy->setTrackingProtection(PrivacySettings::TrackingProtectionStandard);

    // The engine blocked trackers on the page: said while it is on.
    auto *security = currentWebView()->property("security").value<QObject *>();
    QVERIFY(!shownIn(page, "siteTrackersBlocked"));
    security->setProperty("blockedTrackingContent", true);
    QVERIFY(shownIn(page, "siteTrackersBlocked"));
    QCOMPARE(textOf(page, "siteTrackersBlocked"),
             QStringLiteral("Trackers were blocked on this page"));

    // Turned off for the site: the allow list of the site's origin, and the page loaded
    // again, which is where the engine applies it.
    const int before = reloads();
    click(toggle);
    QCOMPARE(sites->decision(SitePermissions::TrackingProtection, site),
             int(SitePermissions::Allow));
    QCOMPARE(lastToEngine().value(QStringLiteral("msg")).toString(), QStringLiteral("add"));
    QCOMPARE(lastToEngine().value(QStringLiteral("type")).toString(),
             QStringLiteral("trackingprotection"));
    QCOMPARE(lastToEngine().value(QStringLiteral("uri")).toString(), site);
    QCOMPARE(lastToEngine().value(QStringLiteral("permission")).toInt(), 1);
    QCOMPARE(reloads(), before + 1);
    QVERIFY(!toggle->property("checked").toBool());
    QVERIFY(toggle->property("enabled").toBool());
    QCOMPARE(toggle->property("description").toString(),
             QStringLiteral("Off for this site. Turn it on to block trackers here again."));
    // Nothing was blocked while it was off.
    QVERIFY(!shownIn(page, "siteTrackersBlocked"));
    // The site is on the page of the sites it is off for.
    QCOMPARE(sites->count(SitePermissions::TrackingProtection), 1);

    // And on again: the allow list entry goes, and the page is loaded again.
    click(toggle);
    QCOMPARE(sites->decision(SitePermissions::TrackingProtection, site),
             int(SitePermissions::Default));
    QCOMPARE(lastToEngine().value(QStringLiteral("msg")).toString(), QStringLiteral("remove"));
    QCOMPARE(lastToEngine().value(QStringLiteral("type")).toString(),
             QStringLiteral("trackingprotection"));
    QCOMPARE(reloads(), before + 2);
    QVERIFY(toggle->property("checked").toBool());
    QVERIFY(shownIn(page, "siteTrackersBlocked"));

    // Off in Settings it is off for every site: the switch is off, dimmed, and says why.
    privacy->setTrackingProtection(PrivacySettings::TrackingProtectionOff);
    QVERIFY(!toggle->property("checked").toBool());
    QVERIFY(!toggle->property("enabled").toBool());
    QCOMPARE(toggle->property("description").toString(), QStringLiteral("Off in Settings"));
    QVERIFY(!shownIn(page, "siteTrackersBlocked"));
    privacy->setTrackingProtection(PrivacySettings::TrackingProtectionStandard);
    QVERIFY(toggle->property("checked").toBool());

    // A view gone since the page was opened is not reloaded, and the change is made all the same.
    page->setProperty("view", QVariant::fromValue<QObject *>(nullptr));
    click(find(QStringLiteral("siteTrackingSwitch")));
    QCOMPARE(sites->decision(SitePermissions::TrackingProtection, site),
             int(SitePermissions::Allow));
    QCOMPARE(reloads(), before + 2);
}

// What the site has been given, one row a kind, each changed where it is; the cookies
// only while tracking protection is off, for every site or this one, or when the site
// has an exception to them; and the button that clears them all (0040-site-details.md).
void tst_qmlload::siteDetailsPermissions()
{
    SitePermissions *sites = m_core->sitePermissions();
    PrivacySettings *privacy = m_core->privacySettings();
    SitePermissionSettings *defaults = m_core->sitePermissionSettings();
    QObject *scope = find(QStringLiteral("viewArea"));
    const QString site = QStringLiteral("https://www.qwant.com");
    const auto lastToEngine = [this, scope]() {
        const QVariantList sent =
            evaluate(scope, QStringLiteral("WebEngine.notifications")).toList();
        return sent.isEmpty() ? QVariantMap()
                              : sent.last().toMap().value(QStringLiteral("value")).toMap();
    };
    const auto reloads = [this]() {
        return currentWebView()->property("calls").toStringList().count(QStringLiteral("reload"));
    };
    tapBar(QStringLiteral("menu"));
    click(find(QStringLiteral("menuHeaderDetails")));
    QObject *page = currentPage();
    const auto row = [this](int kind) -> QObject * {
        for (QObject *candidate : findAll(QStringLiteral("siteDecisionRow"))) {
            if (candidate->property("kind").toInt() == kind) {
                return candidate;
            }
        }
        return nullptr;
    };
    const auto rowShown = [&row](int kind) {
        return qobject_cast<QQuickItem *>(row(kind))->isVisible();
    };
    const auto value = [&row](int kind) { return textOf(row(kind), "siteDecisionValue"); };
    // A row following the default says so, and what the default is.
    const auto marked = [&value](int kind) {
        return value(kind).startsWith(QStringLiteral("Follow default: "));
    };
    const auto pulley = [this]() { return find(QStringLiteral("siteDetailsPulley")); };
    const auto pulleyShown = [&pulley]() {
        return qobject_cast<QQuickItem *>(pulley())->isVisible();
    };

    // One row for each kind, in the order Settings lists them, over the section's heading;
    // the cookies are tracking protection's while it is on.
    QCOMPARE(textOf(page, "sitePermissionsSection"), QStringLiteral("Permissions"));
    QCOMPARE(textOf(page, "siteTrackingSection"), QStringLiteral("Tracking protection"));
    QCOMPARE(findAll(QStringLiteral("siteDecisionRow")).count(), 6);
    QStringList names;
    for (QObject *candidate : byRow(findAll(QStringLiteral("siteDecisionRow")))) {
        names.append(textOf(candidate, "siteDecisionName"));
    }
    QCOMPARE(names, (QStringList{QStringLiteral("Notifications"), QStringLiteral("Pop-ups"),
                                 QStringLiteral("Cookies"), QStringLiteral("Location"),
                                 QStringLiteral("Camera"), QStringLiteral("Microphone")}));
    QCOMPARE(findObjects(row(SitePermissions::Camera), QStringLiteral("siteDecisionIcon"))
                 .first()
                 ->property("source")
                 .toString(),
             QStringLiteral("image://theme/icon-m-browser-camera"));
    QVERIFY(!rowShown(SitePermissions::Cookies));
    for (int kind : {int(SitePermissions::Notifications), int(SitePermissions::Popups),
                     int(SitePermissions::Location), int(SitePermissions::Camera),
                     int(SitePermissions::Microphone)}) {
        QVERIFY(rowShown(kind));
        QVERIFY(marked(kind));
    }
    QCOMPARE(value(SitePermissions::Notifications), QStringLiteral("Follow default: Ask"));
    QCOMPARE(value(SitePermissions::Popups), QStringLiteral("Follow default: Block"));
    QCOMPARE(value(SitePermissions::Location), QStringLiteral("Follow default: Ask"));
    // Nothing to clear, so no pulley to pull for nothing.
    QVERIFY(!pulleyShown());
    // The padlock stands clear of the page header.
    QCOMPARE(find(QStringLiteral("siteSecurityHero"))->property("topPadding").toReal(),
             evaluate(page, QStringLiteral("Theme.paddingLarge * 2")).toReal());

    // The choices: allowing, blocking, asking each time where a page asks for it, and
    // following the default, which says what that is.
    const auto choices = [&row](int kind) {
        QStringList texts;
        for (const char *name :
             {"siteDecisionAllow", "siteDecisionBlock", "siteDecisionAsk", "siteDecisionDefault"}) {
            if (shownIn(row(kind), name)) {
                texts.append(textOf(row(kind), name));
            }
        }
        return texts;
    };
    QCOMPARE(choices(SitePermissions::Notifications),
             (QStringList{QStringLiteral("Allow"), QStringLiteral("Block"),
                          QStringLiteral("Always ask"), QStringLiteral("Follow default: Ask")}));
    QCOMPARE(choices(SitePermissions::Location),
             (QStringList{QStringLiteral("Allow"), QStringLiteral("Block"),
                          QStringLiteral("Always ask"), QStringLiteral("Follow default: Ask")}));
    QCOMPARE(choices(SitePermissions::Popups),
             (QStringList{QStringLiteral("Allow"), QStringLiteral("Block"),
                          QStringLiteral("Follow default: Block")}));
    defaults->setCameraBlocked(true);
    QCOMPARE(textOf(row(SitePermissions::Camera), "siteDecisionDefault"),
             QStringLiteral("Follow default: Block"));
    QCOMPARE(value(SitePermissions::Camera), QStringLiteral("Follow default: Block"));

    // Asked each time, whatever the default: the engine's prompt record, which a blocked
    // default does not override.
    click(findObjects(row(SitePermissions::Camera), QStringLiteral("siteDecisionAsk")).first());
    QCOMPARE(sites->decision(SitePermissions::Camera, site), int(SitePermissions::Ask));
    QCOMPARE(value(SitePermissions::Camera), QStringLiteral("Always ask"));
    QCOMPARE(lastToEngine().value(QStringLiteral("msg")).toString(), QStringLiteral("add"));
    QCOMPARE(lastToEngine().value(QStringLiteral("type")).toString(), QStringLiteral("camera"));
    QCOMPARE(lastToEngine().value(QStringLiteral("permission")).toInt(), 3);
    click(findObjects(row(SitePermissions::Camera), QStringLiteral("siteDecisionDefault")).first());
    QCOMPARE(sites->decision(SitePermissions::Camera, site), int(SitePermissions::Default));
    defaults->setCameraBlocked(false);
    QCOMPARE(textOf(row(SitePermissions::Camera), "siteDecisionDefault"),
             QStringLiteral("Follow default: Ask"));

    click(row(SitePermissions::Notifications));
    QVERIFY(row(SitePermissions::Notifications)->property("menuOpen").toBool());
    click(findObjects(row(SitePermissions::Notifications), QStringLiteral("siteDecisionAllow"))
              .first());
    QCOMPARE(sites->decision(SitePermissions::Notifications, site), int(SitePermissions::Allow));
    QCOMPARE(value(SitePermissions::Notifications), QStringLiteral("Allowed"));
    QVERIFY(!marked(SitePermissions::Notifications));
    QCOMPARE(lastToEngine().value(QStringLiteral("type")).toString(),
             QStringLiteral("desktop-notification"));
    QCOMPARE(lastToEngine().value(QStringLiteral("permission")).toInt(), 1);
    // The notifications' own model has it, as it is what the page asks.
    QVERIFY(m_core->notificationPermissions()->isAllowed(site));

    click(findObjects(row(SitePermissions::Location), QStringLiteral("siteDecisionBlock")).first());
    QCOMPARE(value(SitePermissions::Location), QStringLiteral("Blocked"));
    QCOMPARE(lastToEngine().value(QStringLiteral("permission")).toInt(), 2);
    click(findObjects(row(SitePermissions::Popups), QStringLiteral("siteDecisionAllow")).first());
    QCOMPARE(value(SitePermissions::Popups), QStringLiteral("Allowed"));
    QVERIFY(!marked(SitePermissions::Popups));
    QVERIFY(marked(SitePermissions::Camera));

    // Following the default again takes the exception away.
    click(findObjects(row(SitePermissions::Popups), QStringLiteral("siteDecisionDefault")).first());
    QCOMPARE(sites->decision(SitePermissions::Popups, site), int(SitePermissions::Default));
    QCOMPARE(value(SitePermissions::Popups), QStringLiteral("Follow default: Block"));
    QVERIFY(marked(SitePermissions::Popups));
    QCOMPARE(lastToEngine().value(QStringLiteral("msg")).toString(), QStringLiteral("remove"));
    QCOMPARE(lastToEngine().value(QStringLiteral("type")).toString(), QStringLiteral("popup"));

    // With some, the pulley's item to clear them, which clears this site's and no other's.
    sites->set(SitePermissions::Camera, QStringLiteral("https://other.example"),
               SitePermissions::Block);
    QVERIFY(pulleyShown());
    QVERIFY(find(QStringLiteral("clearSitePermissionsButton")) == nullptr);
    QCOMPARE(textOf(page, "clearSitePermissionsMenuItem"),
             QStringLiteral("Clear site permissions"));
    const int before = reloads();
    click(find(QStringLiteral("clearSitePermissionsMenuItem")));
    QCOMPARE(sites->originCount(site), 0);
    QCOMPARE(sites->originCount(QStringLiteral("https://other.example")), 1);
    QCOMPARE(value(SitePermissions::Notifications), QStringLiteral("Follow default: Ask"));
    QVERIFY(marked(SitePermissions::Notifications));
    QVERIFY(!pulleyShown());
    QVERIFY(!m_core->notificationPermissions()->isAllowed(site));
    // Tracking protection was not among them: nothing to load again.
    QCOMPARE(reloads(), before);

    // The cookies: a site that has an exception to them has the row; so does every site
    // while tracking protection is off.
    sites->set(SitePermissions::Cookies, site, SitePermissions::Block);
    QVERIFY(rowShown(SitePermissions::Cookies));
    QCOMPARE(value(SitePermissions::Cookies), QStringLiteral("Blocked"));
    QCOMPARE(textOf(row(SitePermissions::Cookies), "siteDecisionDefault"),
             QStringLiteral("Follow default: Block cross-site"));
    QVERIFY(!shownIn(row(SitePermissions::Cookies), "siteDecisionAsk"));
    sites->remove(SitePermissions::Cookies, site);
    QVERIFY(!rowShown(SitePermissions::Cookies));
    privacy->setTrackingProtection(PrivacySettings::TrackingProtectionOff);
    QVERIFY(rowShown(SitePermissions::Cookies));
    QCOMPARE(value(SitePermissions::Cookies), QStringLiteral("Follow default: Block cross-site"));
    QVERIFY(marked(SitePermissions::Cookies));
    privacy->setTrackingProtection(PrivacySettings::TrackingProtectionStandard);
    QVERIFY(!rowShown(SitePermissions::Cookies));
    // Off for this site alone is enough, and clearing the site's permissions turns it on
    // again, as the switch would, loading the page again.
    click(find(QStringLiteral("siteTrackingSwitch")));
    QVERIFY(rowShown(SitePermissions::Cookies));
    QVERIFY(pulleyShown());
    const int afterSwitch = reloads();
    click(find(QStringLiteral("clearSitePermissionsMenuItem")));
    QVERIFY(!rowShown(SitePermissions::Cookies));
    QCOMPARE(reloads(), afterSwitch + 1);
    QVERIFY(find(QStringLiteral("siteTrackingSwitch"))->property("checked").toBool());
}

QTEST_MAIN(tst_qmlload)
namespace {

// The tutorial's sketched cells, not the browsing page's own grid's, in the order they are
// laid out: by row, and left to right in a row.
QList<QObject *> tutorialCells(QObject *grid)
{
    QList<QObject *> found = findObjects(grid, QStringLiteral("tabPreview"));
    std::sort(found.begin(), found.end(), [](QObject *one, QObject *other) {
        const qreal oneY = one->property("y").toReal();
        const qreal otherY = other->property("y").toReal();
        return oneY < otherY ||
               (oneY == otherY && one->property("x").toReal() < other->property("x").toReal());
    });
    return found;
}

} // namespace

// The first start shows the tutorial over the browsing page, on its first card: the
// application's mark and name over what it is, and the ways to start the tutorial or skip
// it. From then on it has been shown: the next start is the page alone
// (docs/DECISIONS/0034-tutorial.md).
void tst_qmlload::tutorialOnFirstStart()
{
    QVERIFY(startWithoutTabs(false));
    QTRY_COMPARE(currentPage()->objectName(), QStringLiteral("tutorialPage"));
    QCOMPARE(pageStack()->property("depth").toInt(), 2);
    QVERIFY(m_core->settings()->tutorialShown());

    QObject *page = currentPage();
    QCOMPARE(page->property("step").toString(), QStringLiteral("welcome"));
    QObject *card = find(QStringLiteral("tutorialWelcome"));
    QVERIFY(card->property("visible").toBool());
    QObject *logo = findObjects(card, QStringLiteral("tutorialLogo")).first();
    QVERIFY(logo->property("visible").toBool());
    QVERIFY(logo->property("source").toUrl().toString().endsWith(QLatin1String("art/logo.png")));
    QVERIFY(QFile::exists(QLatin1String(SALAMA_SOURCE_DIR "/art/logo.png")));
    QCOMPARE(findObjects(card, QStringLiteral("tutorialCardHeading")).first()->property("text"),
             QVariant(QStringLiteral("Salama")));
    QVERIFY(!findObjects(card, QStringLiteral("tutorialCardSubheading"))
                 .first()
                 ->property("text")
                 .toString()
                 .isEmpty());
    // What the lessons cover, as three icons with their names rather than a sentence.
    QVERIFY(!findObjects(card, QStringLiteral("tutorialCardText"))
                 .first()
                 ->property("visible")
                 .toBool());
    QStringList topics;
    for (QObject *name : findObjects(card, QStringLiteral("tutorialTopicName"))) {
        topics.append(name->property("text").toString());
    }
    QCOMPARE(topics, (QStringList{QStringLiteral("Address bar"), QStringLiteral("Menu"),
                                  QStringLiteral("Tabs")}));
    QVERIFY(
        !findObjects(card, QStringLiteral("tutorialCheck")).first()->property("visible").toBool());
    QVERIFY(find(QStringLiteral("tutorialStartButton")) != nullptr);
    QVERIFY(!find(QStringLiteral("tutorialTouchHint"))->property("running").toBool());
    QVERIFY(!find(QStringLiteral("tutorialTapHint"))->property("running").toBool());

    // Skipped, it is the browsing page under it, on the start page, as a first start is.
    click(find(QStringLiteral("tutorialSkipButton")));
    QCOMPARE(currentPage()->objectName(), QStringLiteral("browserPage"));
    QVERIFY(find(QStringLiteral("startPageLayer"))->property("active").toBool());

    m_window.reset();
    m_engine.reset();
    QVERIFY(loadWindow());
    QTest::qWait(100);
    QCOMPARE(currentPage()->objectName(), QStringLiteral("browserPage"));
    QCOMPARE(pageStack()->property("depth").toInt(), 1);

    // Started instead, the first card gives way to the first lesson.
    evaluate(m_window.data(), QStringLiteral("showTutorial()"));
    page = currentPage();
    QCOMPARE(page->property("step").toString(), QStringLiteral("welcome"));
    click(find(QStringLiteral("tutorialStartButton")));
    QCOMPARE(page->property("step").toString(), QStringLiteral("address"));
    QTRY_VERIFY(!find(QStringLiteral("tutorialWelcome"))->property("visible").toBool());
}

// The tutorial, from Settings, straight into its lessons as the platform's own Tutorial
// runs them: each step shown by a hint where the finger is to go and said by a label at
// the other end of the screen, and waiting for its own gesture -- a gesture out of turn
// does nothing. The address bar, which goes to an address and searches alike, and the
// menu; every step says something of its own (docs/DECISIONS/0034-tutorial.md).
void tst_qmlload::tutorial()
{
    openMenuItem(QStringLiteral("settingsMenuButton"));
    click(find(QStringLiteral("tutorialSettingsEntry")));
    QObject *page = currentPage();
    QCOMPARE(page->objectName(), QStringLiteral("tutorialPage"));
    QCOMPARE(pageStack()->property("depth").toInt(), 3);

    QObject *deck = find(QStringLiteral("tutorialDeck"));
    QObject *bar = find(QStringLiteral("tutorialBar"));
    QObject *tapHint = find(QStringLiteral("tutorialTapHint"));
    QObject *touchHint = find(QStringLiteral("tutorialTouchHint"));
    QObject *label = find(QStringLiteral("tutorialHintLabel"));
    const qreal threshold = deck->property("pullThreshold").toReal();
    const auto step = [page]() { return page->property("step").toString(); };
    const auto tapping = [tapHint]() { return tapHint->property("running").toBool(); };
    const auto moving = [touchHint]() { return touchHint->property("running").toBool(); };
    const auto centre = [](QObject *item) {
        auto *quick = qobject_cast<QQuickItem *>(item);
        return quick->mapToScene(QPointF(quick->width() / 2, quick->height() / 2));
    };
    const auto point = [&](QObject *item, const char *name) {
        auto *quick = qobject_cast<QQuickItem *>(item);
        return quick->mapToScene(quick->property(name).toPointF());
    };
    const auto labelAtTop = [&]() {
        return label->property("invert").toBool() && label->property("y").toReal() == 0;
    };

    // Every step says something, and no two say the same.
    QStringList said;
    for (const QVariant &lesson : page->property("lessons").toList()) {
        said.append(
            evaluate(page, QStringLiteral("stepText('%1')").arg(lesson.toString())).toString());
        QVERIFY2(!said.last().isEmpty(), qPrintable(lesson.toString()));
    }
    QCOMPARE(said.count(), 9);
    QCOMPARE(QSet<QString>(said.begin(), said.end()).count(), 9);

    // No first card from Settings. The address bar: a tap on it, the words at the head of
    // the screen, the band inverted to lie along it.
    QCOMPARE(step(), QStringLiteral("address"));
    QVERIFY(!find(QStringLiteral("tutorialWelcome"))->property("visible").toBool());
    QVERIFY(page->property("hinting").toBool());
    QVERIFY(tapping());
    QVERIFY(!moving());
    QCOMPARE(centre(tapHint), point(bar, "addressCentre"));
    QVERIFY(labelAtTop());
    QCOMPARE(label->property("text").toString(), said.at(0));
    QCOMPARE(said.at(0), QStringLiteral("Tap the address bar to open a website or search."));
    // Under the words, which of the five lessons this is.
    QObject *progress = find(QStringLiteral("tutorialProgress"));
    QCOMPARE(progress->property("count").toInt(), 5);
    QCOMPARE(findObjects(progress, QStringLiteral("tutorialProgressDot")).count(), 5);
    QCOMPARE(progress->property("current").toInt(), 0);
    QVERIFY(progress->property("visible").toBool());
    QVERIFY(progress->property("y").toReal() >= label->property("height").toReal());
    const QStringList lessonOrder{
        QStringLiteral("address"),  QStringLiteral("omnibar"),  QStringLiteral("menu"),
        QStringLiteral("menuOpen"), QStringLiteral("open"),     QStringLiteral("closeTab"),
        QStringLiteral("moveTab"),  QStringLiteral("groupTab"), QStringLiteral("close")};
    const QList<int> lessonIndex{0, 0, 1, 1, 2, 3, 3, 3, 4};
    for (int i = 0; i < lessonOrder.count(); ++i) {
        QCOMPARE(evaluate(page, QStringLiteral("lessonOf('%1')").arg(lessonOrder.at(i))).toInt(),
                 lessonIndex.at(i));
    }
    QCOMPARE(evaluate(page, QStringLiteral("lessonOf('done')")).toInt(), -1);

    // Out of turn: the menu, and a drag up, do nothing yet.
    evaluate(bar, QStringLiteral("activate('menu')"));
    evaluate(bar, QStringLiteral("dragStarted()"));
    evaluate(bar, QStringLiteral("dragMoved(%1)").arg(threshold + 1));
    evaluate(bar, QStringLiteral("dragFinished(%1)").arg(threshold + 1));
    QCOMPARE(step(), QStringLiteral("address"));
    QVERIFY(!deck->property("tabsOpen").toBool());
    QCOMPARE(deck->property("tabsOffset").toReal(), qreal(0));

    // Tapped, the address is a field with an address in it, and above it the rows to go
    // there and to search for it, which names the engine Settings chose.
    evaluate(bar, QStringLiteral("activate('address')"));
    QCOMPARE(step(), QStringLiteral("omnibar"));
    QVERIFY(bar->property("editing").toBool());
    QVERIFY(find(QStringLiteral("tutorialOmnibar"))->property("visible").toBool());
    const QString typed = bar->property("typed").toString();
    QVERIFY(!typed.isEmpty());
    const QString go = find(QStringLiteral("tutorialGoAction"))->property("title").toString();
    const QString search =
        find(QStringLiteral("tutorialSearchAction"))->property("title").toString();
    QVERIFY(go.contains(typed));
    QVERIFY(search.contains(typed));
    QVERIFY(search.contains(m_core->searchEngines()->engineNames().first()));
    QVERIFY(!tapping());
    QVERIFY(!moving());
    QVERIFY(page->property("saying").toBool());
    QCOMPARE(label->property("text").toString(), said.at(1));
    QObject *continueButton = find(QStringLiteral("tutorialContinueButton"));
    QVERIFY(continueButton->property("visible").toBool());
    click(continueButton);

    // The menu: a tap on its button, and then outside the sheet, which puts it away.
    QCOMPARE(step(), QStringLiteral("menu"));
    QVERIFY(!bar->property("editing").toBool());
    QVERIFY(!continueButton->property("visible").toBool());
    QVERIFY(tapping());
    QCOMPARE(centre(tapHint), point(bar, "menuCentre"));
    evaluate(bar, QStringLiteral("activate('address')"));
    QCOMPARE(step(), QStringLiteral("menu"));
    evaluate(bar, QStringLiteral("activate('menu')"));
    QCOMPARE(step(), QStringLiteral("menuOpen"));
    QCOMPARE(progress->property("current").toInt(), 1);
    QObject *menu = find(QStringLiteral("tutorialMenu"));
    QVERIFY(menu->property("visible").toBool());
    const qreal sheetTop = qobject_cast<QQuickItem *>(find(QStringLiteral("tutorialMenuSheet")))
                               ->mapToScene(QPointF(0, 0))
                               .y();
    QVERIFY(sheetTop > 0);
    QCOMPARE(findObjects(menu, QStringLiteral("menuButtonIcon")).count(), 9);
    QVERIFY(tapping());
    QVERIFY(centre(tapHint).y() < sheetTop);
    QVERIFY(labelAtTop());
    evaluate(menu, QStringLiteral("dismissed()"));
    QCOMPARE(step(), QStringLiteral("open"));
    QVERIFY(!menu->property("visible").toBool());
    QVERIFY(!tapping());
    QVERIFY(moving());
}

// The tutorial's grid, from its step on: the bar dragged up, which a finger lifted short of
// the threshold leaves as it was; in the grid, four made-up pages, a tab closed, moved and
// moved to another group; and the grid pulled down again. What is done is done to a
// sketch: no tab is touched. At its end a card, and closed, it goes back to where it was
// opened from.
void tst_qmlload::tutorialGrid()
{
    QObject *settings = openMenuItem(QStringLiteral("settingsMenuButton"));
    click(find(QStringLiteral("tutorialSettingsEntry")));
    QObject *page = currentPage();
    evaluate(page, QStringLiteral("step = 'open'"));
    const int tabCount = m_core->tabs()->count();
    const int activeTab = m_core->tabs()->activeTabId();

    QObject *deck = find(QStringLiteral("tutorialDeck"));
    QObject *bar = find(QStringLiteral("tutorialBar"));
    QObject *grid = find(QStringLiteral("tutorialGrid"));
    QObject *strip = find(QStringLiteral("tutorialStrip"));
    QObject *touchHint = find(QStringLiteral("tutorialTouchHint"));
    QObject *label = find(QStringLiteral("tutorialHintLabel"));
    QObject *recap = find(QStringLiteral("tutorialRecap"));
    const qreal threshold = deck->property("pullThreshold").toReal();
    const qreal height = page->property("height").toReal();
    const qreal width = page->property("width").toReal();
    const auto step = [page]() { return page->property("step").toString(); };
    const auto moving = [touchHint]() { return touchHint->property("running").toBool(); };
    const auto enumValue = [&](const char *name) {
        return evaluate(page, QStringLiteral("TouchInteraction.") + QLatin1String(name));
    };
    const auto dragBar = [&](qreal distance) {
        evaluate(bar, QStringLiteral("dragStarted()"));
        evaluate(bar, QStringLiteral("dragMoved(%1)").arg(distance));
        evaluate(bar, QStringLiteral("dragFinished(%1)").arg(distance));
    };
    const auto pullGrid = [&](qreal distance) {
        evaluate(grid, QStringLiteral("pullStarted()"));
        evaluate(grid, QStringLiteral("pulled(%1)").arg(distance));
        evaluate(grid, QStringLiteral("pullFinished(%1)").arg(distance));
    };
    const auto cells = [&]() { return tutorialCells(grid); };
    const auto labelAtTop = [&]() {
        return label->property("invert").toBool() && label->property("y").toReal() == 0;
    };
    const auto labelAtFoot = [&]() {
        return !label->property("invert").toBool() &&
               label->property("y").toReal() + label->property("height").toReal() == height;
    };

    // The bar dragged up, shown going up from its handle as a pull; no hint and no words
    // under a finger already dragging; short of the threshold it springs back and the
    // step is still the bar's.
    QVERIFY(moving());
    QCOMPARE(touchHint->property("direction"), enumValue("Up"));
    QCOMPARE(touchHint->property("interactionMode"), enumValue("Pull"));
    QVERIFY(touchHint->property("loops").toInt() < 0);
    QCOMPARE(touchHint->property("startY").toReal() + touchHint->property("height").toReal() / 2,
             height - bar->property("height").toReal());
    QVERIFY(labelAtTop());
    evaluate(bar, QStringLiteral("dragStarted()"));
    QVERIFY(!moving());
    QVERIFY(!page->property("hinting").toBool());
    evaluate(bar, QStringLiteral("dragMoved(%1)").arg(threshold - 1));
    QVERIFY(deck->property("tabsOffset").toReal() > 0);
    QVERIFY(grid->property("visible").toBool());
    evaluate(bar, QStringLiteral("dragFinished(%1)").arg(threshold - 1));
    QVERIFY(!deck->property("tabsOpen").toBool());
    QCOMPARE(step(), QStringLiteral("open"));
    QVERIFY(moving());
    dragBar(threshold + 1);
    QVERIFY(deck->property("tabsOpen").toBool());

    // In the grid, four made-up pages, the first in front, and a strip with the group
    // they are in and one other. A tab slid to the left closes, shown going left across
    // the first row, the words at the foot. The grid cannot be pulled down yet.
    QCOMPARE(step(), QStringLiteral("closeTab"));
    QCOMPARE(cells().count(), 4);
    QVERIFY(cells().first()->property("highlighted").toBool());
    QVERIFY(QFile::exists(evaluate(cells().first(), QStringLiteral("model.thumbnail")).toString()));
    QCOMPARE(touchHint->property("direction"), enumValue("Left"));
    QCOMPARE(touchHint->property("interactionMode"), enumValue("Swipe"));
    // Placed where the first row will be once the grid is up, the deck still springing.
    auto *gridItem = qobject_cast<QQuickItem *>(grid);
    const auto inGrid = [&](QObject *item) {
        auto *quick = qobject_cast<QQuickItem *>(item);
        return quick->mapToItem(gridItem, QPointF(quick->width() / 2, quick->height() / 2));
    };
    QCOMPARE(evaluate(touchHint, QStringLiteral("anchors.verticalCenterOffset")).toReal(),
             inGrid(cells().first()).y() - height / 2);
    QVERIFY(labelAtFoot());
    pullGrid(threshold + 1);
    QVERIFY(deck->property("tabsOpen").toBool());
    QObject *second = cells().at(1);
    evaluate(second, QStringLiteral("swipeTo(-width / 4)"));
    QVERIFY(!moving());
    evaluate(second, QStringLiteral("releaseSwipe()"));
    QCOMPARE(cells().count(), 4);
    QCOMPARE(step(), QStringLiteral("closeTab"));
    QVERIFY(moving());
    evaluate(second, QStringLiteral("swipeTo(-width / 2)"));
    evaluate(second, QStringLiteral("releaseSwipe()"));
    QCOMPARE(cells().count(), 3);

    // A tab held until it lifts and carried over another trades places with it, shown
    // going right from the first cell.
    QCOMPARE(step(), QStringLiteral("moveTab"));
    QCOMPARE(touchHint->property("direction"), enumValue("Right"));
    QCOMPARE(touchHint->property("startX").toReal() + touchHint->property("width").toReal() / 2,
             inGrid(cells().first()).x());
    QVERIFY(labelAtFoot());
    QObject *first = cells().first();
    const QString firstPicture = evaluate(first, QStringLiteral("model.thumbnail")).toString();
    evaluate(first, QStringLiteral("pickUp()"));
    QVERIFY(!moving());
    evaluate(first, QStringLiteral("moveRequested(0, 1)"));
    evaluate(first, QStringLiteral("drop()"));
    QCOMPARE(evaluate(cells().at(1), QStringLiteral("model.thumbnail")).toString(), firstPicture);

    // A tab carried onto the other group's name leaves the grid for that group, shown
    // going down onto the name; the strip names the group in front by what it holds.
    QCOMPARE(step(), QStringLiteral("groupTab"));
    QCOMPARE(touchHint->property("direction"), enumValue("Down"));
    QCOMPARE(evaluate(touchHint, QStringLiteral("anchors.horizontalCenterOffset")).toReal(),
             qobject_cast<QQuickItem *>(strip)
                     ->mapToItem(gridItem, strip->property("otherCentre").toPointF())
                     .x() -
                 width / 2);
    QVERIFY(labelAtTop());
    QObject *current = find(QStringLiteral("tutorialCurrentGroup"));
    const QString threeTabs = current->property("text").toString();
    QVERIFY(threeTabs.contains(QLatin1Char('3')));
    QObject *carried = cells().last();
    evaluate(carried, QStringLiteral("pickUp()"));
    evaluate(strip, QStringLiteral("carryOver(groupStrip, 0, 0)"));
    QCOMPARE(strip->property("dropIndex").toInt(), -1);
    evaluate(strip, QStringLiteral("carryOver(groupStrip, otherCentre.x, otherCentre.y)"));
    QCOMPARE(strip->property("dropIndex").toInt(), 1);
    const int carriedId = evaluate(carried, QStringLiteral("model.tabId")).toInt();
    evaluate(strip, QStringLiteral("dropTab(%1)").arg(carriedId));
    evaluate(carried, QStringLiteral("drop()"));
    QTRY_COMPARE(step(), QStringLiteral("close"));
    QCOMPARE(cells().count(), 2);
    QVERIFY(current->property("text").toString() != threeTabs);

    // Pulled short, the grid stays; all the way, the page is back, and the card says it
    // is complete a moment after.
    QCOMPARE(touchHint->property("direction"), enumValue("Down"));
    QCOMPARE(touchHint->property("interactionMode"), enumValue("Pull"));
    QCOMPARE(touchHint->property("startY").toReal() + touchHint->property("height").toReal() / 2,
             height / 3);
    QVERIFY(labelAtFoot());
    pullGrid(threshold - 1);
    QVERIFY(deck->property("tabsOpen").toBool());
    QCOMPARE(step(), QStringLiteral("close"));
    pullGrid(threshold + 1);
    QVERIFY(!deck->property("tabsOpen").toBool());
    QCOMPARE(step(), QStringLiteral("done"));
    QVERIFY(!moving());
    QVERIFY(!page->property("hinting").toBool());
    QVERIFY(!recap->property("visible").toBool());
    QTRY_VERIFY(recap->property("visible").toBool());
    // The card says it is done under a check mark, and where to find it again.
    QVERIFY(
        findObjects(recap, QStringLiteral("tutorialCheck")).first()->property("visible").toBool());
    QCOMPARE(
        findObjects(recap, QStringLiteral("tutorialCardText")).first()->property("text").toString(),
        QStringLiteral("You can open it again from Settings."));
    QVERIFY(!find(QStringLiteral("tutorialProgress"))->property("visible").toBool());

    // Nothing of it was done to a tab, and the browsing page's own grid stayed down.
    QCOMPARE(m_core->tabs()->count(), tabCount);
    QCOMPARE(m_core->tabs()->activeTabId(), activeTab);
    QVERIFY(!find(QStringLiteral("browserPage"))->property("tabsOpen").toBool());

    // Closed, it is where it was opened from.
    click(find(QStringLiteral("tutorialCloseButton")));
    QCOMPARE(currentPage(), settings);
}

// The tutorial's gestures under a real finger, from the first card on: taps on the bar
// and beside the menu, the drag up that goes through the bar's own gesture, a cell slid
// away, held and carried, and carried onto the other group through the real grid's own
// cells, and the pull down that is the sketched grid's overscroll.
void tst_qmlload::tutorialUnderAFinger()
{
    evaluate(m_window.data(), QStringLiteral("showTutorial()"));
    QObject *page = currentPage();
    QCOMPARE(page->objectName(), QStringLiteral("tutorialPage"));
    FingerWindow host(m_window.data());
    QQuickWindow &window = *host.window();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    const auto step = [page]() { return page->property("step").toString(); };
    const auto tap = [&](const QPointF &at) {
        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, at.toPoint());
    };

    QObject *deck = find(QStringLiteral("tutorialDeck"));
    QObject *bar = find(QStringLiteral("tutorialBar"));
    auto *gesture = qobject_cast<QQuickItem *>(find(QStringLiteral("tutorialBarGesture")));
    const int threshold = deck->property("pullThreshold").toInt();
    const QPointF gestureTop = gesture->mapToScene(QPointF(0, 0));
    const int onBar = int(gestureTop.y() + gesture->property("reach").toReal() +
                          gesture->property("strip").toReal() / 2);
    const int across = int(gesture->width()) / 2;
    const QPoint up(0, 3 * threshold);
    const auto cells = [&]() { return tutorialCells(find(QStringLiteral("tutorialGrid"))); };
    // What the sketch holds: a cell closed or carried away lingers a moment after its tab.
    const auto count = [&]() {
        return find(QStringLiteral("tutorialGrid"))->property("count").toInt();
    };

    // The first card keeps the sketch from being dragged, and Start gives way to it.
    drag(&window, QPoint(across, onBar), QPoint(across, onBar) - up);
    QVERIFY(!deck->property("tabsOpen").toBool());
    click(find(QStringLiteral("tutorialStartButton")));
    QTRY_VERIFY(!find(QStringLiteral("tutorialWelcome"))->property("visible").toBool());

    // Taps: the address, then the menu button, then beside the sheet.
    auto *barItem = qobject_cast<QQuickItem *>(bar);
    tap(barItem->mapToScene(bar->property("addressCentre").toPointF()));
    QCOMPARE(step(), QStringLiteral("omnibar"));
    click(find(QStringLiteral("tutorialContinueButton")));
    tap(barItem->mapToScene(bar->property("menuCentre").toPointF()));
    QCOMPARE(step(), QStringLiteral("menuOpen"));
    tap(QPointF(across, window.height() / 4.0));
    QCOMPARE(step(), QStringLiteral("open"));

    // A drag up from the bar that stops short springs back; one past the threshold
    // brings the grid up, the handle lit while the finger was on it.
    drag(&window, QPoint(across, onBar), QPoint(across, onBar - threshold / 2));
    QVERIFY(!deck->property("tabsOpen").toBool());
    QTRY_COMPARE(deck->property("tabsOffset").toReal(), qreal(0));
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(across, onBar));
    QTest::mouseMove(&window, QPoint(across, onBar - threshold));
    QVERIFY(find(QStringLiteral("tutorialDragHandle"))->property("active").toBool());
    QTest::mouseMove(&window, QPoint(across, onBar) - up);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(across, onBar) - up);
    QVERIFY(deck->property("tabsOpen").toBool());
    QCOMPARE(step(), QStringLiteral("closeTab"));
    QTRY_COMPARE(deck->property("tabsOffset").toReal(), deck->property("fullHeight").toReal());

    // Slid to the left, a cell closes its tab.
    const QPoint slide = centreOf(cells().at(1));
    drag(&window, slide, slide - QPoint(cells().at(1)->property("width").toInt() / 2, 0));
    QCOMPARE(count(), 3);
    QCOMPARE(step(), QStringLiteral("moveTab"));
    QTRY_COMPARE(cells().count(), 3);

    // Held until it lifts and carried over its neighbour, a cell trades places with it.
    QObject *first = cells().first();
    const QPoint grab = centreOf(first);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, grab);
    QTRY_VERIFY(first->property("held").toBool());
    const QPoint neighbour = centreOf(cells().at(1));
    QTest::mouseMove(&window, (grab + neighbour) / 2);
    QTest::mouseMove(&window, neighbour);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, neighbour);
    QCOMPARE(step(), QStringLiteral("groupTab"));

    // Held and carried down onto the other group's name, it leaves for that group.
    QObject *last = cells().last();
    const QPoint hold = centreOf(last);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, hold);
    QTRY_VERIFY(last->property("held").toBool());
    const QPoint onOther = centreOf(find(QStringLiteral("tutorialOtherGroup")));
    QTest::mouseMove(&window, (hold + onOther) / 2);
    QTest::mouseMove(&window, onOther);
    QCOMPARE(find(QStringLiteral("tutorialStrip"))->property("dropIndex").toInt(), 1);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, onOther);
    QTRY_COMPARE(step(), QStringLiteral("close"));
    QCOMPARE(count(), 2);

    // Pulled down from the middle of the grid, the page comes back and it is done.
    QTRY_VERIFY(!find(QStringLiteral("tutorialGridView"))->property("moving").toBool());
    const QPoint middle(across, window.height() / 2);
    drag(&window, middle, middle + up);
    QVERIFY(!deck->property("tabsOpen").toBool());
    QCOMPARE(step(), QStringLiteral("done"));
}

#include "tst_qmlload.moc"
