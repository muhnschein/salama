// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Real QML on tests/silica-stubs, driven by objectNames.
// Stubs have no layout: tests prove structure and wiring, not appearance.
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
using Salama::DohSettings;
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
// Site showing notifications, named as frame script names page.
const char *const ChatSite = "https://chat.example";
// Start page of tests, in only tab.
const char *const FirstPage = "https://www.qwant.com/";

} // namespace

// Serves OpenSearch descriptions on loopback for engine-found-while-browsing taps. Unknown path
// = 404 HTML page, as stale description address often is.
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

// Catches Qt.openUrlExternally addresses for one scheme instead of launching platform handler.
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
    void menuSheetDoesNotScroll();
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
    void httpsOnlySettingsPage();
    void dohSettingsPage();
    void dohProviderDialog();
    void dohExceptionsPage();
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

// Software rendering before QtQuick loads: offscreen has no OpenGL, gridGesturesUnderAFinger()
// needs real window.
void tst_qmlload::initTestCase()
{
    QQuickWindow::setSceneGraphBackend(QSGRendererInterface::Software);
}

void tst_qmlload::init()
{
    m_dir.reset(new QTemporaryDir);
    m_core.reset(new Core(m_dir->path(), m_dir->path() + QStringLiteral("/salama.conf"),
                          m_dir->path() + QStringLiteral("/Downloads/Salama")));
    // PageMedia queries pages shortly after first tab opens. Answer here: left pending, it fired in
    // a later slow test and stub's "nothing plays" undid that test's playing state.
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

// Startup ask for notification-allowed sites is notificationsStart()'s to check; others count
// from zero.
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

// First start: new data dir, no tab to restore. Tutorial taken as seen unless asked.
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

// Stub page stack destroys popped pages with deferred destroy(); settle so stale pages not found.
void settle()
{
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
}

// Repeater/ListView delegates have no QObject parent: walk visual tree too.
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
        // Item views batch model changes until next frame; no frame here.
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
        // Reverse to keep document order.
        for (int i = children.count() - 1; i >= 0; --i) {
            pending.append(children.at(i));
        }
    }
    return found;
}

// Delegates in layout order: list view parents in creation order, inserted row made later.
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

// Address is label until tapped, edited in place. Bar's gesture handler owns presses, so tap
// raised its way.
void tst_qmlload::typeAddress(const QString &text)
{
    tapBar(QStringLiteral("address"));
    QObject *field = find(QStringLiteral("addressField"));
    QVERIFY(field->property("visible").toBool());
    field->setProperty("text", text);
    enterKey(field);
}

void tst_qmlload::tapBar(const QString &region)
{
    evaluate(find(QStringLiteral("navigationBar")), QStringLiteral("activate('%1')").arg(region));
}

// Drags need window; these raise distances bar/grid report for full gesture (bar up opens
// grid, grid past top closes it).
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

// Tap one icon of bar menu sheet.
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
    // Engine zoom larger than platform default 1.5 * Theme.pixelRatio.
    QObject *page = find(QStringLiteral("browserPage"));
    const qreal zoom = evaluate(page, QStringLiteral("pageZoom()")).toReal();
    QVERIFY(zoom > 1.5 * evaluate(page, QStringLiteral("Theme.pixelRatio")).toReal() - 0.5);
    // Read via view: it carries BrowserPage.qml's Sailfish.WebEngine import, page context doesn't.
    QCOMPARE(evaluate(webView, QStringLiteral("WebEngineSettings.pixelRatio")).toReal(), zoom);
    QVERIFY(webView->property("downloadsEnabled").toBool());
    QCOMPARE(evaluate(webView, QStringLiteral("WebEngineSettings.downloadDir")).toString(),
             m_core->downloads()->directory());
    QVERIFY(evaluate(webView, QStringLiteral("WebEngineSettings.useDownloadDir")).toBool());
    QVERIFY(!webView->property("desktopMode").toBool());

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
    for (const QVariant &content : EngineMessages::contentPreferences(false, true)) {
        QVERIFY(takeGiven(content.toMap()));
    }
    for (const QVariant &https : EngineMessages::httpsOnlyPreferences(false)) {
        QVERIFY(takeGiven(https.toMap()));
    }
    for (const QVariant &doh : EngineMessages::dohPreferences(DohSettings::ProtectionOff,
                                                              DohSettings::defaultProvider(), {})) {
        QVERIFY(takeGiven(doh.toMap()));
    }
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

    QCOMPARE(m_core->history()->count(), 1);
    QCOMPARE(find(QStringLiteral("addressLabel"))->property("text").toString(),
             QStringLiteral("qwant.com"));
    QVERIFY(!find(QStringLiteral("addressField"))->property("visible").toBool());
}

// Nothing of startup pending, however slow runner: slow runner once saw pages asked and stub
// answer reset media state (coverShowsWhatPlays(), mediaControls() failed on CI).
void tst_qmlload::startsQuiet()
{
    Salama::PageMedia *media = m_core->pageMedia();
    TabModel *tabs = m_core->tabs();
    const int front = tabs->activeTabId();
    // Simulate loaded runner: stall past delay.
    QTest::qSleep(media->queryDelay() * 2);
    tabs->setMediaState(front, TabModel::MediaPlaying);
    QSignalSpy asked(media, &Salama::PageMedia::requested);
    QTest::qWait(media->queryDelay() * 2);
    QVERIFY(asked.isEmpty());
    QCOMPARE(tabs->mediaState(front), TabModel::MediaPlaying);
    QVERIFY(currentWebView()->property("scripts").toStringList().isEmpty());
}

// First start: one tab on start page, no view, no visit, bar asks for address.
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

    QVERIFY(find(QStringLiteral("startPagePlaceholder"))->property("enabled").toBool());
    QVERIFY(!find(QStringLiteral("topSitesSection"))->property("visible").toBool());
    QVERIFY(!find(QStringLiteral("bookmarksSection"))->property("visible").toBool());
    QVERIFY(!find(QStringLiteral("recentPagesSection"))->property("visible").toBool());

    QCOMPARE(find(QStringLiteral("addressLabel"))->property("text").toString(),
             QStringLiteral("Search or enter address"));
    QVERIFY(!find(QStringLiteral("navigationBar"))->property("canGoBack").toBool());
    QVERIFY(find(QStringLiteral("backButton"))->property("opacity").toReal() < 1);
    QVERIFY(find(QStringLiteral("reloadButton"))->property("opacity").toReal() < 1);
    tapBar(QStringLiteral("back"));
    tapBar(QStringLiteral("reload"));
    QVERIFY(tabs->activeUrl().isEmpty());

    tapBar(QStringLiteral("address"));
    QVERIFY(find(QStringLiteral("addressField"))->property("text").toString().isEmpty());
    evaluate(find(QStringLiteral("navigationBar")), QStringLiteral("endEditing()"));

    tabs->closeActiveTab();
    QCOMPARE(tabs->count(), 1);
    QVERIFY(tabs->activeUrl().isEmpty());
    QCOMPARE(tabs->closedTabs()->count(), 0);
}

// Start page opens in its tab; back from first page returns to it. Content = visited +
// bookmarked, per Settings > Start page.
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
    view->setProperty("loading", true);
    QCOMPARE(find(QStringLiteral("reloadButton"))->property("source").toUrl(),
             QUrl(QStringLiteral("image://theme/icon-m-reset")));
    view->setProperty("loading", false);
    QCOMPARE(find(QStringLiteral("reloadButton"))->property("source").toUrl(),
             QUrl(QStringLiteral("image://theme/icon-m-refresh")));

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

    click(tiles.first());
    QCOMPARE(tabs->activeUrl(), QStringLiteral("https://example.org"));
    QVERIFY(currentWebView() != nullptr);
    QVERIFY(bar->property("canGoBack").toBool());
    tapBar(QStringLiteral("back"));
    QVERIFY(tabs->activeUrl().isEmpty());

    pullUpToTabs();
    QList<QObject *> previews = findAll(QStringLiteral("tabPreview"));
    QCOMPARE(previews.count(), 1);
    QCOMPARE(findObjects(previews.first(), QStringLiteral("tabPreviewPlaceholder"))
                 .first()
                 ->property("text")
                 .toString(),
             QStringLiteral("Start page"));
    pullDownToBrowser();

    m_core->bookmarks()->add(QStringLiteral("https://sailfishos.org/"),
                             QStringLiteral("Sailfish OS"));
    QVERIFY(find(QStringLiteral("bookmarksSection"))->property("visible").toBool());
    QList<QObject *> marks = findAll(QStringLiteral("bookmarkTile"));
    QCOMPARE(marks.count(), 1);
    QCOMPARE(findObjects(marks.first(), QStringLiteral("siteTileName")).first()->property("text"),
             QVariant(QStringLiteral("Sailfish OS")));

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
    QVERIFY(!find(QStringLiteral("addressField"))->property("visible").toBool());
    QCOMPARE(find(QStringLiteral("addressLabel"))->property("text").toString(),
             QStringLiteral("example.org"));
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

// Rows in model order: list laid out bottom-up, first is lowest.
QList<QObject *> omnibarRows(QObject *root)
{
    QList<QObject *> rows = byRow(findObjects(root, QStringLiteral("omnibarResult")));
    std::reverse(rows.begin(), rows.end());
    return rows;
}

// Row title without bold of typed words.
QString titleOf(QObject *row)
{
    return textIn(row, QStringLiteral("omnibarResultTitle"))
        .remove(QStringLiteral("<b>"))
        .remove(QStringLiteral("</b>"));
}

QObject *omnibarRowTitled(QObject *root, const QString &title)
{
    for (QObject *row : omnibarRows(root)) {
        if (titleOf(row) == title) {
            return row;
        }
    }
    return nullptr;
}

bool learnt(Core *core, const QString &typed, const QString &url)
{
    return core->history()->inputRanks(typed, QDateTime::currentMSecsSinceEpoch()).contains(url);
}

// Opens bar if needed, types, skips debounce.
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

// Omnibar fixture: one "forest" item of each kind. Front tab has it (never listed); one tab in
// same group, one in another; bookmark; one history page (tab visits cleared); running download.
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

// Omnibar pane: typed words find tabs (all groups), bookmarks, history, downloads, plus go/search
// rows below. Model ranking is tst_omnibarmodel's; this tests pane + bar.
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

    // Declared after both bars so nothing drawn over it.
    auto *paneItem = qobject_cast<QQuickItem *>(pane);
    const QList<QQuickItem *> layer = paneItem->parentItem()->childItems();
    QVERIFY(layer.indexOf(paneItem) > layer.indexOf(qobject_cast<QQuickItem *>(bar)));
    QVERIFY(layer.indexOf(paneItem) >
            layer.indexOf(qobject_cast<QQuickItem *>(find(QStringLiteral("findBar")))));
    const QColor tint = find(QStringLiteral("omnibarTint"))->property("color").value<QColor>();
    QCOMPARE(tint.alphaF(), 1.0);
    QCOMPARE(tint, find(QStringLiteral("gridHeadRow"))->property("color").value<QColor>());

    tapBar(QStringLiteral("address"));
    QCOMPARE(bar->property("typedText").toString(), home);
    QVERIFY(!bar->property("edited").toBool());
    QVERIFY(!bar->property("paneUp").toBool());
    QVERIFY(!pane->property("visible").toBool());
    QCOMPARE(field->property("focusOutBehavior").toInt(), clearFocus);
    QVERIFY(gesture->property("reach").toReal() > 0);

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

    // List sized to content, hangs from go/search rows; never sets currentItem (would steal focus).
    auto *results = qobject_cast<QQuickItem *>(find(QStringLiteral("omnibarResults")));
    auto *actions = qobject_cast<QQuickItem *>(find(QStringLiteral("omnibarActions")));
    QCOMPARE(results->property("currentIndex").toInt(), -1);
    QCOMPARE(results->y() + results->height(), actions->y());
    QCOMPARE(actions->y() + actions->height(), paneItem->height());
    QVERIFY(results->height() < actions->y());

    // Keyboard close / focus loss don't end edit while pane up. Field drops focus with keyboard so
    // tap brings keyboard back.
    QVERIFY(field->property("focus").toBool());
    evaluate(bar, QStringLiteral("keyboardVisibilityChanged(false)"));
    QVERIFY(!field->property("focus").toBool());
    evaluate(bar, QStringLiteral("focusChanged(false)"));
    QMetaObject::invokeMethod(results, "dragStarted");
    QVERIFY(bar->property("editing").toBool());
    QVERIFY(pane->property("visible").toBool());

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

// Rows follow sources live: come/go; download progress and tab icon update in place (not
// recreated under finger); max 8 rows.
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

// Each listed item and action row does its thing; all end edit.
void tst_qmlload::omnibarChoices()
{
    const Forest forest = plantForest(m_core.data());
    QObject *root = m_window.data();
    TabModel *tabs = m_core->tabs();
    QObject *bar = find(QStringLiteral("navigationBar"));
    QObject *webView = currentWebView();
    const int open = tabs->count();

    typeIntoBar(root, QStringLiteral("forest work"));
    QCOMPARE(omnibarRows(root).count(), 1);
    click(omnibarRows(root).first());
    QVERIFY(!bar->property("editing").toBool());
    QVERIFY(!find(QStringLiteral("omnibarView"))->property("visible").toBool());
    QCOMPARE(tabs->activeTabId(), forest.away);
    QCOMPARE(tabs->currentGroupId(), forest.work);
    QVERIFY(tabs->activateTabById(forest.front));
    QCOMPARE(currentWebView(), webView);
    QVERIFY(learnt(m_core.data(), QStringLiteral("forest work"),
                   QStringLiteral("https://forest.example/work")));

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
    QVERIFY(!learnt(m_core.data(), QStringLiteral("forest.example"), searched));
}

// New-tab mode (cover search): empty field, pane up over whole page listing bookmarks, like
// sailfish-browser's new-tab overlay. No tab made until choice; choice opens in new tab.
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
    QVERIFY(learnt(m_core.data(), QStringLiteral("example.org"),
                   QStringLiteral("https://example.org")));
    evaluate(page, QStringLiteral("openOmnibar(true)"));
    typeIntoBar(root, QStringLiteral("forest work"));
    click(omnibarRows(root).first());
    QCOMPARE(tabs->count(), open + 2);
    QCOMPARE(tabs->activeTabId(), forest.away);

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

    // Regions tile bar; unreachable region is how first gesture handler failed.
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

    tapBar(QStringLiteral("back"));
    QVERIFY(webView->property("calls").toStringList().isEmpty());
    webView->setProperty("canGoBack", true);
    tapBar(QStringLiteral("back"));
    tapBar(QStringLiteral("reload"));
    QCOMPARE(webView->property("calls").toStringList(),
             QStringList({QStringLiteral("goBack"), QStringLiteral("reload")}));

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
    tapBar(QStringLiteral("reload"));
    QCOMPARE(webView->property("calls").toStringList().last(), QStringLiteral("stop"));
    webView->setProperty("loading", false);

    // Editing: field takes back/reload room, inset by padding (was half-wide with double margins).
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
    const int hostSize = addressLabel->property("font").value<QFont>().pixelSize();
    QCOMPARE(field->property("font").value<QFont>().pixelSize(), hostSize);
    QVERIFY(hostSize > evaluate(bar, QStringLiteral("Theme.fontSizeSmall")).toInt());
    evaluate(bar, QStringLiteral("endEditing()"));
    QVERIFY(find(QStringLiteral("backButton"))->property("visible").toBool());

    // Deck follows finger: gesture showing nothing until fired looks like system edge swipe on
    // device.
    QObject *page = find(QStringLiteral("browserPage"));
    const qreal threshold = page->property("pullThreshold").toReal();
    QVERIFY(threshold > 0);
    evaluate(bar, QStringLiteral("dragStarted()"));
    evaluate(bar, QStringLiteral("dragMoved(%1)").arg(threshold / 2));
    QCOMPARE(page->property("tabsOffset").toReal(), threshold / 2);
    evaluate(bar, QStringLiteral("dragFinished(%1)").arg(threshold / 2));
    QVERIFY(!page->property("tabsOpen").toBool());

    // Handler covers bar (first one sat behind controls, got no presses) and reaches above it, past
    // system bottom-edge swipe.
    QObject *gesture = find(QStringLiteral("navigationBarGesture"));
    QVERIFY(gesture->property("enabled").toBool());
    QCOMPARE(gesture->property("width").toReal(), barWidth);
    const qreal reach = gesture->property("reach").toReal();
    QVERIFY(reach > 0);
    QCOMPARE(gesture->property("height").toReal(), bar->property("height").toReal() + reach);

    pullUpToTabs();
    QVERIFY(page->property("tabsOpen").toBool());
    QCOMPARE(currentPage()->objectName(), QStringLiteral("browserPage"));
}

// Short address; broken connection drawn on it, only for pages claiming secure.
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

    tapBar(QStringLiteral("address"));
    QVERIFY(!warning->property("visible").toBool());
    evaluate(bar, QStringLiteral("endEditing()"));
    QVERIFY(warning->property("visible").toBool());

    security->setProperty("validState", false);
    QVERIFY(!bar->property("tlsBroken").toBool());
    security->setProperty("validState", true);
    QVERIFY(bar->property("tlsBroken").toBool());

    webView->setProperty("security", QVariant::fromValue<QObject *>(nullptr));
    QVERIFY(!bar->property("tlsBroken").toBool());

    m_core->tabs()->newTab(QStringLiteral("http://plain.example/"));
    QObject *plainView = currentWebView();
    QVERIFY(plainView != webView);
    plainView->property("security").value<QObject *>()->setProperty("allGood", false);
    QCOMPARE(find(QStringLiteral("addressLabel"))->property("text").toString(),
             QStringLiteral("plain.example"));
    QVERIFY(!bar->property("tlsBroken").toBool());
    QVERIFY(!warning->property("visible").toBool());
}

// Engine chrome gesture hides bar on scroll down, shows on scroll up. Without it page foot
// stays under bar: RawWebView::setFooterMargin only reaches engine while VKB up.
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

    // View sits between cutout and bar so first/last line never hidden. Overlay bar left last
    // rows unreachable on device.
    QObject *viewArea = find(QStringLiteral("viewArea"));
    QCOMPARE(viewArea->property("y").toReal(), inset);
    QCOMPARE(viewArea->property("height").toReal(), pageHeight - fullBar - inset);
    QCOMPARE(webView->property("height").toReal(), pageHeight - fullBar - inset);
    QCOMPARE(find(QStringLiteral("cutoutBand"))->property("height").toReal(), inset);
    QCOMPARE(bar->property("y").toReal(), pageHeight - fullBar);
    QVERIFY(webView->property("chromeGestureEnabled").toBool());
    // Threshold constant, not bar height: bar resizes in response, moving threshold would chase it.
    QCOMPARE(webView->property("chromeGestureThreshold").toReal(),
             evaluate(page, QStringLiteral("Theme.itemSizeLarge")).toReal());
    QCOMPARE(webView->property("safeAreaTop").toReal(), qreal(0));

    // View sized for slim height from first frame: resizes once, not every animation frame.
    const qreal slimBar = bar->property("slimHeight").toReal();
    QVERIFY(slimBar < fullBar);
    QVERIFY(slimBar > fullBar * 0.6);
    webView->setProperty("chrome", false);
    QVERIFY(page->property("barCompact").toBool());
    QVERIFY(bar->property("compact").toBool());
    QTRY_COMPARE(bar->property("height").toReal(), slimBar);
    QCOMPARE(bar->property("y").toReal(), pageHeight - slimBar);
    QCOMPARE(viewArea->property("height").toReal(), pageHeight - slimBar - inset);
    QObject *slimGesture = find(QStringLiteral("navigationBarGesture"));
    const qreal strip = slimGesture->property("strip").toReal();
    QCOMPARE(strip, slimBar);
    QCOMPARE(slimGesture->property("height").toReal(),
             strip + slimGesture->property("reach").toReal());

    // Background opaque: faded host unreadable over light page.
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

    tapBar(QStringLiteral("address"));
    QVERIFY(webView->property("chrome").toBool());
    QVERIFY(!bar->property("compact").toBool());
    QVERIFY(!bar->property("editing").toBool());
    tapBar(QStringLiteral("address"));
    QVERIFY(bar->property("editing").toBool());
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

    tapBar(QStringLiteral("address"));
    evaluate(bar, QStringLiteral("keyboardVisibilityChanged(true)"));
    QVERIFY(bar->property("editing").toBool());

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
    // Half size: grab lands mid-gesture, grid never draws picture wider than half screen.
    QCOMPARE(webView->property("lastGrabSize").toSize().width(),
             int(webView->property("width").toReal() / 2));
    // Model encodes/writes picture on worker; GUI thread never saves (grab-callback save stuttered
    // grid-opening drag, issue #27).
    QVERIFY(webView->property("lastGrabPath").toString().isEmpty());
    QTRY_VERIFY(!thumbnail().isEmpty());
    const QString captured = thumbnail();
    QVERIFY(QFile::exists(captured));

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

    // Engine doesn't expose theme colour: page asked by script.
    webView->setProperty("scriptResult", QStringLiteral("#123456"));
    webView->setProperty("loading", true);
    webView->setProperty("loading", false);
    QVERIFY(webView->property("scripts").toStringList().contains(
        m_core->engineMessages()->themeColorScript()));
    QCOMPARE(webView->property("pageThemeColor").toString(), QStringLiteral("#123456"));
    QCOMPARE(find(QStringLiteral("cutoutBand"))->property("color").value<QColor>(),
             QColor(QStringLiteral("#123456")));

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
    auto *gridView = qobject_cast<QQuickItem *>(grid);
    QCOMPARE(headRow->mapToItem(gridView, QPointF(0, 0)).y(), qreal(0));
    QCOMPARE(footRow->mapToItem(gridView, QPointF(0, footRow->height())).y(), gridView->height());
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
        QVERIFY(corner->childItems().isEmpty());
    }
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
    QList<QObject *> underlines = findAll(QStringLiteral("tabGroupUnderline"));
    QCOMPARE(underlines.count(), 1);
    QVERIFY(underlines.first()->property("visible").toBool());

    // Head and corner close button once sat under notch.
    const qreal cutout = grid->property("cutoutHeight").toReal();
    QVERIFY(cutout > 0);
    QVERIFY(headRow->height() > cutout);
    QVERIFY(item("gridHeadControls")->y() >= cutout);
    QObject *indicator = find(QStringLiteral("gridPullIndicator"));
    QVERIFY(indicator != nullptr);
    QCOMPARE(indicator->property("y").toReal(), 0.0);
    // Highlight background colour per stub theme.
    QCOMPARE(indicator->property("color").value<QColor>(), QColor(QStringLiteral("#aaccff")));
    QCOMPARE(indicator->property("width").toReal(), headRow->width());
    QVERIFY(indicator->property("height").toReal() > 0);
    QVERIFY(find(QStringLiteral("gridDragHandle")) == nullptr);
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
    auto *headerItem = find(QStringLiteral("tabGrid"))->property("headerItem").value<QObject *>();
    QVERIFY(headerItem != nullptr);
    QCOMPARE(headerItem->property("height").toReal(), headRow->height());
    auto *footerItem = find(QStringLiteral("tabGrid"))->property("footerItem").value<QObject *>();
    QVERIFY(footerItem != nullptr);
    QCOMPARE(footerItem->property("height").toReal(), footRow->height());

    QList<QObject *> previews = findAll(QStringLiteral("tabPreview"));
    QCOMPARE(previews.count(), 2);

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

    QMetaObject::invokeMethod(previews.at(0), "tapped");
    QCOMPARE(m_core->tabs()->activeTabIndex(), 0);
    QVERIFY(!page->property("tabsOpen").toBool());
    QCOMPARE(currentWebView()->property("url").toUrl().toString(), QLatin1String(FirstPage));

    QObject *homeView = currentWebView();
    QCOMPARE(homeView->property("grabCount").toInt(), 0);
    pullUpToTabs();
    QCOMPARE(homeView->property("grabCount").toInt(), 1);

    previews = findAll(QStringLiteral("tabPreview"));
    click(findObjects(previews.at(1), QStringLiteral("closeTabButton")).first());
    QCOMPARE(m_core->tabs()->count(), 1);
    QCOMPARE(findAll(QStringLiteral("tabPreview")).count(), 1);

    QVERIFY(page->property("tabsOpen").toBool());
    pullDownToBrowser();
    QVERIFY(!page->property("tabsOpen").toBool());

    pullUpToTabs();
    const qreal threshold = page->property("pullThreshold").toReal();
    evaluate(grid, QStringLiteral("pullStarted()"));
    evaluate(grid, QStringLiteral("pulled(%1)").arg(threshold / 2));
    QCOMPARE(page->property("tabsOffset").toReal(),
             page->property("height").toReal() - threshold / 2);
    evaluate(grid, QStringLiteral("pullFinished(%1)").arg(threshold / 2));
    QVERIFY(page->property("tabsOpen").toBool());

    // Pull = view's overscroll: past top it reports distance and moves up same amount, cancelling
    // flickable shift so content stays under finger.
    QObject *view = find(QStringLiteral("tabGrid"));
    const qreal originY = view->property("originY").toReal();
    view->setProperty("contentY", originY - threshold);
    QCOMPARE(view->property("overscroll").toReal(), threshold);
    QCOMPARE(view->property("y").toReal(), -threshold);
    view->setProperty("contentY", originY);
    QCOMPARE(view->property("overscroll").toReal(), qreal(0));
    QCOMPARE(view->property("y").toReal(), qreal(0));

    // Carrying cell reorders tabs. Moved tab's view carried, not rebuilt: Repeater recreating
    // delegates on move would reload page.
    m_core->tabs()->newTab(QStringLiteral("https://three.example/"));
    pullUpToTabs();
    QObject *carried = currentWebView();
    const int carriedId = m_core->tabs()->activeTabId();
    QCOMPARE(m_core->tabs()->activeTabIndex(), 1);
    previews = findAll(QStringLiteral("tabPreview"));
    QCOMPARE(previews.count(), 2);
    // Carried cell must not open on release (used to drop grid, jump to tab).
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

    QObject *strip = find(QStringLiteral("tabGroupStrip"));
    QVERIFY(strip != nullptr);
    QCOMPARE(findAll(QStringLiteral("tabGroupItem")).count(), 1);
    QCOMPARE(tabs->currentGroupIndex(), 0);

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

    click(delegates.at(1));
    QCOMPARE(currentPage()->objectName(), QStringLiteral("browserPage"));
    QVERIFY(page->property("tabsOpen").toBool());
    QCOMPARE(findAll(QStringLiteral("tabPreview")).count(), 0);
    QCOMPARE(findAll(QStringLiteral("tabGroupItem")).count(), 2);
    QCOMPARE(tabs->currentGroupIndex(), 1);
    QVERIFY(findAll(QStringLiteral("tabGroupUnderline")).at(1)->property("visible").toBool());
    QCOMPARE(tabs->activeTabId(), first);

    click(find(QStringLiteral("newTabButton")));
    const int second = tabs->activeTabId();
    QVERIFY(second != first);
    QCOMPARE(tabs->tabCountInGroup(work), 1);
    QVERIFY(!page->property("tabsOpen").toBool());
    QVERIFY(currentWebView() == nullptr);
    typeAddress(QStringLiteral("work.example"));
    QCOMPARE(tabs->activeTabId(), second);
    pullUpToTabs();
    QCOMPARE(findAll(QStringLiteral("tabPreview")).count(), 1);
    QCOMPARE(findAll(QStringLiteral("webView")).count(), 2);

    evaluate(strip, QStringLiteral("select(0)"));
    QCOMPARE(tabs->currentGroupId(), home);
    QCOMPARE(tabs->activeTabId(), first);
    QCOMPARE(findAll(QStringLiteral("tabPreview")).count(), 1);
    QCOMPARE(findAll(QStringLiteral("webView")).count(), 2);

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

// Group list row: tabs picture, name, count, grip, menu (none on default). Under last row,
// same-height row creating group via name dialog.
void tst_qmlload::tabGroupRows()
{
    TabModel *tabs = m_core->tabs();
    const int first = tabs->activeTabId();
    const int work = tabs->addGroup(QStringLiteral("Work"));
    pullUpToTabs();
    const QString shot = tabs->thumbnailPath(first);
    tabs->updateThumbnail(first, shot);
    click(find(QStringLiteral("editGroupsButton")));
    QCOMPARE(currentPage()->objectName(), QStringLiteral("tabGroupsPage"));
    const QList<QObject *> rows = byRow(findAll(QStringLiteral("tabGroupDelegate")));
    QCOMPARE(rows.count(), 2);

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
    QCOMPARE(findObjects(workPicture, QStringLiteral("tabGroupCollageFrame"))
                 .first()
                 ->property("radius")
                 .toReal(),
             qreal(0));

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

    click(findObjects(rows.at(1), QStringLiteral("renameGroupMenu")).first());
    QCOMPARE(currentPage()->property("groupId").toInt(), work);
    header = find(QStringLiteral("tabGroupDialogHeader"));
    QCOMPARE(header->property("title").toString(), QStringLiteral("Rename tab group"));
    QCOMPARE(header->property("acceptText").toString(), QStringLiteral("Save"));
    popPage();
    popPage();
}

// Grip carry swaps with group its middle enters, lit while held; default stays first; strip
// follows order. Ungroup: group gone, tabs stay open in default group, grid follows.
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
    // Strip row lays names out in child order on next frame.
    const auto stripName = [this](int place) {
        return textOf(qobject_cast<QQuickItem *>(find(QStringLiteral("tabGroupItem")))
                          ->parentItem()
                          ->childItems()
                          .at(place),
                      "tabGroupLabel");
    };
    QCOMPARE(stripName(1), QStringLiteral("Mail"));
    QCOMPARE(stripName(2), QStringLiteral("Play"));
    // Passed row animates aside; read after it settles.
    QTRY_COMPARE(textOf(byRow(findAll(QStringLiteral("tabGroupDelegate"))).at(1), "tabGroupName"),
                 QStringLiteral("Mail"));

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

// Grid head field lists matching tabs by group instead of cells; tap -> front, grid away. Nothing
// pushed.
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
    field->setProperty("text", QString());
    QVERIFY(!found->property("visible").toBool());
    QVERIFY(cellsShown());
    field->setProperty("text", QStringLiteral("MAIL two"));
    QMetaObject::invokeMethod(debounce, "triggered");
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
    QVERIFY(m_core->tabSearch()->searchTerm().isEmpty());
    QVERIFY(field->property("text").toString().isEmpty());
    QVERIFY(!debounce->property("running").toBool());
    QVERIFY(cellsShown());
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

// Tab dropped on strip group moves there via strip's own functions (like tap on name). Finger
// version: carryToGroupUnderAFinger().
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

    QCOMPARE(strip->property("dropIndex").toInt(), -1);
    strip->setProperty("dropIndex", 1);
    QVERIFY(evaluate(strip, QStringLiteral("dropTab(%1)").arg(second)).toBool());
    QCOMPARE(strip->property("dropIndex").toInt(), -1);
    // Deferred: carried cell's release still running at lift, move removes cell from grid.
    QCOMPARE(tabs->tabCountInGroup(home), 2);
    QTRY_COMPARE(tabs->tabCountInGroup(work), 1);
    QCOMPARE(tabs->currentGroupId(), work);
    QCOMPARE(tabs->activeTabId(), second);
    QCOMPARE(findAll(QStringLiteral("webView")).count(), 2);
    QVERIFY(page->property("tabsOpen").toBool());

    QVERIFY(!evaluate(strip, QStringLiteral("dropTab(%1)").arg(second)).toBool());
    QCoreApplication::processEvents();
    QCOMPARE(tabs->tabCountInGroup(work), 1);
    strip->setProperty("dropIndex", 0);
    QVERIFY(!evaluate(strip, QStringLiteral("carryOver(null, -1, -1)")).toBool());
    QCOMPARE(strip->property("dropIndex").toInt(), -1);
    strip->setProperty("dropIndex", 0);
    evaluate(strip, QStringLiteral("endCarry()"));
    QCOMPARE(strip->property("dropIndex").toInt(), -1);

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

    QObject *timer = findObjects(cell, QStringLiteral("holdTimer")).first();
    QCOMPARE(timer->property("interval").toInt(), 1000);
    QCOMPARE(cell->property("holdInterval").toInt(), 1000);
    // Grid may take drag while hold forming (flickable refused a touch never retakes it -> no
    // scroll/pull from cell); not once cell up.
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

    const qreal width = cell->property("width").toReal();
    QCOMPARE(cell->property("closeDistance").toReal(), width / 3);
    evaluate(cell, QStringLiteral("swipeTo(-10)"));
    QVERIFY(cell->property("swiping").toBool());
    QVERIFY(cell->property("carried").toBool());
    evaluate(cell, QStringLiteral("releaseSwipe()"));
    QVERIFY(!cell->property("swiping").toBool());
    QCOMPARE(tabs->count(), 2);
    evaluate(cell, QStringLiteral("swipeTo(50)"));
    evaluate(cell, QStringLiteral("releaseSwipe()"));
    QCOMPARE(tabs->count(), 2);

    evaluate(cell, QStringLiteral("swipeTo(%1)").arg(-width / 2));
    evaluate(cell, QStringLiteral("releaseSwipe()"));
    QCOMPARE(tabs->count(), 1);
    QCOMPARE(findAll(QStringLiteral("tabPreview")).count(), 1);
    QVERIFY(page->property("tabsOpen").toBool());

    // Disc colour carries faintness, not item opacity: cross stays opaque.
    QObject *mark = find(QStringLiteral("closeTabMark"));
    QVERIFY(mark != nullptr);
    const QColor disc = mark->property("color").value<QColor>();
    QVERIFY(qAbs(disc.alphaF() - 0.5) < 0.01);
    QCOMPARE(disc.rgb(),
             evaluate(mark, QStringLiteral("Theme.overlayBackgroundColor")).value<QColor>().rgb());
    QCOMPARE(mark->property("opacity").toReal(), 1.0);
    const QList<QQuickItem *> cross = qobject_cast<QQuickItem *>(mark)->childItems();
    QCOMPARE(cross.count(), 3); // 2 strokes + their Repeater
    const qreal discWidth = mark->property("width").toReal();
    for (QQuickItem *stroke : cross) {
        if (stroke->property("rotation").toReal() != 0) {
            QCOMPARE(stroke->property("color").value<QColor>(),
                     evaluate(mark, QStringLiteral("Theme.primaryColor")).value<QColor>());
            QCOMPARE(stroke->height(), evaluate(mark, QStringLiteral("Theme._lineWidth")).toReal());
            QCOMPARE(stroke->width(), discWidth * 2 / 5);
        }
    }
    QCOMPARE(mark->property("radius").toReal(), discWidth / 2);
    QVERIFY(find(QStringLiteral("closeTabDisc")) == nullptr);
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

// App in real window while alive: for gestures decided by Qt event delivery.
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

// Collects QML script errors while alive: throwing handler otherwise just scrolls past.
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

// Grid gestures with real finger in real window. Whether drag from cell reaches grid is decided
// in Qt delivery (cell keeping touch from press starves flickable); signal-raising can't see it.
// Regression: grid once only pullable from gaps between cells.
void tst_qmlload::gridGesturesUnderAFinger()
{
    TabModel *tabs = m_core->tabs();
    const int second = tabs->newTab(QStringLiteral("https://two.example/"));
    QObject *page = find(QStringLiteral("browserPage"));
    auto *root = qobject_cast<QQuickItem *>(m_window.data());
    FingerWindow host(root);
    QQuickWindow &window = *host.window();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    // Press on moving grid stops it, doesn't reach cell (deck settling first doesn't rule it out).
    QObject *gridView = find(QStringLiteral("tabGrid"));
    const auto openGrid = [&]() {
        pullUpToTabs();
        QTRY_COMPARE(page->property("tabsOffset").toReal(), page->property("fullHeight").toReal());
        QTRY_VERIFY(!gridView->property("moving").toBool());
    };
    const auto cells = [&]() { return byRow(findAll(QStringLiteral("tabPreview"))); };
    const QPoint down(0, 3 * page->property("pullThreshold").toInt());

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

    openGrid();
    QCOMPARE(tabs->activeTabId(), second);
    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, centreOf(cells().first()));
    QVERIFY(!page->property("tabsOpen").toBool());
    QVERIFY(tabs->activeTabId() != second);

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

    // Left slide closes, even slanted: grid would steal slide drifting down drag distance before
    // crossing hold tolerance.
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

// Drag down head row returns page from any scroll; elsewhere long grid scrolls to top first.
// Row overlies search field: must pass field its taps, clear-button presses, drag-up to grid.
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

    openGrid();
    const QPoint head = centreOf(find(QStringLiteral("gridHeadControls")));
    const qreal scrolled =
        grid->property("contentY").toReal() + grid->property("cellHeight").toReal();
    grid->setProperty("contentY", scrolled);
    drag(&window, head, head + QPoint(0, 3 * int(threshold)));
    QVERIFY(!page->property("tabsOpen").toBool());
    QTRY_COMPARE(page->property("tabsOffset").toReal(), qreal(0));
    QCOMPARE(grid->property("contentY").toReal(), scrolled);

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

    openGrid();
    QCOMPARE(grid->property("contentY").toReal(), scrolled);
    drag(&window, head, head + QPoint(0, int(threshold) / 2));
    QVERIFY(page->property("tabsOpen").toBool());
    QCOMPARE(grid->property("contentY").toReal(), scrolled);
    QVERIFY(!field->hasFocus());

    drag(&window, head, head - QPoint(0, head.y() * 3 / 4));
    QTRY_VERIFY(!grid->property("moving").toBool());
    QVERIFY(grid->property("contentY").toReal() > scrolled);
    QVERIFY(page->property("tabsOpen").toBool());
    QVERIFY(!field->hasFocus());

    // Stub field lacks clear button; this one counts taps.
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

// Real finger: held cell carried onto strip name moves tab there. Strip lights name under
// finger; cells hidden under strip not swapped.
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
    // Immediate move removes carried cell mid-release; rest of handler throws.
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
    QObject *wash = findAll(QStringLiteral("tabGroupDropHighlight")).at(1);
    QCOMPARE(wash->property("radius").toReal(),
             evaluate(wash, QStringLiteral("Theme.paddingSmall")).toReal());
    QCOMPARE(wash->property("color").value<QColor>(),
             evaluate(wash, QStringLiteral("Theme.rgba(Theme.highlightBackgroundColor,"
                                           " Theme.highlightBackgroundOpacity)"))
                 .value<QColor>());
    QCOMPARE(workLabel->property("color").value<QColor>(), QColor(QStringLiteral("#aaccff")));
    QCOMPARE(tabOrder(), order);

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, workName);
    QTRY_COMPARE(tabs->tabCountInGroup(work), 1);
    QCOMPARE(tabs->tabCountInGroup(home), 1);
    QCOMPARE(tabs->currentGroupId(), home);
    QCOMPARE(tabs->activeTabId(), second);
    QVERIFY(page->property("tabsOpen").toBool());
    QCOMPARE(strip->property("dropIndex").toInt(), -1);
    QTRY_COMPARE(findAll(QStringLiteral("tabPreview")).count(), 1);
    QCOMPARE(highlights(), 0);

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

// Over strip finger picks group not place: no swap with cells under strip; grid-cancelled carry
// leaves nothing lit; scrolled-out name not a drop target.
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
    QVERIFY(cellUnder(left) >= 0);
    QVERIFY(cellUnder(right) >= 0);

    QObject *cell = cellAt(0);
    QPoint at = lift(cell);
    QTRY_VERIFY(cell->property("held").toBool());
    carry(at, QPoint(at.x(), stripY));
    const QList<int> onTheStrip = tabOrder();
    carry(QPoint(at.x(), stripY), right);
    QCOMPARE(tabOrder(), onTheStrip);
    QCOMPARE(strip->property("dropIndex").toInt(), -1);
    const QPoint corner = centreOf(find(QStringLiteral("editGroupsButton")));
    QCOMPARE(corner.y(), stripY);
    QVERIFY(cellUnder(corner) >= 0);
    QVERIFY(cellUnder(corner) != evaluate(cell, QStringLiteral("index")).toInt());
    carry(right, corner);
    QCOMPARE(tabOrder(), onTheStrip);
    QCOMPARE(strip->property("dropIndex").toInt(), -1);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, corner);
    QCoreApplication::processEvents();
    QCOMPARE(tabs->tabCountInGroup(home), 10);
    QCOMPARE(currentPage()->objectName(), QStringLiteral("browserPage"));
    QVERIFY(page->property("tabsOpen").toBool());

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

// Overflowing strip fades slightly only at end with hidden names; fitting row no fade.
void tst_qmlload::tabGroupStripFades()
{
    TabModel *tabs = m_core->tabs();
    // Window needed: row lays out when drawn.
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
    tabs->groupModel()->activate(0);
    QTRY_VERIFY(names->property("interactive").toBool());
    QTRY_COMPARE(names->property("contentX").toReal(), qreal(0));
    QVERIFY(!left->property("enabled").toBool());
    QVERIFY(right->property("enabled").toBool());
    QCOMPARE(right->property("sourceItem").value<QObject *>(), names);
    QCOMPARE(right->property("direction").toInt(), 0);
    const qreal slope = right->property("slope").toReal();
    const qreal screenWidth = evaluate(names, QStringLiteral("Screen.width")).toReal();
    QVERIFY(names->width() / slope <= screenWidth / 20);
    QCOMPARE(right->property("offset").toReal(), 1 - 1 / slope);

    tabs->groupModel()->activate(4);
    QTRY_VERIFY(left->property("enabled").toBool());
    QVERIFY(right->property("enabled").toBool());
    QCOMPARE(right->property("sourceItem").value<QObject *>(), left);
    QCOMPARE(left->property("sourceItem").value<QObject *>(), names);
    QCOMPARE(left->property("direction").toInt(), 1);

    tabs->groupModel()->activate(8);
    QTRY_VERIFY(!right->property("enabled").toBool());
    QVERIFY(left->property("enabled").toBool());
    QCOMPARE(names->property("contentX").toReal(),
             names->property("contentWidth").toReal() - names->width());
}

// Real finger grip reorder: whether list leaves grip drag vs scrolls is decided in Qt delivery,
// which calling grip functions skips.
void tst_qmlload::tabGroupsReorderUnderAFinger()
{
    TabModel *tabs = m_core->tabs();
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

    rows = byRow(findAll(QStringLiteral("tabGroupDelegate")));
    const QPoint name = centreOf(findObjects(rows.at(2), QStringLiteral("tabGroupName")).first());
    drag(&window, name, name - QPoint(0, rowHeight * 3));
    QTRY_VERIFY(!list->property("moving").toBool());
    QVERIFY(list->property("contentY").toReal() > top);
    QCOMPARE(tabs->groups().at(3).id, moved);
    popPage();
}

// Issue #27: bar drag start stuttered. First frame had: deck leap of Theme.startDragDistance
// (measured from press, not catch point); first grid draw uploading all previews; tab PNG
// encode on GUI thread. Real finger: picture + grid prepared while finger down and still; deck
// then tracks finger from catch point pixel for pixel.
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

    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(x, onBar));
    QCOMPARE(webView->property("grabCount").toInt(), grabs + 1);
    QVERIFY(grid->property("visible").toBool());
    QCOMPARE(offset(), qreal(0));

    int y = onBar;
    while (!gesture->property("dragging").toBool()) {
        QVERIFY(onBar - y <= shake);
        QCOMPARE(offset(), qreal(0));
        QTest::mouseMove(&window, QPoint(x, --y));
    }
    QCOMPARE(offset(), qreal(0));
    QVERIFY(page->property("dragging").toBool());
    QCOMPARE(webView->property("grabCount").toInt(), grabs + 1);

    const int caught = y;
    for (int step = 1; step <= 10; ++step) {
        QTest::mouseMove(&window, QPoint(x, caught - step * 4));
        QCOMPARE(offset(), qreal(step * 4));
    }
    QVERIFY(40 < threshold);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(x, caught - 40));
    QVERIFY(!page->property("tabsOpen").toBool());
    QTRY_COMPARE(offset(), qreal(0));
    QVERIFY(!grid->property("visible").toBool());

    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(x, inReach));
    QVERIFY(grid->property("visible").toBool());
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(x, inReach));
    QVERIFY(!grid->property("visible").toBool());
    drag(&window, QPoint(x, inReach), QPoint(x + 10 * shake, inReach));
    QVERIFY(!grid->property("visible").toBool());

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

// Issue #27, scrolled page with slim bar: drag made bar whole, bar grew under finger ~200 ms and
// view resized mid-drag (relayout) and again on spring back. Bar must stay slim.
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

// Reach above bar overlaps page foot (player seek bar, buttons). Keeps only drag up (opens grid);
// tap, sideways/down drag go to page. Held press kept: handle sits in reach.
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

    drag(&window, QPoint(200, inReach), QPoint(700, inReach));
    QCOMPARE(touch(2).value(QStringLiteral("phase")).toString(), QStringLiteral("begin"));
    QCOMPARE(touch(2).value(QStringLiteral("x")).toReal(), local.x());
    QVERIFY(touches().count() > 5);
    QCOMPARE(touches().last().toMap().value(QStringLiteral("phase")).toString(),
             QStringLiteral("end"));
    QCOMPARE(touches().last().toMap().value(QStringLiteral("x")).toReal(),
             view->mapFromScene(QPointF(700, inReach)).x());
    QVERIFY(!page->property("tabsOpen").toBool());

    int before = touches().count();
    drag(&window, QPoint(300, int(gestureTop.y()) + 1), QPoint(300, inReach + shake));
    QVERIFY(touches().count() > before + 2);
    before = touches().count();
    drag(&window, QPoint(300, inReach), QPoint(300 + shake / 2, inReach));
    QCOMPARE(touches().count(), before + 2);

    before = touches().count();
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(300, inReach));
    QTRY_VERIFY(gesture->property("heldDown").toBool());
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(300, inReach));
    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier,
                      QPoint(int(gesture->width()) / 2, onBar));
    QCOMPARE(touches().count(), before);
    QVERIFY(find(QStringLiteral("navigationBar"))->property("editing").toBool());
    evaluate(find(QStringLiteral("navigationBar")), QStringLiteral("endEditing()"));
    const QPoint address(int(gesture->width()) / 2, onBar);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, address);
    QTest::qWait(QGuiApplication::styleHints()->mousePressAndHoldInterval() + 200);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, address);
    QVERIFY(find(QStringLiteral("navigationBar"))->property("editing").toBool());
    evaluate(find(QStringLiteral("navigationBar")), QStringLiteral("endEditing()"));
    QCOMPARE(touches().count(), before);

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

    QObject *panel = find(QStringLiteral("recentlyClosedPanel"));
    QVERIFY(panel != nullptr);
    QVERIFY(!panel->property("open").toBool());
    QVERIFY(panel->property("modal").toBool());
    QMetaObject::invokeMethod(find(QStringLiteral("newTabButton")), "pressAndHold");
    QVERIFY(panel->property("open").toBool());
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
    // Handle on panel's top edge, as on nav bar (#38).
    QCOMPARE(handle->mapToScene(QPointF()).y() -
                 qobject_cast<QQuickItem *>(panel)->mapToScene(QPointF()).y(),
             -handle->height() / 2);
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
    // 5 as in Jolla browser; 3 here so 4 tabs exceed it.
    QCOMPARE(tabs->liveTabLimit(), int(TabModel::LiveTabLimit));
    QCOMPARE(tabs->liveTabLimit(), 5);
    tabs->setLiveTabLimit(3);

    // Read via BrowserPage.qml item: its scope has engine singleton, inline browsing component's
    // doesn't.
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
    tabs->newTab(QStringLiteral("https://four.example/"));
    QCOMPARE(findAll(QStringLiteral("webView")).count(), 3);
    QList<QObject *> loaders = findAll(QStringLiteral("webViewLoader"));
    QCOMPARE(loaders.count(), 4);
    QVERIFY(!loaders.at(0)->property("active").toBool());
    QVERIFY(loaders.at(3)->property("active").toBool());

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
    QCOMPARE(m_core->history()->count(), 3);

    m_core->tabs()->activateTab(2);
    QCOMPARE(findAll(QStringLiteral("webView")).count(), 2);
    QCOMPARE(currentWebView()->property("url").toUrl().toString(),
             QStringLiteral("https://three.example/"));
    QCOMPARE(m_core->history()->count(), 3);
}

// Menu button -> icon sheet from under bar, two rows: page, browser. Nothing pushed; each icon
// hides it. No new tab (grid's plus).
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
    QObject *reader = find(QStringLiteral("readerMenuButton"));
    QVERIFY(reader != nullptr);
    QVERIFY(!reader->property("enabled").toBool());
    QVERIFY(!reader->property("iconSource").toString().isEmpty());
    QCOMPARE(reader->property("text").toString(), QStringLiteral("Reader view"));
    // Via BrowserPage.qml item: its scope names page and menu.
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
    QVERIFY(find(QStringLiteral("tabsItem")) == nullptr);
    QVERIFY(find(QStringLiteral("moveToGroupItem")) == nullptr);

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

    tapBar(QStringLiteral("menu"));
    QObject *share = find(QStringLiteral("shareAction"));
    click(find(QStringLiteral("shareMenuButton")));
    QCOMPARE(share->property("triggerCount").toInt(), 1);
    QCOMPARE(share->property("mimeType").toString(), QStringLiteral("text/x-url"));
    const QVariantMap resource = share->property("resources").toList().first().toMap();
    QCOMPARE(resource.value(QStringLiteral("status")).toString(), tabs->activeUrl());
    QVERIFY(!menu->property("open").toBool());

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
    tapBar(QStringLiteral("menu"));
    click(desktop);
    QVERIFY(!front->property("desktopMode").toBool());

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

    tapBar(QStringLiteral("menu"));
    tabs->newTab(QStringLiteral("https://three.example/"));
    QVERIFY(!menu->property("open").toBool());
}

// Stub-visible sheet: 5 page actions on discs in one row, line, 4 browser ones without; opaque
// ground; on switch or pressed entry lit (disc, icon, name), no wash.
void tst_qmlload::menuSheetLayout()
{
    QObject *menu = find(QStringLiteral("browserMenu"));
    tapBar(QStringLiteral("menu"));
    QVERIFY(menu->property("open").toBool());
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
    auto *line = qobject_cast<QQuickItem *>(find(QStringLiteral("menuSeparator")));
    QVERIFY(line != nullptr);
    const qreal lineY = line->mapToScene(QPointF()).y();
    QVERIFY(lineY > rowY + find(QStringLiteral("findMenuButton"))->property("height").toReal() - 1);
    QVERIFY(lineY < qobject_cast<QQuickItem *>(find(QStringLiteral("historyMenuButton")))
                        ->mapToScene(QPointF())
                        .y());
    const QList<QQuickItem *> halves = line->childItems();
    QCOMPARE(halves.count(), 2);
    QCOMPARE(halves.at(0)->rotation(), 180.0);
    QCOMPARE(halves.at(1)->rotation(), 0.0);
    QObject *ground = findObjects(menu, QStringLiteral("sheetBackground")).first();
    QCOMPARE(ground->property("color").value<QColor>().alphaF(), 1.0);
    QCOMPARE(ground->property("color").value<QColor>(),
             evaluate(menu, QStringLiteral("Theme.highlightDimmerColor")).value<QColor>());
    QCOMPARE(ground->property("width").toReal(), sheetItem->width());
    QCOMPARE(ground->property("height").toReal(), sheetItem->height());
    QVERIFY(findObjects(menu, QStringLiteral("menuDragHandle")).count() == 1);
    // Handle on sheet's top edge, as on nav bar (#38).
    auto *handle = qobject_cast<QQuickItem *>(find(QStringLiteral("menuDragHandle")));
    QCOMPARE(handle->mapToScene(QPointF()).y() - sheetItem->mapToScene(QPointF()).y(),
             -handle->height() / 2);

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

// Sheet head names page (icon, title, padlock for https + host), copies address. Start page:
// head says so, page actions dimmed.
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

    QQuickItem *header = item("menuHeader");
    QVERIFY(header != nullptr);
    QVERIFY(sceneY(header) > sceneY(item("menuDragHandle")));
    QVERIFY(sceneY(header) + header->height() <= sceneY(item("findMenuButton")));
    // Empty band, no line, under header; as tall as gap between rows, ink to ink (#38).
    QVERIFY(find(QStringLiteral("menuHeaderSeparator")) == nullptr);
    const auto part = [](QQuickItem *button, const char *name) {
        return qobject_cast<QQuickItem *>(findObjects(button, QLatin1String(name)).first());
    };
    QQuickItem *firstRow = item("findMenuButton");
    QQuickItem *secondRow = item("bookmarksMenuButton");
    const qreal headerGap =
        sceneY(part(firstRow, "menuButtonIconSlot")) -
        (sceneY(header) + header->height() - header->property("inkMargin").toReal());
    QQuickItem *firstLabel = part(firstRow, "menuButtonLabel");
    const qreal rowGap =
        sceneY(part(secondRow, "menuButtonIconSlot")) - (sceneY(firstLabel) + firstLabel->height());
    QVERIFY(rowGap > 0);
    QCOMPARE(headerGap, rowGap);
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
    QVERIFY(!shown("menuPageFavicon"));
    QVERIFY(shown("menuPageInitial"));
    QCOMPARE(text("menuPageInitial"), QStringLiteral("Q"));
    tabs->updateFavicon(front, QStringLiteral("image://theme/qwant-favicon"));
    QCOMPARE(item("menuPageFavicon")->property("source").toString(),
             QStringLiteral("image://theme/qwant-favicon"));
    QTRY_VERIFY(shown("menuPageFavicon"));
    QVERIFY(!shown("menuPageInitial"));
    tabs->updateTitle(front, QString());
    QCOMPARE(text("menuPageTitle"), QStringLiteral("qwant.com"));
    tabs->updateTitle(front, title);

    auto *security = currentWebView()->property("security").value<QObject *>();
    security->setProperty("allGood", false);
    QVERIFY(menu->property("tlsBroken").toBool());
    QCOMPARE(item("menuPageSecurity")->property("source").toString(),
             QStringLiteral("image://theme/icon-s-filled-warning"));
    QCOMPARE(item("menuPageSecurity")->property("color").value<QColor>(),
             evaluate(menu, QStringLiteral("Theme.errorColor")).value<QColor>());
    security->setProperty("allGood", true);
    QVERIFY(!menu->property("tlsBroken").toBool());

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

    typeAddress(QStringLiteral("http://plain.example/"));
    tapBar(QStringLiteral("menu"));
    QCOMPARE(text("menuPageHost"), QStringLiteral("plain.example"));
    QVERIFY(!shown("menuPageSecurity"));
    evaluate(menu, QStringLiteral("hide()"));

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
    for (const char *entry : {"bookmarksMenuButton", "historyMenuButton", "downloadsMenuButton",
                              "settingsMenuButton"}) {
        QVERIFY2(item(entry)->isEnabled(), entry);
        QCOMPARE(item(entry)->opacity(), 1.0);
    }
}

// Downloads ring shows combined progress while any running; none when all done.
void tst_qmlload::menuShowsDownloadsComing()
{
    // BrowserPage.qml item: scope has engine.
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
    for (const char *entry : {"findMenuButton", "historyMenuButton", "settingsMenuButton"}) {
        QVERIFY(!findObjects(find(QLatin1String(entry)), QStringLiteral("menuButtonProgress"))
                     .first()
                     ->property("visible")
                     .toBool());
    }
}

// Finger pull (even from icon) moves sheet down, icon under finger: past short distance hides,
// short returns. Stub icons take no presses, so only sheet pull proven; Silica buttons passing
// drag up is device-only.
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

    // Sheet moves ~2/3 of finger travel: flickable reads finger in own coords and moves with sheet.
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

    const int longPull = int(closeDistance * 2);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, grab);
    pullTo(longPull);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, grab + QPoint(0, longPull));
    QVERIFY(!menu->property("open").toBool());
    QCOMPARE(currentPage()->objectName(), QStringLiteral("browserPage"));

    tapBar(QStringLiteral("menu"));
    QTRY_COMPARE(menu->y(), openY);
    QTRY_COMPARE(find(QStringLiteral("menuSheet"))->property("y").toReal(), qreal(0));
}

// Fixed-size sheet (#38): drag up from icon moves nothing, no fling, no quick scroll.
void tst_qmlload::menuSheetDoesNotScroll()
{
    auto *root = qobject_cast<QQuickItem *>(m_window.data());
    FingerWindow host(root);
    QQuickWindow &window = *host.window();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto *menu = qobject_cast<QQuickItem *>(find(QStringLiteral("browserMenu")));
    tapBar(QStringLiteral("menu"));
    QVERIFY(menu->property("open").toBool());
    QObject *sheet = find(QStringLiteral("menuSheet"));
    const qreal openY = menu->y();
    const qreal restY = sheet->property("contentY").toReal();
    QVERIFY(!sheet->property("quickScroll").toBool());
    QCOMPARE(sheet->property("maximumFlickVelocity").toReal(), qreal(0));
    const auto atRest = [&sheet, restY]() {
        return qAbs(sheet->property("contentY").toReal() - restY) < 0.5;
    };
    QVERIFY(atRest());

    auto *icon = qobject_cast<QQuickItem *>(find(QStringLiteral("historyMenuButton")));
    const QPoint grab = centreOf(icon);
    const qreal iconY = icon->mapToScene(QPointF(0, 0)).y();
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, grab);
    for (int step = 1; step <= 12; ++step) {
        QTest::mouseMove(&window, grab - QPoint(0, 240 * step / 12));
    }
    QVERIFY(atRest());
    QCOMPARE(menu->y(), openY);
    QVERIFY(qAbs(icon->mapToScene(QPointF(0, 0)).y() - iconY) < 1);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, grab - QPoint(0, 240));
    QVERIFY(atRest());
    QCOMPARE(menu->y(), openY);
    QVERIFY(menu->property("open").toBool());

    // Pull down still the sheet's own after an attempted scroll.
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, grab);
    for (int step = 1; step <= 12; ++step) {
        QTest::mouseMove(&window, grab + QPoint(0, 240 * step / 12));
    }
    QVERIFY(menu->y() > openY);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, grab + QPoint(0, 240));
    QTRY_VERIFY(!menu->property("open").toBool());
}

// Find bar over nav bar; search/step are engine messages to page; answers on name page told to
// listen for.
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

    QVERIFY(
        view->property("messageListeners").toStringList().contains(QStringLiteral("embed:find")));
    QVERIFY(!bar->property("visible").toBool());

    QObject *menu = find(QStringLiteral("browserMenu"));
    tapBar(QStringLiteral("menu"));
    click(find(QStringLiteral("findMenuButton")));
    QVERIFY(!menu->property("open").toBool());
    QVERIFY(bar->property("active").toBool());
    QVERIFY(bar->property("visible").toBool());
    QCOMPARE(bar->property("y").toReal(), navigation->property("y").toReal());
    QCOMPARE(bar->property("height").toReal(), navigation->property("height").toReal());
    view->setProperty("chrome", false);
    QVERIFY(!navigation->property("compact").toBool());
    view->setProperty("chrome", true);
    QVERIFY(find(QStringLiteral("findPreviousButton"))->property("enabled").toBool() == false);

    QObject *field = find(QStringLiteral("findField"));
    field->setProperty("text", QStringLiteral("salama"));
    enterKey(field);
    QCOMPARE(lastMessage(view).value(QStringLiteral("name")).toString(),
             QStringLiteral("embedui:find"));
    QCOMPARE(request(view).value(QStringLiteral("text")).toString(), QStringLiteral("salama"));
    QVERIFY(!request(view).value(QStringLiteral("again")).toBool());
    QVERIFY(!request(view).value(QStringLiteral("backwards")).toBool());

    click(find(QStringLiteral("findNextButton")));
    QVERIFY(request(view).value(QStringLiteral("again")).toBool());
    QVERIFY(!request(view).value(QStringLiteral("backwards")).toBool());
    click(find(QStringLiteral("findPreviousButton")));
    QVERIFY(request(view).value(QStringLiteral("again")).toBool());
    QVERIFY(request(view).value(QStringLiteral("backwards")).toBool());
    QCOMPARE(request(view).value(QStringLiteral("text")).toString(), QStringLiteral("salama"));

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

    click(find(QStringLiteral("findCloseButton")));
    QVERIFY(!bar->property("active").toBool());
    QCOMPARE(request(view).value(QStringLiteral("text")).toString(), QString());
    const int sent = view->property("messages").toList().count();

    tapBar(QStringLiteral("menu"));
    click(find(QStringLiteral("findMenuButton")));
    QVERIFY(bar->property("active").toBool());
    m_core->tabs()->newTab(QStringLiteral("https://two.example/"));
    QVERIFY(currentWebView() != view);
    QVERIFY(!bar->property("active").toBool());
    QCOMPARE(view->property("messages").toList().count(), sent + 1);
    QCOMPARE(request(view).value(QStringLiteral("text")).toString(), QString());
}

// Reader view as in Firefox: offered when Readability says article, opened as own page in view
// history, left with back; tab, bar, history keep article address. Stub view answers scripts.
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
    // Page answers to Readability quick check and parse: JS expressions, article as string literal.
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

    answer(QStringLiteral("true"), articleLiteral);
    load();
    QCOMPARE(runs(engine->readerableScript()), 0);
    QVERIFY(!reader->property("readerable").toBool());

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
    QCOMPARE(tabs->activeUrl(), story);
    QCOMPARE(m_core->history()->count(), visits);
    // Engine calls reader doc insecure, but no connection involved: no broken-https warning.
    QObject *bar = find(QStringLiteral("navigationBar"));
    evaluate(webView, QStringLiteral("security.allGood = false"));
    QVERIFY(!bar->property("tlsBroken").toBool());
    const int checks = runs(engine->readerableScript());
    load();
    QCOMPARE(runs(engine->readerableScript()), checks);
    QCOMPARE(tabs->activeFavicon(), QStringLiteral("https://example.com/favicon.ico"));

    const QVariantMap ambience = reader->property("ambience").toMap();
    QCOMPARE(ambience.value(QStringLiteral("highlightColor")).value<QColor>(),
             evaluate(reader, QStringLiteral("Theme.highlightColor")).value<QColor>());
    QVERIFY(html.contains(QLatin1String("class=\"ambience ambience-dark sans-serif\"")));
    QCOMPARE(runs(engine->styleScript(ambience)), 0);
    m_core->readerSettings()->setColors(ReaderSettings::Sepia);
    QCOMPARE(runs(engine->styleScript(ambience)), 1);
    QVERIFY(engine->styleScript(ambience).contains(QLatin1String("'sepia sans-serif'")));
    QCOMPARE(calls(QStringLiteral("loadHtml")), 1);

    webView->setProperty("canGoBack", true);
    tapBar(QStringLiteral("menu"));
    click(button);
    QCOMPARE(calls(QStringLiteral("goBack")), 1);
    webView->setProperty("url", QUrl(story));
    QVERIFY(!reader->property("active").toBool());
    QVERIFY(!button->property("checked").toBool());
    QVERIFY(bar->property("tlsBroken").toBool());
    evaluate(webView, QStringLiteral("security.allGood = true"));
    QVERIFY(!bar->property("tlsBroken").toBool());
    QVERIFY(reader->property("readerable").toBool());
    QCOMPARE(tabs->activeUrl(), story);
    m_core->readerSettings()->setColors(ReaderSettings::Dark);
    QCOMPARE(runs(engine->styleScript(ambience)), 0);

    webView->setProperty("url", QUrl(QStringLiteral("https://example.com/linked")));
    QCOMPARE(tabs->activeUrl(), QStringLiteral("https://example.com/linked"));
    webView->setProperty("url", readerUrl);
    QVERIFY(reader->property("active").toBool());
    QCOMPARE(tabs->activeUrl(), story);
    webView->setProperty("canGoBack", false);
    evaluate(reader, QStringLiteral("toggle()"));
    QCOMPARE(webView->property("url").toUrl(), QUrl(story));
    QVERIFY(!reader->property("active").toBool());

    answer(QStringLiteral("true"), QStringLiteral("''"));
    load();
    QVERIFY(button->property("enabled").toBool());
    tapBar(QStringLiteral("menu"));
    click(button);
    QCOMPARE(calls(QStringLiteral("loadHtml")), 1);
    QVERIFY(!reader->property("readerable").toBool());
    QVERIFY(!button->property("enabled").toBool());
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
    // BrowserPage.qml item: scope has engine.
    QObject *scope = find(QStringLiteral("viewArea"));
    QVERIFY(evaluate(scope, QStringLiteral("WebEngine.observers"))
                .toStringList()
                .contains(downloads->topic()));
    const auto engineSays = [this, scope](const QString &message) {
        evaluate(scope, QStringLiteral("WebEngine.recvObserve('embed:download', %1)").arg(message));
    };
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
    // Menu exactly these: no open-folder (platform won't from Sailjail), no copy-link.
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

    const UrlCatcher files(QStringLiteral("file"));
    click(rows.first());
    QVERIFY(files.opened.isEmpty());

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
    click(part(rows.first(), "resumeDownloadMenu"));
    QCOMPARE(engineTold(), told(QStringLiteral("retryDownload"), 1));
    engineSays(QStringLiteral("{msg: 'dl-start', id: 1, displayName: 'report.pdf',"
                              " sourceUrl: 'https://files.example/report.pdf',"
                              " targetPath: '%1', mimeType: 'application/pdf', size: 2048}")
                   .arg(report));
    QCOMPARE(text(rows.first(), "downloadStatus"), QStringLiteral("819 B of 2.0 kB · 40%"));
    click(part(rows.first(), "pauseDownloadMenu"));
    QCOMPARE(engineTold(), told(QStringLiteral("cancelDownload"), 1));

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
    QVERIFY(!shows(rows.first(), "pauseDownloadMenu"));
    QVERIFY(!shows(rows.first(), "resumeDownloadMenu"));
    click(part(rows.first(), "openDownloadMenu"));
    QCOMPARE(files.opened.count(), 2);

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

    click(part(rows.first(), "removeDownloadMenu"));
    QCOMPARE(downloads->count(), 1);

    rows = findAll(QStringLiteral("downloadDelegate"));
    QCOMPARE(rows.first()->property("remorseCount").toInt(), 0);
    click(part(rows.first(), "deleteDownloadMenu"));
    QCOMPARE(rows.first()->property("remorseCount").toInt(), 1);
    QVERIFY(!QFileInfo::exists(report));
    QCOMPARE(downloads->count(), 0);

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
    click(part(findAll(QStringLiteral("downloadDelegate")).first(), "removeDownloadMenu"));
    QCOMPARE(engineTold(), told(QStringLiteral("cancelDownload"), 4));
    QCOMPARE(downloads->count(), 0);
    QCOMPARE(findAll(QStringLiteral("downloadDelegate")).count(), 0);
    QVERIFY(!find(QStringLiteral("clearFinishedDownloadsMenu"))->property("enabled").toBool());
}

// Earlier-run download refetched from source; engine's start becomes replacement row.
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

// Download status banner above bar on browsing page.
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

    engineSays(QStringLiteral("{msg: 'dl-start', id: 1, displayName: 'report.pdf',"
                              " sourceUrl: 'https://files.example/report.pdf',"
                              " targetPath: '/tmp/report.pdf', mimeType: 'application/pdf',"
                              " size: 2048}"));
    engineSays(QStringLiteral("{msg: 'dl-progress', id: 1, percent: 40}"));
    QVERIFY(banner->property("shown").toBool());
    QCOMPARE(text("bannerTitle"), QStringLiteral("report.pdf"));
    QCOMPARE(text("bannerDetail"), QStringLiteral("819 B of 2.0 kB · 40%"));
    QCOMPARE(part("downloadBannerProgress")->property("value").toReal(), 0.4);
    QTRY_COMPARE(banner->property("opacity").toReal(), 1.0);
    QObject *banners = find(QStringLiteral("barBanners"));
    QCOMPARE(banners->property("y").toReal() + banners->property("height").toReal(),
             bar->property("y").toReal());
    QCOMPARE(banner->property("y").toReal() + banner->property("height").toReal(),
             banners->property("height").toReal());
    QVERIFY(part("bannerDetail")->property("visible").toBool());
    QCOMPARE(banner->property("width").toReal(), bar->property("width").toReal());
    QCOMPARE(findObjects(banner, QStringLiteral("sheetBackground")).count(), 1);
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

    tapBar(QStringLiteral("address"));
    QVERIFY(!banner->property("shown").toBool());
    evaluate(bar, QStringLiteral("endEditing()"));
    QVERIFY(banner->property("shown").toBool());
    pullUpToTabs();
    QVERIFY(!banner->property("shown").toBool());
    pullDownToBrowser();
    QVERIFY(banner->property("shown").toBool());

    evaluate(banner, QStringLiteral("activate()"));
    QCOMPARE(currentPage()->objectName(), QStringLiteral("downloadsPage"));
    popPage();
    click(part("bannerAction"));
    QCOMPARE(currentPage()->objectName(), QStringLiteral("downloadsPage"));
    popPage();

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
    part("downloadBannerFlash")->setProperty("running", false);
    QCOMPARE(text("bannerTitle"), QStringLiteral("2 download(s) · 20%"));

    evaluate(banner, QStringLiteral("dismiss()"));
    QVERIFY(!banner->property("shown").toBool());
    engineSays(QStringLiteral("{msg: 'dl-progress', id: 2, percent: 70}"));
    QVERIFY(!banner->property("shown").toBool());
    engineSays(QStringLiteral("{msg: 'dl-start', id: 2}"));
    QVERIFY(banner->property("shown").toBool());
    QCOMPARE(text("bannerTitle"), QStringLiteral("b.iso"));

    engineSays(QStringLiteral("{msg: 'dl-done', id: 2}"));
    QVERIFY(banner->property("flashing").toBool());
    evaluate(banner, QStringLiteral("dismiss()"));
    QVERIFY(!banner->property("flashing").toBool());
    QVERIFY(!banner->property("shown").toBool());
}

// Real finger: far sideways drag dismisses, short one stays, tap opens list.
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

// ContextMenuHandler.js payload for long press on link, picture, or picture link; engine
// message field names.
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

// Long press on link/picture -> link sheet; other presses left to platform; platform menu shows
// nothing.
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

    // Platform opener builds menu from view's provider: stand-in file that loads, accepts opener's
    // props, never opens.
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

    holdOn(view, {{QStringLiteral("types"), QStringList{QStringLiteral("content-text")}}});
    QVERIFY(!menu->property("open").toBool());
    holdOn(view, heldOn(QStringLiteral("javascript:void(0)"), QStringLiteral("More")));
    QVERIFY(!menu->property("open").toBool());
    QMetaObject::invokeMethod(
        view, "recvAsyncMessage", Q_ARG(QString, QStringLiteral("embed:find")),
        Q_ARG(QVariant, QVariant(heldOn(QStringLiteral("https://a.example/")))));
    QVERIFY(!menu->property("open").toBool());

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
    QCOMPARE(findObjects(menu, QStringLiteral("sheetBackground")).count(), 1);
    QVERIFY(find(QStringLiteral("linkMenuDragHandle")) != nullptr);
    // Handle on sheet's top edge, as on nav bar (#38).
    auto *linkHandle = qobject_cast<QQuickItem *>(find(QStringLiteral("linkMenuDragHandle")));
    QCOMPARE(linkHandle->mapToScene(QPointF()).y() -
                 qobject_cast<QQuickItem *>(find(QStringLiteral("linkMenuSheet")))
                     ->mapToScene(QPointF())
                     .y(),
             -linkHandle->height() / 2);
    auto *overlay = qobject_cast<QQuickItem *>(find(QStringLiteral("linkMenuOverlay")));
    QCOMPARE(overlay->parentItem(), qobject_cast<QQuickItem *>(menu)->parentItem());
    QVERIFY(overlay->z() < menu->property("z").toReal());
    QVERIFY(overlay->property("shown").toBool());
    QTRY_VERIFY(shownIn(find(QStringLiteral("linkMenuDim"))));
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

    holdOn(view, heldOn(QStringLiteral("https://trails.example/")));
    QCOMPARE(text("linkMenuTitle"), QStringLiteral("trails.example"));
    QVERIFY(!shownIn(find(QStringLiteral("linkMenuAddress"))));
    evaluate(menu, QStringLiteral("hide()"));
    QVERIFY(!overlay->property("shown").toBool());

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

// Link actions: new tab front, background tab + banner, share, download, clipboard.
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

    const int grabs = view->property("grabCount").toInt();
    holdOn(view, heldOn(ridge, ridgeTitle));
    click(find(QStringLiteral("newTabLinkButton")));
    QVERIFY(!menu->property("open").toBool());
    QCOMPARE(tabs->count(), 2);
    QCOMPARE(tabs->activeUrl(), ridge);
    QVERIFY(view->property("grabCount").toInt() > grabs);
    tabs->activateTabById(front);

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
    for (QObject *loader : findAll(QStringLiteral("webViewLoader"))) {
        if (loader->property("tabId").toInt() == behind.id) {
            QVERIFY(!loader->property("active").toBool());
        }
    }
    click(findObjects(banner, QStringLiteral("bannerAction")).first());
    QCOMPARE(tabs->activeTabId(), behind.id);
    QCOMPARE(currentWebView()->property("url").toString(), ridge);
    QVERIFY(!banner->property("shown").toBool());
    tabs->activateTabById(front);
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

    QObject *share = find(QStringLiteral("linkShareAction"));
    holdOn(readingView, heldOn(ridge, ridgeTitle));
    click(find(QStringLiteral("shareLinkButton")));
    QVERIFY(!menu->property("open").toBool());
    QCOMPARE(share->property("triggerCount").toInt(), 1);
    QCOMPARE(share->property("mimeType").toString(), QStringLiteral("text/x-url"));
    const QVariantMap resource = share->property("resources").toList().first().toMap();
    QCOMPARE(resource.value(QStringLiteral("status")).toString(), ridge);
    QCOMPARE(resource.value(QStringLiteral("linkTitle")).toString(), ridgeTitle);

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

    QObject *notice = find(QStringLiteral("linkCopiedNotice"));
    holdOn(readingView, heldOn(ridge, ridgeTitle));
    click(find(QStringLiteral("copyLinkButton")));
    QVERIFY(!menu->property("open").toBool());
    QCOMPARE(evaluate(scope, QStringLiteral("Clipboard.text")).toString(), ridge);
    QCOMPARE(notice->property("shownCount").toInt(), 1);
    QCOMPARE(notice->property("shownText").toString(), QStringLiteral("Link copied"));
    QCOMPARE(errors.all(), QString());
}

// Other-app link: app action + Share, no tab/preview; head shows/copies mailbox or number
// without scheme.
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

    const UrlCatcher mail(QStringLiteral("mailto"));
    holdOn(view, heldOn(QStringLiteral("mailto:walks@trails.example")));
    click(app);
    QVERIFY(!menu->property("open").toBool());
    QCOMPARE(mail.opened, QList<QUrl>{QUrl(QStringLiteral("mailto:walks@trails.example"))});
    QCOMPARE(m_core->tabs()->count(), 1);

    holdOn(view, heldOn(QStringLiteral("mailto:walks@trails.example")));
    click(find(QStringLiteral("copyLinkButton")));
    QCOMPARE(evaluate(scope, QStringLiteral("Clipboard.text")).toString(),
             QStringLiteral("walks@trails.example"));
    QCOMPARE(find(QStringLiteral("linkCopiedNotice"))->property("shownText").toString(),
             QStringLiteral("Copied"));

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

// Picture: lifted above sheet, pinch-zoomable, own action row after link's, no preview.
void tst_qmlload::linkMenuForPictures()
{
    ScriptErrors errors;
    TabModel *tabs = m_core->tabs();
    QObject *menu = find(QStringLiteral("linkMenu"));
    QObject *view = currentWebView();
    QObject *scope = find(QStringLiteral("viewArea"));
    const QString photo = QStringLiteral("https://cdn.example/photos/ridge.jpg");
    const QString mapLink = QStringLiteral("https://trails.example/maps/ridge");

    holdOn(view, heldOn(mapLink, QString(), photo));
    QVERIFY(menu->property("open").toBool());
    QVERIFY(shownIn(find(QStringLiteral("linkPageRow"))));
    QVERIFY(shownIn(find(QStringLiteral("linkImageRow"))));
    QVERIFY(shownIn(
        qobject_cast<QQuickItem *>(find(QStringLiteral("linkMenuSeparator")))->parentItem()));
    QVERIFY(!shownIn(find(QStringLiteral("linkPreview"))));
    QVERIFY(!menu->property("previewShown").toBool());
    QVERIFY(view->property("active").toBool());
    QCOMPARE(find(QStringLiteral("linkMenuThumbnail"))->property("source").toString(), photo);
    QCOMPARE(find(QStringLiteral("linkMenuTitle"))->property("text").toString(),
             QStringLiteral("trails.example/maps/ridge"));
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
    picture->setProperty("scale", 2.5);
    holdOn(view, heldOn(QString(), QString(), QStringLiteral("https://cdn.example/b.jpg")));
    QCOMPARE(picture->property("scale").toReal(), 1.0);

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
    click(find(QStringLiteral("copyLinkButton")));
    QCOMPARE(evaluate(scope, QStringLiteral("Clipboard.text")).toString(),
             QStringLiteral("https://cdn.example/b.jpg"));
    QCOMPARE(find(QStringLiteral("linkCopiedNotice"))->property("shownText").toString(),
             QStringLiteral("Image link copied"));

    holdOn(view, heldOn(mapLink, QString(), photo));
    click(find(QStringLiteral("openImageButton")));
    QVERIFY(!menu->property("open").toBool());
    QCOMPARE(tabs->count(), 2);
    QCOMPARE(tabs->activeUrl(), photo);
    view = currentWebView();
    evaluate(scope, QStringLiteral("WebEngine.notifications = []"));
    holdOn(view, heldOn(mapLink, QString(), photo));
    click(find(QStringLiteral("saveImageButton")));
    const QVariantList sent = evaluate(scope, QStringLiteral("WebEngine.notifications")).toList();
    QCOMPARE(sent.count(), 1);
    const QVariantMap asked = sent.first().toMap().value(QStringLiteral("value")).toMap();
    QCOMPARE(asked.value(QStringLiteral("from")).toString(), photo);
    QCOMPARE(asked.value(QStringLiteral("to")).toString(),
             QDir(m_core->downloads()->directory()).filePath(QStringLiteral("ridge.jpg")));
    holdOn(view, heldOn(mapLink, QString(), photo));
    click(find(QStringLiteral("copyImageLinkButton")));
    QVERIFY(!menu->property("open").toBool());
    QCOMPARE(evaluate(scope, QStringLiteral("Clipboard.text")).toString(), photo);
    QCOMPARE(find(QStringLiteral("linkCopiedNotice"))->property("shownText").toString(),
             QStringLiteral("Image link copied"));
    QVERIFY(!find(QStringLiteral("linkMenuOverlay"))->property("shown").toBool());
    QCOMPARE(errors.all(), QString());
}

// Link target preview in sheet like Safari: shown for all links until hidden and vice versa.
// Engine draws one picture, so front page swapped for still while preview drawn.
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
    QVERIFY(loader->property("active").toBool());
    auto *preview = loader->property("item").value<QObject *>();
    QVERIFY(preview != nullptr);
    QCOMPARE(preview->objectName(), QStringLiteral("linkPreviewView"));
    QCOMPARE(preview->property("url").toString(), ridge);
    QVERIFY(!view->property("active").toBool());
    QVERIFY(!view->property("visible").toBool());
    QTRY_VERIFY(shownIn(still));
    QCOMPARE(still->property("source").toString(),
             QStringLiteral("image://grab/%1").arg(view->property("grabCount").toInt()));
    QCOMPARE(still->property("y").toReal(), page->property("pageCutoutInset").toReal());
    QCOMPARE(still->property("height").toReal(), view->property("height").toReal());
    QCOMPARE(still->property("width").toReal(), view->property("width").toReal());
    QCOMPARE(view->property("lastGrabSize").toSizeF(),
             QSizeF(view->property("width").toReal(), view->property("height").toReal()));

    evaluate(find(QStringLiteral("linkPreviewTap")), QStringLiteral("clicked(null)"));
    QVERIFY(!menu->property("open").toBool());
    QVERIFY(!menu->property("previewShown").toBool());
    QVERIFY(!loader->property("active").toBool());
    QCOMPARE(view->property("url").toString(), ridge);
    QCOMPARE(tabs->count(), 1);
    QVERIFY(view->property("active").toBool());
    QVERIFY(view->property("visible").toBool());
    QVERIFY(!shownIn(still));

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

    evaluate(menu, QStringLiteral("hide()"));
    QVERIFY(!menu->property("previewShown").toBool());
    QVERIFY(!loader->property("active").toBool());
    QVERIFY(menu->property("pageStill").isNull() ||
            menu->property("pageStill").value<QObject *>() == nullptr);
    QVERIFY(view->property("active").toBool());

    // Ungrabbable page not put aside: nothing to stand in.
    view->setProperty("grabFails", true);
    holdOn(view, heldOn(ridge, QStringLiteral("The ridge loop")));
    QVERIFY(!menu->property("previewShown").toBool());
    QVERIFY(view->property("active").toBool());
    evaluate(menu, QStringLiteral("hide()"));
    view->setProperty("grabFails", false);

    // Not offered while front page plays: aside would pause it.
    tabs->setMediaState(tabs->activeTabId(), TabModel::MediaPlaying);
    holdOn(view, heldOn(ridge, QStringLiteral("The ridge loop")));
    QVERIFY(!shownIn(find(QStringLiteral("linkPreview"))));
    QVERIFY(!menu->property("previewShown").toBool());
    QVERIFY(view->property("active").toBool());
    evaluate(menu, QStringLiteral("hide()"));
    tabs->setMediaState(tabs->activeTabId(), TabModel::NoMedia);
    QCOMPARE(errors.all(), QString());
}

// Page ends where bar banners begin (one or two), regains room when gone: banner never covers
// page foot.
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

// Settings column contents in order: objectName per item, section header as "#text".
// Declaration order, not y: Column only places items on polish, never happens windowless.
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

// Settings main page: entries to own pages (start page, search, reader, cover, tracking,
// notifications, history) under Browsing/Appearance/Privacy/Help; website colours and cutout
// inline last under Appearance. Entry = theme icon, name, current value; pushes its page.
void tst_qmlload::settingsPage()
{
    QObject *page = openMenuItem(QStringLiteral("settingsMenuButton"));
    QCOMPARE(page->objectName(), QStringLiteral("settingsPage"));
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
        QStringLiteral("httpsOnlySettingsEntry"),
        QStringLiteral("dohSettingsEntry"),
        QStringLiteral("trackingSettingsEntry"),
        QStringLiteral("globalPrivacyControlSwitch"),
        QStringLiteral("javascriptSwitch"),
        QStringLiteral("sitePermissionsSettingsEntry"),
        QStringLiteral("historySettingsEntry"),
        QStringLiteral("#Help"),
        QStringLiteral("tutorialSettingsEntry"),
    };
    QCOMPARE(columnOf(find(QStringLiteral("searchSettingsEntry"))), expected);

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
        {QStringLiteral("httpsOnlySettingsEntry"), QStringLiteral("icon-m-keys"),
         QStringLiteral("httpsOnlySettingsPage"), QStringLiteral("Off")},
        {QStringLiteral("dohSettingsEntry"), QStringLiteral("icon-m-browser"),
         QStringLiteral("dohSettingsPage"), QStringLiteral("Off")},
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

// sailfish-browser's Appearance/Privacy rows, its wording, and effect on page and engine.
void tst_qmlload::sailfishBrowserSettings()
{
    QObject *page = openMenuItem(QStringLiteral("settingsMenuButton"));
    QObject *colors = find(QStringLiteral("websiteColorsCombo"));
    const qreal switchMargin =
        evaluate(page, QStringLiteral("Theme.horizontalPageMargin + Theme.paddingLarge"
                                      " + Math.round((Theme.iconSizeMedium"
                                      " - Theme.itemSizeExtraSmall) / 2)"))
            .toReal();
    for (const QString &name :
         {QStringLiteral("fixedToolbarSwitch"), QStringLiteral("globalPrivacyControlSwitch"),
          QStringLiteral("javascriptSwitch")}) {
        QObject *control = find(name);
        QVERIFY2(findObjects(control, QStringLiteral("settingsSwitchIcon")).isEmpty(),
                 qPrintable(name));
        QCOMPARE(control->property("leftMargin").toReal(), switchMargin);
        QVERIFY2(!control->property("description").toString().isEmpty(), qPrintable(name));
    }

    QCOMPARE(colors->property("label").toString(), QStringLiteral("Preferred color scheme"));
    QCOMPARE(colors->property("description").toString(),
             QStringLiteral("The website style to use when available"));
    QCOMPARE(evaluate(page, QStringLiteral("names.websiteColors(Settings.WebsiteColorsAutomatic)"))
                 .toString(),
             QStringLiteral("Match ambience"));

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

    QObject *toolbar = find(QStringLiteral("fixedToolbarSwitch"));
    QVERIFY(!toolbar->property("checked").toBool());
    toolbar->setProperty("checked", true);
    QVERIFY(m_core->settings()->fixedToolbar());
    webView->setProperty("chrome", false);
    QVERIFY(!browser->property("barCompact").toBool());
    toolbar->setProperty("checked", false);
    QVERIFY(browser->property("barCompact").toBool());
    webView->setProperty("chrome", true);

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
    QCOMPARE(lastPreference(QStringLiteral("privacy.globalprivacycontrol.enabled")),
             QVariant(false));
    QCOMPARE(lastPreference(QStringLiteral("privacy.globalprivacycontrol.functionality.enabled")),
             QVariant(true));
    QCOMPARE(lastPreference(QStringLiteral("javascript.enabled")), QVariant(true));
    QObject *gpc = find(QStringLiteral("globalPrivacyControlSwitch"));
    QCOMPARE(gpc->property("text").toString(),
             QStringLiteral("Tell websites not to share & sell data"));
    QCOMPARE(gpc->property("description").toString(),
             QStringLiteral("Global Privacy Control (GPC)"));
    QVERIFY(!gpc->property("checked").toBool());
    gpc->setProperty("checked", true);
    QVERIFY(m_core->privacySettings()->globalPrivacyControl());
    QCOMPARE(lastPreference(QStringLiteral("privacy.globalprivacycontrol.enabled")),
             QVariant(true));
    QCOMPARE(lastPreference(QStringLiteral("privacy.donottrackheader.enabled")), QVariant(false));
    QObject *javascript = find(QStringLiteral("javascriptSwitch"));
    QCOMPARE(javascript->property("description").toString(),
             QStringLiteral("Allowed (recommended)"));
    javascript->setProperty("checked", false);
    QVERIFY(!m_core->privacySettings()->javascript());
    QCOMPARE(lastPreference(QStringLiteral("javascript.enabled")), QVariant(false));
    QCOMPARE(javascript->property("description").toString(),
             QStringLiteral("Blocked, some sites may not work correctly"));
}

// Start page: sections vs blank as one choice, chosen lit; section switches on by default,
// dimmed while blank; then preview of new-tab screen following them. No home page setting:
// start page is home.
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

// Search: address bar engine (all listed, chosen lit) and suggestion sources (switches, on by
// default). Entry names engine.
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

    using Flag = bool (SearchSettings::*)() const;
    const QList<QPair<QString, Flag>> sources{
        {QStringLiteral("omnibarTabsSwitch"), &SearchSettings::omnibarTabs},
        {QStringLiteral("omnibarBookmarksSwitch"), &SearchSettings::omnibarBookmarks},
        {QStringLiteral("omnibarHistorySwitch"), &SearchSettings::omnibarHistory},
        {QStringLiteral("omnibarDownloadsSwitch"), &SearchSettings::omnibarDownloads},
    };
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

// ContentLinkHandler.jsm payload for page with own search: description title/href + page url.
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

// Two valid descriptions + error page for third.
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

// Found search engines: offer heard on every view, kept once, only on right message;
// Settings > Search lists them.
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

// Tap fetches description via page XMLHttpRequest from loopback server: success -> added,
// chosen, no longer offered, announced; failure -> stays.
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

    const QList<QObject *> rows = findAll(QStringLiteral("searchEngineRow"));
    QCOMPARE(rows.count(), 4);
    QCOMPARE(choiceIn(rows.last())->property("text").toString(), QStringLiteral("Find"));
    QCOMPARE(choiceIn(rows.last())->property("description").toString(),
             QStringLiteral("Added from find.example"));
    QVERIFY(choiceIn(rows.last())->property("checked").toBool());
    QVERIFY(!choiceIn(rows.first())->property("checked").toBool());
    QVERIFY(choiceIn(rows.first())->property("description").toString().isEmpty());

    const QList<QObject *> found = findAll(QStringLiteral("foundSearchEngine"));
    click(found.first());
    QTRY_COMPARE(notice->property("shownCount").toInt(), 2);
    QCOMPARE(notice->property("shownText").toString(), QStringLiteral("Could not add Broken"));
    QCOMPARE(foundTitles(list), QStringList{QStringLiteral("Broken")});
    QCOMPARE(list->engineNames().count(), 4);
    QCOMPARE(search->engineIndex(), 3);

    const int requests = server.requests();
    click(found.first());
    click(found.first());
    QTRY_COMPARE(notice->property("shownCount").toInt(), 3);
    QCOMPARE(server.requests(), requests + 1);
    QTest::qWait(50);
    QCOMPARE(notice->property("shownCount").toInt(), 3);
}

// Added engine removed via press-and-hold menu, or all via pulley, after remorse; first
// built-in becomes current if removed one was.
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

    offerSearch(view, QStringLiteral("Third"), server.url(QStringLiteral("/third.xml")),
                QStringLiteral("https://third.example/"));
    click(findAll(QStringLiteral("foundSearchEngine")).first());
    QTRY_COMPARE(list->engineNames().last(), QStringLiteral("Third"));
    popPage();
    QCOMPARE(
        textIn(find(QStringLiteral("searchSettingsEntry")), QStringLiteral("settingsEntryValue")),
        QStringLiteral("Third"));
}

// Reader look: colour swatches painted as reader, typeface tiles in own face, set one lit;
// Firefox middle size labelled in full; entry summarises all three.
void tst_qmlload::readerSettingsPage()
{
    openMenuItem(QStringLiteral("settingsMenuButton"));
    QObject *entry = find(QStringLiteral("readerSettingsEntry"));
    click(entry);
    QCOMPARE(currentPage()->objectName(), QStringLiteral("readerSettingsPage"));

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
    QVERIFY(heading->property("y").toReal() > domain->property("y").toReal());
    QCOMPARE(fontOf(text).family(), QStringLiteral("sans-serif"));
    click(swatches.at(0));
    QCOMPARE(m_core->readerSettings()->colors(), int(ReaderSettings::Automatic));
    QCOMPARE(preview->property("color").value<QColor>(),
             Reader::backgroundOf(QStringLiteral("dark")));
    click(swatches.at(3));

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

    m_core->readerSettings()->setColors(ReaderSettings::Light);
    QCOMPARE(preview->property("color").value<QColor>(),
             Reader::backgroundOf(QStringLiteral("light")));
    QVERIFY(swatches.at(2)->property("selected").toBool());
    m_core->readerSettings()->setColors(ReaderSettings::Dark);
    QCOMPARE(domain->property("color").value<QColor>(),
             Reader::linkColorOf(QStringLiteral("dark")));
    m_core->readerSettings()->setColors(ReaderSettings::Ambience);
    QCOMPARE(fontOf(text).family(), QStringLiteral("serif"));
    QCOMPARE(fontOf(heading).family(), QStringLiteral("serif"));
}

// Last value page gave engine for pref, or null.
QVariant lastPreferenceGiven(const QVariantList &given, const QString &name)
{
    for (int i = given.count() - 1; i >= 0; --i) {
        if (given.at(i).toMap().value(QStringLiteral("key")).toString() == name) {
            return given.at(i).toMap().value(QStringLiteral("value"));
        }
    }
    return {};
}

// HTTPS-Only: Firefox Android switch wording, off default; engine told at once, HTTPS-First on
// either way; while off, line says connections may still upgrade.
void tst_qmlload::httpsOnlySettingsPage()
{
    openMenuItem(QStringLiteral("settingsMenuButton"));
    QObject *entry = find(QStringLiteral("httpsOnlySettingsEntry"));
    QCOMPARE(entry->property("text").toString(), QStringLiteral("HTTPS-Only Mode"));
    click(entry);
    QObject *page = currentPage();
    QCOMPARE(page->objectName(), QStringLiteral("httpsOnlySettingsPage"));
    QObject *pageScope = find(QStringLiteral("viewArea"));
    const auto last = [&](const char *name) {
        return lastPreferenceGiven(
            evaluate(pageScope, QStringLiteral("WebEngineSettings.preferences")).toList(),
            QLatin1String(name));
    };
    QCOMPARE(last("dom.security.https_only_mode"), QVariant(false));
    QCOMPARE(last("dom.security.https_first"), QVariant(true));

    QObject *toggle = find(QStringLiteral("httpsOnlySwitch"));
    QCOMPARE(toggle->property("text").toString(), QStringLiteral("HTTPS-Only Mode"));
    QVERIFY(toggle->property("description")
                .toString()
                .startsWith(QStringLiteral("Automatically attempts to connect to sites using "
                                           "HTTPS")));
    QVERIFY(!toggle->property("checked").toBool());
    QVERIFY(shownIn(page, "httpsFirstNote"));
    QCOMPARE(textOf(page, "httpsFirstNote"),
             QStringLiteral("Salama may still upgrade some connections"));

    toggle->setProperty("checked", true);
    QVERIFY(m_core->privacySettings()->httpsOnly());
    QCOMPARE(last("dom.security.https_only_mode"), QVariant(true));
    QCOMPARE(last("dom.security.https_first"), QVariant(true));
    QVERIFY(!shownIn(page, "httpsFirstNote"));
    popPage();
    QCOMPARE(find(QStringLiteral("httpsOnlySettingsEntry"))->property("value").toString(),
             QStringLiteral("On"));
    m_core->privacySettings()->setHttpsOnly(false);
    QCOMPARE(last("dom.security.https_only_mode"), QVariant(false));
    QCOMPARE(find(QStringLiteral("httpsOnlySettingsEntry"))->property("value").toString(),
             QStringLiteral("Off"));
}

// DoH: Firefox Android levels minus Default, Off default, wording, set one lit; level sent at
// once as GeckoView resolver mode after provider and exceptions. Provider choosable only when
// used; entry names level.
void tst_qmlload::dohSettingsPage()
{
    openMenuItem(QStringLiteral("settingsMenuButton"));
    click(find(QStringLiteral("dohSettingsEntry")));
    QObject *page = currentPage();
    QCOMPARE(page->objectName(), QStringLiteral("dohSettingsPage"));
    QObject *pageScope = find(QStringLiteral("viewArea"));
    const auto given = [&]() {
        return evaluate(pageScope, QStringLiteral("WebEngineSettings.preferences")).toList();
    };
    const auto last = [&](const char *name) {
        return lastPreferenceGiven(given(), QLatin1String(name));
    };
    QCOMPARE(last("network.trr.mode"), QVariant(5));
    QCOMPARE(last("network.trr.uri"), QVariant(DohSettings::defaultProvider()));
    QCOMPARE(last("network.trr.excluded-domains"), QVariant(QString()));
    QVERIFY(textOf(page, "dohSummary").startsWith(QStringLiteral("Domain Name System (DNS)")));

    const QList<QObject *> levels = findAll(QStringLiteral("dohProtectionChoice"));
    QCOMPARE(levels.count(), 3);
    const QStringList names{QStringLiteral("Increased Protection"),
                            QStringLiteral("Max Protection"), QStringLiteral("Off")};
    const QList<int> stored{DohSettings::ProtectionIncreased, DohSettings::ProtectionMax,
                            DohSettings::ProtectionOff};
    QStringList descriptions;
    for (int i = 0; i < levels.count(); ++i) {
        QCOMPARE(levels.at(i)->property("text").toString(), names.at(i));
        QVERIFY(!levels.at(i)->property("automaticCheck").toBool());
        QCOMPARE(levels.at(i)->property("checked").toBool(),
                 stored.at(i) == DohSettings::ProtectionOff);
        const QString description = levels.at(i)->property("description").toString();
        QVERIFY2(!description.isEmpty(), qPrintable(names.at(i)));
        QVERIFY(!descriptions.contains(description));
        descriptions.append(description);
    }
    QCOMPARE(descriptions.last(), QStringLiteral("Use your default DNS resolver"));
    QVERIFY(!shownIn(page, "dohProviderCombo"));

    const int before = given().count();
    click(levels.at(1));
    QCOMPARE(m_core->dohSettings()->protection(), int(DohSettings::ProtectionMax));
    QVERIFY(levels.at(1)->property("checked").toBool());
    QVERIFY(!levels.at(2)->property("checked").toBool());
    const QVariantList after = given();
    QCOMPARE(after.count(), before + 3);
    QCOMPARE(after.at(before).toMap().value(QStringLiteral("key")).toString(),
             QStringLiteral("network.trr.uri"));
    QCOMPARE(after.last().toMap().value(QStringLiteral("key")).toString(),
             QStringLiteral("network.trr.mode"));
    QCOMPARE(last("network.trr.mode"), QVariant(3));
    click(levels.at(0));
    QCOMPARE(last("network.trr.mode"), QVariant(2));

    QObject *provider = find(QStringLiteral("dohProviderCombo"));
    QVERIFY(shownIn(page, "dohProviderCombo"));
    QCOMPARE(provider->property("label").toString(), QStringLiteral("Choose provider"));
    QCOMPARE(provider->property("currentIndex").toInt(), 0);
    QCOMPARE(provider->property("description").toString(), QString());
    const QList<QObject *> choices = findAll(QStringLiteral("dohProviderChoice"));
    QCOMPARE(choices.count(), 3);
    QCOMPARE(choices.at(0)->property("text").toString(), QStringLiteral("Cloudflare (default)"));
    QCOMPARE(choices.at(1)->property("text").toString(), QStringLiteral("NextDNS"));
    QCOMPARE(choices.at(2)->property("text").toString(), QStringLiteral("Custom"));
    click(choices.at(1));
    QCOMPARE(m_core->dohSettings()->provider(), QStringLiteral("https://firefox.dns.nextdns.io/"));
    QCOMPARE(provider->property("currentIndex").toInt(), 1);
    QCOMPARE(last("network.trr.uri"), QVariant(QStringLiteral("https://firefox.dns.nextdns.io/")));
    click(choices.at(0));
    QCOMPARE(m_core->dohSettings()->provider(), DohSettings::defaultProvider());

    click(levels.at(2));
    QCOMPARE(last("network.trr.mode"), QVariant(5));
    QVERIFY(!shownIn(page, "dohProviderCombo"));

    QObject *exceptions = find(QStringLiteral("dohExceptionsEntry"));
    QCOMPARE(exceptions->property("text").toString(), QStringLiteral("Exceptions"));
    QCOMPARE(exceptions->property("value").toString(), QStringLiteral("None"));
    QVERIFY(!findObjects(exceptions, QStringLiteral("settingsEntryIcon"))
                 .first()
                 ->property("visible")
                 .toBool());
    m_core->dohSettings()->addException(QStringLiteral("router.local"));
    QCOMPARE(exceptions->property("value").toString(), QStringLiteral("1 site(s)"));
    QCOMPARE(last("network.trr.excluded-domains"), QVariant(QStringLiteral("router.local")));
    m_core->dohSettings()->removeAllExceptions();
    click(exceptions);
    QCOMPARE(currentPage()->objectName(), QStringLiteral("dohExceptionsPage"));
    popPage();

    m_core->dohSettings()->setProtection(DohSettings::ProtectionIncreased);
    popPage();
    QCOMPARE(find(QStringLiteral("dohSettingsEntry"))->property("value").toString(),
             QStringLiteral("Increased Protection"));
}

// Custom provider: Firefox Android dialog + its two errors; invalid can't be added; choice
// reverts to set one when dialog left either way.
void tst_qmlload::dohProviderDialog()
{
    m_core->dohSettings()->setProtection(DohSettings::ProtectionIncreased);
    openMenuItem(QStringLiteral("settingsMenuButton"));
    click(find(QStringLiteral("dohSettingsEntry")));
    QObject *page = currentPage();
    QObject *provider = find(QStringLiteral("dohProviderCombo"));
    const QList<QObject *> choices = findAll(QStringLiteral("dohProviderChoice"));

    // Silica combo takes tapped item as choice before dialog opens.
    provider->setProperty("currentIndex", 2);
    click(choices.at(2));
    QObject *dialog = currentPage();
    QCOMPARE(dialog->objectName(), QStringLiteral("dohProviderDialog"));
    QObject *address = find(QStringLiteral("dohProviderAddress"));
    QCOMPARE(address->property("text").toString(), QStringLiteral("https://"));
    QCOMPARE(address->property("label").toString(), QStringLiteral("Provider"));
    QVERIFY(!address->property("errorHighlight").toBool());
    QVERIFY(!dialog->property("canAccept").toBool());
    address->setProperty("text", QStringLiteral("http://dns.example.org/"));
    QVERIFY(address->property("errorHighlight").toBool());
    QCOMPARE(address->property("label").toString(),
             QStringLiteral("URL must start with “https://”"));
    QVERIFY(!dialog->property("canAccept").toBool());
    address->setProperty("text", QStringLiteral("https:///dns-query"));
    QCOMPARE(address->property("label").toString(), QStringLiteral("Invalid URL"));
    QVERIFY(!dialog->property("canAccept").toBool());
    QMetaObject::invokeMethod(dialog, "reject");
    popPage();
    QCOMPARE(currentPage(), page);
    QCOMPARE(m_core->dohSettings()->provider(), DohSettings::defaultProvider());
    QCOMPARE(provider->property("currentIndex").toInt(), 0);

    provider->setProperty("currentIndex", 2);
    click(choices.at(2));
    dialog = currentPage();
    address = find(QStringLiteral("dohProviderAddress"));
    address->setProperty("text", QStringLiteral("https://dns.example.org/dns-query"));
    QVERIFY(!address->property("errorHighlight").toBool());
    QVERIFY(dialog->property("canAccept").toBool());
    QMetaObject::invokeMethod(dialog, "accept");
    popPage();
    QCOMPARE(m_core->dohSettings()->provider(),
             QStringLiteral("https://dns.example.org/dns-query"));
    QVERIFY(m_core->dohSettings()->customProvider());
    QCOMPARE(provider->property("currentIndex").toInt(), 2);
    QCOMPARE(provider->property("description").toString(),
             QStringLiteral("https://dns.example.org/dns-query"));
    QCOMPARE(lastPreferenceGiven(evaluate(find(QStringLiteral("viewArea")),
                                          QStringLiteral("WebEngineSettings.preferences"))
                                     .toList(),
                                 QStringLiteral("network.trr.uri")),
             QVariant(QStringLiteral("https://dns.example.org/dns-query")));
    click(choices.at(2));
    QCOMPARE(find(QStringLiteral("dohProviderAddress"))->property("text").toString(),
             QStringLiteral("https://dns.example.org/dns-query"));
    QMetaObject::invokeMethod(currentPage(), "reject");
    popPage();
    click(choices.at(0));
    QCOMPARE(provider->property("currentIndex").toInt(), 0);
    QCOMPARE(provider->property("description").toString(), QString());
}

// Exceptions: domains with remove menu; pulley adds (Firefox Android dialog) or removes all
// after remorse.
void tst_qmlload::dohExceptionsPage()
{
    DohSettings *doh = m_core->dohSettings();
    openMenuItem(QStringLiteral("settingsMenuButton"));
    click(find(QStringLiteral("dohSettingsEntry")));
    click(find(QStringLiteral("dohExceptionsEntry")));
    QObject *page = currentPage();
    QCOMPARE(page->objectName(), QStringLiteral("dohExceptionsPage"));
    QCOMPARE(textOf(page, "dohExceptionsSummary"),
             QStringLiteral("Salama won’t use secure DNS on these sites and their subdomains."));
    QVERIFY(findAll(QStringLiteral("dohException")).isEmpty());
    QVERIFY(!shownIn(page, "removeAllDohExceptionsMenuItem"));
    QCOMPARE(textOf(page, "addDohExceptionMenuItem"), QStringLiteral("Add site"));

    click(find(QStringLiteral("addDohExceptionMenuItem")));
    QObject *dialog = currentPage();
    QCOMPARE(dialog->objectName(), QStringLiteral("dohExceptionDialog"));
    QObject *site = find(QStringLiteral("dohExceptionSite"));
    QCOMPARE(site->property("placeholderText").toString(), QStringLiteral("example.com"));
    QCOMPARE(site->property("label").toString(), QStringLiteral("Site"));
    QVERIFY(!dialog->property("canAccept").toBool());
    site->setProperty("text", QStringLiteral("not a domain"));
    QVERIFY(site->property("errorHighlight").toBool());
    QCOMPARE(site->property("label").toString(), QStringLiteral("Must be a valid domain"));
    QVERIFY(!dialog->property("canAccept").toBool());
    site->setProperty("text", QStringLiteral("https://Intranet.Example.com/wiki"));
    QVERIFY(!site->property("errorHighlight").toBool());
    QCOMPARE(dialog->property("domain").toString(), QStringLiteral("intranet.example.com"));
    QMetaObject::invokeMethod(dialog, "accept");
    popPage();
    QCOMPARE(doh->exceptions(), QStringList{QStringLiteral("intranet.example.com")});
    doh->addException(QStringLiteral("router.local"));

    QList<QObject *> rows = byRow(findAll(QStringLiteral("dohException")));
    QCOMPARE(rows.count(), 2);
    QCOMPARE(textOf(rows.at(0), "dohExceptionDomain"), QStringLiteral("intranet.example.com"));
    QCOMPARE(textOf(rows.at(1), "dohExceptionDomain"), QStringLiteral("router.local"));
    QVERIFY(shownIn(page, "removeAllDohExceptionsMenuItem"));
    QCOMPARE(textOf(rows.at(0), "dohExceptionRemove"), QStringLiteral("Remove"));
    click(findObjects(rows.at(0), QStringLiteral("dohExceptionRemove")).first());
    QCOMPARE(doh->exceptions(), QStringList{QStringLiteral("router.local")});
    QCOMPARE(findAll(QStringLiteral("dohException")).count(), 1);

    doh->addException(QStringLiteral("a.example"));
    const int remorses = evaluate(page, QStringLiteral("Remorse.popupCount")).toInt();
    click(find(QStringLiteral("removeAllDohExceptionsMenuItem")));
    QCOMPARE(evaluate(page, QStringLiteral("Remorse.popupCount")).toInt(), remorses + 1);
    QCOMPARE(evaluate(page, QStringLiteral("Remorse.popupText")).toString(),
             QStringLiteral("Removing exceptions"));
    QVERIFY(doh->exceptions().isEmpty());
    QVERIFY(findAll(QStringLiteral("dohException")).isEmpty());
}

// Tracking protection: three levels shown together, each described, set one lit; change reaches
// engine at once. Entry names level.
void tst_qmlload::trackingSettingsPage()
{
    openMenuItem(QStringLiteral("settingsMenuButton"));
    QObject *entry = find(QStringLiteral("trackingSettingsEntry"));
    click(entry);
    QObject *page = currentPage();
    QCOMPARE(page->objectName(), QStringLiteral("trackingSettingsPage"));

    // Browsing page writes prefs, so read via BrowserPage.qml item.
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
    click(levels.at(PrivacySettings::TrackingProtectionOff));
    QCOMPARE(evaluate(pageScope, QStringLiteral("WebEngineSettings.preferences.length")).toInt(),
             given + 2 * strict.count());

    const QString choice = QStringLiteral("trackingProtectionChoice");
    QCOMPARE(columnOf(levels.first()),
             (QStringList{choice, choice, choice, QStringLiteral("trackingProtectionLimits")}));
    QVERIFY(find(QStringLiteral("trackingProtectionLimits"))
                ->property("text")
                .toString()
                .contains(QStringLiteral("some trackers may still get through")));
}

// History: keep-pages and clear-on-close switches; kept data counted; clear-data button asks
// first. Entry says whether/how long kept.
void tst_qmlload::historySettingsPage()
{
    PrivacySettings *settings = m_core->privacySettings();
    TabModel *tabs = m_core->tabs();
    openMenuItem(QStringLiteral("settingsMenuButton"));
    // Settings' startup engine asks (settingsPage()) not this page's.
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
    QVERIFY(remember->property("description").toString().isEmpty());
    QCOMPARE(onClose->property("description").toString(),
             QStringLiteral("With it, the list of downloads and the recently closed tabs"));

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

    QVERIFY(!onClose->property("checked").toBool());
    m_core->clearOnClose();
    QCOMPARE(m_core->history()->count(), visits);
    onClose->setProperty("checked", true);
    QVERIFY(settings->clearHistoryOnClose());
    m_core->clearOnClose();
    QCOMPARE(m_core->history()->count(), 0);
    onClose->setProperty("checked", false);

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

// Clear data dialog: time range + kinds (all on except open tabs, Clear dimmed when none on);
// accepted kinds cleared under one remorse on history page, each as its old button did.
void tst_qmlload::clearDataDialog()
{
    TabModel *tabs = m_core->tabs();
    tabs->newTab(QStringLiteral("https://two.example/"));
    QVERIFY(m_core->history()->count() > 0);
    openMenuItem(QStringLiteral("settingsMenuButton"));
    // Settings' startup engine asks (settingsPage()) not this page's.
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
    // Reopen with switches in order above, accept. Stub stack leaves popping accepted dialog to
    // caller.
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

    const auto said = [this](const QString &name) {
        return find(name)->property("description").toString();
    };
    QCOMPARE(said(QStringLiteral("clearTabsSwitch")), QStringLiteral("2 tab(s), in every group"));
    QCOMPARE(said(QStringLiteral("clearHistorySwitch")), QStringLiteral("2 page(s)"));
    QCOMPARE(said(QStringLiteral("clearSiteDataSwitch")),
             QStringLiteral("Signs you out of most sites"));
    QVERIFY(said(QStringLiteral("clearCacheSwitch")).isEmpty());
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
    popPage();
    QCOMPARE(currentPage(), privacy);
    QCOMPARE(remorses(), 0);
    QCOMPARE(tabs->count(), 2);
    QVERIFY(m_core->history()->count() > 0);
    QCOMPARE(sent(), 0);

    clear({false, true, false, false});
    QCOMPARE(remorses(), 1);
    QCOMPARE(evaluate(privacy, QStringLiteral("Remorse.popupItem")).value<QObject *>(), privacy);
    QCOMPARE(evaluate(privacy, QStringLiteral("Remorse.popupText")).toString(),
             QStringLiteral("Clearing browsing data"));
    QCOMPARE(currentPage(), privacy);
    QCOMPARE(m_core->history()->count(), 0);
    QCOMPARE(tabs->count(), 2);
    QCOMPARE(sent(), 0);

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

    clear({false, false, false, true});
    QCOMPARE(remorses(), 3);
    QCOMPARE(sent(), 2);
    QCOMPARE(notification(1, QStringLiteral("topic")), QStringLiteral("clear-private-data"));
    QCOMPARE(notification(1, QStringLiteral("value")), QStringLiteral("cache"));
    QCOMPARE(m_core->history()->count(), visits);
    QCOMPARE(tabs->count(), 3);

    clear({true, false, false, false});
    QCOMPARE(remorses(), 4);
    QCOMPARE(tabs->count(), 1);
    QVERIFY(tabs->activeUrl().isEmpty());
    QVERIFY(m_core->history()->count() >= visits);
    QCOMPARE(sent(), 2);
    QCOMPARE(currentPage(), privacy);

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

    // Tabs cleared first so history cleared after doesn't keep replacement page.
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

// Cover quick action: row per action with glyph, under cover preview showing it. Entry names
// action in choice wording.
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

QObject *coverPart(QObject *coverItem, const QString &name)
{
    return coverItem->findChild<QObject *>(name);
}

bool shown(QObject *coverItem, const QString &name)
{
    return coverPart(coverItem, name)->property("visible").toBool();
}

// Visible cover view: where-you-were, downloads, media, or none (halftone only).
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

// Idle cover: front tab site + title over faint halftone, bolt centred. Quick action = search
// by default.
void tst_qmlload::cover()
{
    auto *coverItem = m_window->property("coverItem").value<QObject *>();
    QVERIFY(coverItem != nullptr);
    // Home screen size, so layout measurable.
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
    QCOMPARE(text(QStringLiteral("coverPlaceTitle")), QStringLiteral("qwant.com"));
    QVERIFY(coverPart(coverItem, QStringLiteral("coverPlaceCount")) == nullptr);
    QVERIFY(coverPart(coverItem, QStringLiteral("coverPlaceGroup")) == nullptr);
    auto *place = qobject_cast<QQuickItem *>(coverPart(coverItem, QStringLiteral("coverPlace")));
    auto *title =
        qobject_cast<QQuickItem *>(coverPart(coverItem, QStringLiteral("coverPlaceTitle")));
    QCOMPARE(title->y() + title->height(), place->height());
    QCOMPARE(text(QStringLiteral("coverPlaceLetter")), QStringLiteral("Q"));
    QVERIFY(shown(coverItem, QStringLiteral("coverPlaceLetter")));

    tabs->updateTitle(front, QStringLiteral("Catatumbo lightning"));
    QCOMPARE(text(QStringLiteral("coverPlaceTitle")), QStringLiteral("Catatumbo lightning"));
    const int second = tabs->newTab(QStringLiteral("https://yle.fi/uutiset"));
    QCOMPARE(text(QStringLiteral("coverPlaceHost")), QStringLiteral("yle.fi"));
    tabs->closeTabById(second);

    // Halftone taller than cover, cropped equally top/bottom so bolt stays centred.
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

// Nothing to show (no tab, or front on start page): halftone only, quick action stays.
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

// Downloads progress in 5% steps; takes precedence over media.
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
    QSignalSpy redrawn(ring, SIGNAL(valueChanged()));
    progress(1, 44);
    QCOMPARE(redrawn.count(), 0);
    progress(1, 45);
    QCOMPARE(redrawn.count(), 1);
    QCOMPARE(percent(), QStringLiteral("45"));

    start(2, QStringLiteral("iso.pdf"));
    QCOMPARE(
        coverPart(coverItem, QStringLiteral("coverDownloadsCount"))->property("text").toString(),
        QStringLiteral("2 file(s)"));
    m_core->tabs()->setMediaState(m_core->tabs()->activeTabId(), TabModel::MediaPlaying);
    QCOMPARE(coverView(coverItem), QStringLiteral("coverDownloads"));
    QVERIFY(
        coverPart(coverItem, QStringLiteral("mediaCoverActions"))->property("enabled").toBool());

    observeDownload(core, {{QStringLiteral("msg"), QStringLiteral("dl-done")},
                           {QStringLiteral("id"), 1},
                           {QStringLiteral("targetPath"), QStringLiteral("/tmp/map.pdf")}});
    observeDownload(
        core, {{QStringLiteral("msg"), QStringLiteral("dl-fail")}, {QStringLiteral("id"), 2}});
    QCOMPARE(coverView(coverItem), QStringLiteral("coverMedia"));
    m_core->tabs()->setMediaState(m_core->tabs()->activeTabId(), TabModel::NoMedia);
    QCOMPARE(coverView(coverItem), QStringLiteral("coverPlace"));
}

// Front tab playing: with artwork, artwork + page's caption under; without, large site over
// halftone. Muted: reads paused, artwork dims.
void tst_qmlload::coverShowsWhatPlays()
{
    auto *coverItem = m_window->property("coverItem").value<QObject *>();
    QVERIFY(coverItem != nullptr);
    // Home screen size, so layout measurable.
    coverItem->setProperty("width", 234);
    coverItem->setProperty("height", 374);
    TabModel *tabs = m_core->tabs();
    const int front = tabs->activeTabId();
    tabs->updateTitle(front, QStringLiteral("Yle Areena"));
    const auto text = [coverItem](const QString &name) {
        return coverPart(coverItem, name)->property("text").toString();
    };

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

    TabModel::MediaMetadata said{QStringLiteral("Symphony No. 5"), QStringLiteral("Beethoven"),
                                 QString()};
    tabs->setMediaMetadata(front, said);
    QCOMPARE(text(QStringLiteral("coverMediaPlainTitle")), QStringLiteral("Symphony No. 5"));
    QCOMPARE(text(QStringLiteral("coverMediaPlainArtist")), QStringLiteral("Beethoven"));

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

    tabs->setMuted(front, true);
    QCOMPARE(text(QStringLiteral("coverMediaState")), QStringLiteral("Paused · qwant.com"));
    QVERIFY(frame->property("opacity").toReal() < 1.0);
    QVERIFY(coverPart(coverItem, QStringLiteral("muteCoverAction"))
                ->property("iconSource")
                .toUrl()
                .toString()
                .contains(QLatin1String("speaker-mute")));

    tabs->setMuted(front, false);
    tabs->setMediaState(front, TabModel::NoMedia);
    QCOMPARE(coverView(coverItem), QStringLiteral("coverPlace"));
}

namespace {

// Cover action picture is named file, for stub small icon size + dark ambience, readable by
// home screen.
bool drawnFrom(const QUrl &picture, const QString &file)
{
    return picture.toString().endsWith(QStringLiteral("art/cover/") + file) &&
           QFile::exists(picture.toLocalFile());
}

// Cover preview action glyphs, left to right.
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

// Quick action from any app state: top page popped, browsing overlays (sheet, address edit,
// grid) dismissed, then action opens target.
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

    openMenuItem(QStringLiteral("settingsMenuButton"));
    tap();
    QCOMPARE(currentPage(), page);
    QVERIFY(bar->property("editing").toBool());
    QVERIFY(bar->property("forNewTab").toBool());
    QCOMPARE(tabs->count(), open);

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

    m_core->bookmarks()->removeByUrl(wiki);
    tap();
    QCOMPARE(currentPage()->objectName(), QStringLiteral("bookmarksPage"));
    QCOMPARE(tabs->count(), open + 1);
    QCOMPARE(settings->quickAction(), int(CoverSettings::QuickActionBookmark));
    QCOMPARE(settings->quickActionBookmark(), wikiId);

    QCOMPARE(m_window->property("activateCount").toInt(), taps);
    QVERIFY2(errors.all().isEmpty(), qPrintable(errors.all()));
}

// Home screen actions: quick action alone (target's glyph) while nothing plays; plus front tab
// mute while playing/muted; mute alone without quick action; nothing if neither. Home screen
// draws only enabled list.
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
    settings->setQuickActionIcon(QStringLiteral("music"));
    QVERIFY(drawnFrom(picture(quick), QStringLiteral("music-32-white.png")));

    tabs->setMuted(tabs->activeTabId(), true);
    QCOMPARE(enabled(), QStringList{QStringLiteral("mediaCoverActions")});
    QCOMPARE(picture(QStringLiteral("mediaQuickCoverAction")), picture(quick));
    QVERIFY(drawnFrom(picture(QStringLiteral("muteCoverAction")),
                      QStringLiteral("speaker-mute-32-white.png")));

    settings->setQuickAction(CoverSettings::QuickActionNone);
    QCOMPARE(enabled(), QStringList{QStringLiteral("muteCoverActions")});
    QVERIFY(drawnFrom(picture(QStringLiteral("loneMuteCoverAction")),
                      QStringLiteral("speaker-mute-32-white.png")));
    QMetaObject::invokeMethod(
        coverItem->findChild<QObject *>(QStringLiteral("loneMuteCoverAction")), "triggered");
    QVERIFY(!tabs->isMuted(tabs->activeTabId()));

    QCOMPARE(enabled(), QStringList());
    settings->setQuickAction(CoverSettings::QuickActionSearch);
    QCOMPARE(enabled(), QStringList{QStringLiteral("quickCoverActions")});
    QVERIFY2(errors.all().isEmpty(), qPrintable(errors.all()));
}

// Quick action choice: row writes it, lit when set, wears cover glyph, previews draw it.
// "Open a bookmark" asks which (back out = no change); row then names bookmark, glyph choices
// listed under rows, drawn from cover's files.
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
        QCOMPARE(previewGlyphs(preview),
                 choice.glyph.isEmpty() ? QStringList() : QStringList{choice.glyph});
        QVERIFY(!icons->property("visible").toBool());
    }

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

    click(find(QStringLiteral("quickAction-history")));
    QVERIFY(detail->property("visible").toBool());
    QCOMPARE(detail->property("text").toString(), QStringLiteral("Sea"));
    QVERIFY(!icons->property("visible").toBool());

    bookmarks->clear();
    click(bookmarkItem);
    placeholder = find(QStringLiteral("bookmarkPickerPlaceholder"));
    QVERIFY(placeholder->property("enabled").toBool());
    QCOMPARE(placeholder->property("text").toString(), QStringLiteral("No bookmarks"));
    popPage();
    QCOMPARE(settings->quickActionBookmark(), sea);
    QVERIFY2(errors.all().isEmpty(), qPrintable(errors.all()));
}

// Action bookmark kept by id, refound by url: rename -> new name; removed + re-added via menu
// (new id) -> still it; gone -> row says so, cover action opens bookmarks.
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

    bookmarks->edit(bookmarks->count() - 1, url, QStringLiteral("Bee hive"));
    QCOMPARE(value->property("text").toString(), QStringLiteral("Bee hive"));
    QCOMPARE(settings->quickActionBookmarkTitle(), QStringLiteral("Bee hive"));
    backToBrowser();

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

    QObject *page = find(QStringLiteral("browserPage"));
    QMetaObject::invokeMethod(page, "applicationStateChanged",
                              Q_ARG(QVariant, Qt::ApplicationActive));
    QCOMPARE(webView->property("grabCount").toInt(), grabs);

    QMetaObject::invokeMethod(page, "applicationStateChanged",
                              Q_ARG(QVariant, Qt::ApplicationInactive));
    QCOMPARE(webView->property("grabCount").toInt(), grabs + 1);
    QTRY_VERIFY(thumbnail() != onLoad);
    QVERIFY(!thumbnail().isEmpty());
}

// Shortly after going out of sight, loaded pages sleep unless one makes sound; each wakes when its
// view next shown. Timing is PageActivity's (tst_pageactivity); this is browsing page's response.
void tst_qmlload::pagesSleepOutOfSight()
{
    const int firstTab = m_core->tabs()->activeTabId();
    m_core->tabs()->newTab(QStringLiteral("https://two.example/"));
    QList<QObject *> views = findAll(QStringLiteral("webView"));
    QCOMPARE(views.count(), 2);
    QObject *front = currentWebView();
    QObject *behind = views.at(0) == front ? views.at(1) : views.at(0);
    QObject *page = find(QStringLiteral("browserPage"));
    // BrowserPage.qml item: its scope has engine; page item's is root file's.
    QObject *scope = find(QStringLiteral("viewArea"));
    Salama::PageActivity *activity = m_core->pageActivity();
    const auto calls = [](QObject *view, const char *name) {
        return view->property("calls").toStringList().count(QLatin1String(name));
    };
    const auto setState = [page](Qt::ApplicationState state) {
        QMetaObject::invokeMethod(page, "applicationStateChanged", Q_ARG(QVariant, state));
    };

    const QStringList topics =
        activity->topics() +
        QStringList{m_core->downloads()->topic(), m_core->notificationPermissions()->topic()};
    QCOMPARE(evaluate(scope, QStringLiteral("WebEngine.observers")).toStringList(), topics);

    // Until sleep, front stays active: inactive view hides document, hidden document pauses media.
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

    front->setProperty("loading", true);
    front->setProperty("loading", false);
    QCOMPARE(calls(front, "suspendView"), 3);

    setState(Qt::ApplicationActive);
    QVERIFY(!activity->asleep());
    QVERIFY(front->property("active").toBool());
    QCOMPARE(calls(front, "resumeView"), 1);
    QVERIFY(!front->property("suspended").toBool());
    QCOMPARE(calls(behind, "resumeView"), 0);
    QVERIFY(behind->property("suspended").toBool());
    // New document in sleeping behind view left awake: suspending stops shared window all views
    // draw into.
    behind->setProperty("loading", true);
    behind->setProperty("loading", false);
    QCOMPARE(calls(behind, "suspendView"), 1);
    m_core->tabs()->activateTabById(firstTab);
    QVERIFY(behind->property("active").toBool());
    QCOMPARE(calls(behind, "resumeView"), 1);
    QVERIFY(!behind->property("suspended").toBool());
    front->setProperty("loading", true);
    front->setProperty("loading", false);
    QCOMPARE(calls(front, "suspendView"), 3);
    m_core->tabs()->activateTabById(
        m_core->tabs()->data(m_core->tabs()->index(1, 0), roleId(TabModel::Role::TabId)).toInt());
    QCOMPARE(currentWebView(), front);

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

// Grid cell = picture only; active tab marked by surrounding frame only.
void tst_qmlload::gridCellsArePicturesAlone()
{
    m_core->tabs()->newTab(QStringLiteral("https://two.example/"));
    pullUpToTabs();
    QList<QObject *> previews = findAll(QStringLiteral("tabPreview"));
    QCOMPARE(previews.count(), 2);
    QObject *cell = previews.at(1);

    // Rounded box: clipping always rectangular, so mask cuts corners.
    QObject *shot = findObjects(cell, QStringLiteral("tabPreviewShot")).first();
    QVERIFY(shot->property("radius").toReal() > 0);
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
    QVERIFY(gap + stroke < cell->property("inset").toReal());
    QVERIFY(!findObjects(previews.at(0), QStringLiteral("tabPreviewFrame"))
                 .first()
                 ->property("visible")
                 .toBool());
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

// Grid head/foot tint opaque like nav bar: no cell shows through.
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
    // Real window: bar row laid out on draw, grid controls tapped by finger.
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

    QVERIFY(!mute->property("visible").toBool());
    QVERIFY(quickActions->property("enabled").toBool());
    QVERIFY(!mediaActions->property("enabled").toBool());
    // Anchors snap centred item to whole pixel: tolerance 0.5.
    const auto centred = [bar, centreX](QObject *object) {
        return qAbs(centreX(object) - bar->width() / 2) <= 0.5;
    };
    QTRY_VERIFY(centred(host));

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
    QCOMPARE(mute->property("color").value<QColor>(), QColor(QStringLiteral("#aaccff")));
    QCOMPARE(mute->property("width").toReal(), qreal(48));
    QVERIFY(!quickActions->property("enabled").toBool());
    QVERIFY(mediaActions->property("enabled").toBool());
    QVERIFY(coverIcon().toString().endsWith(QLatin1String("art/cover/speaker-on-32-white.png")));
    QVERIFY2(QFile::exists(coverIcon().toLocalFile()), qPrintable(coverIcon().toString()));

    QTRY_COMPARE(regionOf(mute), QStringLiteral("mute"));
    QVERIFY(centred(host));
    QCOMPARE(regionOf(host), QStringLiteral("address"));
    QCOMPARE(evaluate(bar, QStringLiteral("regionAt(0)")).toString(), QStringLiteral("back"));
    QCOMPARE(evaluate(bar, QStringLiteral("regionAt(addressLeft)")).toString(),
             QStringLiteral("mute"));
    auto *muteItem = qobject_cast<QQuickItem *>(mute);
    QVERIFY(muteItem->mapToItem(bar, QPointF(muteItem->width(), 0)).x() <
            qobject_cast<QQuickItem *>(host)->mapToItem(bar, QPointF(0, 0)).x());

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

    // Paused while view still front, before page told hidden.
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

    // Behind page claiming playing is engine-held, shown paused.
    behind->setProperty("scriptResult", QStringLiteral("playing"));
    engineSays("play");
    QTRY_COMPARE(asked(behind).last(), QStringLiteral("pause"));
    QCOMPARE(tabs->shownMediaState(first), TabModel::MediaPaused);
    QCOMPARE(tabs->mediaState(second), TabModel::MediaPlaying);

    front->setProperty("scriptResult", QStringLiteral("paused"));
    QMetaObject::invokeMethod(coverMute, "triggered");
    QVERIFY(tabs->isMuted(second));
    QCOMPARE(asked(front).last(), QStringLiteral("pause"));

    front->setProperty("loading", true);
    QCOMPARE(tabs->mediaState(second), TabModel::NoMedia);
    QVERIFY(mute->property("visible").toBool());
    QVERIFY(mediaActions->property("enabled").toBool());
    front->setProperty("scriptResult", QString());
    front->setProperty("loading", false);

    QVERIFY2(errors.all().isEmpty(), qPrintable(errors.all()));
}

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
    behind->setProperty("scriptResult", QStringLiteral("playing"));
    media->answer(first, Salama::PageMedia::Command::Query, QStringLiteral("playing"));
    tabs->setMuted(second, true);

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
    QVERIFY(secondAction->property("visible").toBool());
    QVERIFY(icon(findObjects(cells.at(1), QStringLiteral("previewMuteIcon")).first())
                .endsWith(QLatin1String("icon-m-speaker-mute")));
    QObject *firstPicture = findObjects(cells.at(0), QStringLiteral("tabPreviewPicture")).first();
    QVERIFY(evaluate(firstPicture, QStringLiteral("layer.enabled")).toBool());
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

    behind->setProperty("scriptResult", QString());
    tabs->setMuted(first, false);
    media->forget(first);
    QVERIFY(!firstAction->property("visible").toBool());
    QVERIFY(!evaluate(firstPicture, QStringLiteral("layer.enabled")).toBool());
    QVERIFY2(errors.all().isEmpty(), qPrintable(errors.all()));
}

namespace {

// Frame script payload to view: page message + origin + Gecko-held permission.
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

// Replies run in view, as "type:id" or "permission:id:state".
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

// Page notifications: frame script + Notification shim per view; shown -> platform
// notification; tap, swipe, page gone.
void tst_qmlload::webNotifications()
{
    WebNotifications *notifications = m_core->webNotifications();
    QObject *view = currentWebView();
    const int tab = m_core->tabs()->activeTabId();

    QVERIFY(
        view->property("messageListeners").toStringList().contains(notifications->messageName()));
    QCOMPARE(view->property("frameScripts").toStringList(),
             QStringList{notifications->relayScriptUrl()});
    // Notification shim installed on document arrival and again after load (engine runs no script
    // before view made).
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

    relay(view, message(QStringLiteral("show"), 2, QStringLiteral("Again"), QStringLiteral("room")),
          QLatin1String(ChatSite), QStringLiteral("granted"));
    QCOMPARE(findAll(QStringLiteral("webNotification")), QList<QObject *>{hello.data()});
    QCOMPARE(hello->property("summary").toString(), QStringLiteral("Again"));
    QCOMPARE(hello->property("publishCount").toInt(), 2);

    m_core->tabs()->newTab(QStringLiteral("https://two.example/"));
    QVERIFY(currentWebView() != view);
    const int activations = m_window->property("activateCount").toInt();
    QMetaObject::invokeMethod(hello, "clicked");
    QVERIFY(hello->property("isClosed").toBool());
    QCOMPARE(m_core->tabs()->activeTabId(), tab);
    QCOMPARE(currentWebView(), view);
    QCOMPARE(m_window->property("activateCount").toInt(), activations + 1);
    QCOMPARE(repliesIn(view).mid(2),
             (QStringList{QStringLiteral("click:2"), QStringLiteral("close:2")}));
    settle();
    QVERIFY(hello.isNull());
    QVERIFY(notifications->keys().isEmpty());

    relay(view, message(QStringLiteral("show"), 3, QStringLiteral("Over")), QLatin1String(ChatSite),
          QStringLiteral("granted"));
    openMenuItem(QStringLiteral("settingsMenuButton"));
    QCOMPARE(currentPage()->objectName(), QStringLiteral("settingsPage"));
    QMetaObject::invokeMethod(findAll(QStringLiteral("webNotification")).first(), "clicked");
    QCOMPARE(currentPage()->objectName(), QStringLiteral("browserPage"));

    relay(view, message(QStringLiteral("show"), 4, QStringLiteral("Swiped")),
          QLatin1String(ChatSite), QStringLiteral("granted"));
    QPointer<QObject> swiped = findAll(QStringLiteral("webNotification")).first();
    QMetaObject::invokeMethod(swiped, "closed", Q_ARG(int, 1));
    QCOMPARE(repliesIn(view).last(), QStringLiteral("close:4"));
    settle();
    QVERIFY(swiped.isNull());

    relay(view, message(QStringLiteral("show"), 5, QStringLiteral("Closed")),
          QLatin1String(ChatSite), QStringLiteral("granted"));
    QPointer<QObject> closed = findAll(QStringLiteral("webNotification")).first();
    relay(view, message(QStringLiteral("close"), 5));
    QVERIFY(closed->property("isClosed").toBool());
    settle();
    QVERIFY(closed.isNull());

    relay(view, message(QStringLiteral("show"), 6, QStringLiteral("No")));
    QVERIFY(findAll(QStringLiteral("webNotification")).isEmpty());
    QCOMPARE(repliesIn(view).last(), QStringLiteral("error:6"));

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

// Permission prompt Firefox-style (allow, always block, not now); only for on-screen page;
// platform's own refusal retracted.
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

    relay(view, message(QStringLiteral("request"), 3), QStringLiteral("https://news.example"));
    QCOMPARE(question(), QStringLiteral("Allow news.example to send notifications?"));
    click(find(QStringLiteral("blockNotificationsButton")));
    QVERIFY(permissions->isBlocked(QStringLiteral("https://news.example")));
    QCOMPARE(repliesIn(view).last(), QStringLiteral("permission:3:denied"));
    popPage();

    relay(view, message(QStringLiteral("request"), 4), QStringLiteral("https://shop.example"));
    QMetaObject::invokeMethod(currentPage(), "reject");
    QCOMPARE(repliesIn(view).last(), QStringLiteral("permission:4:denied"));
    QCOMPARE(permissions->rowCount(), 2);
    popPage();

    relay(view, message(QStringLiteral("request"), 5), QStringLiteral("https://gone.example"));
    QVERIFY(!question().isEmpty());
    relay(view, {{QStringLiteral("type"), QStringLiteral("unload")}});
    QCOMPARE(currentPage()->objectName(), QStringLiteral("browserPage"));
    QCOMPARE(repliesIn(view).count(), 4);

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
    relay(front, message(QStringLiteral("request"), 9), QStringLiteral("https://asleep.example"));
    QCOMPARE(repliesIn(front).last(), QStringLiteral("permission:9:denied"));
    QMetaObject::invokeMethod(page, "applicationStateChanged",
                              Q_ARG(QVariant, Qt::ApplicationActive));
}

// Settings > Notifications: may-ask switch; engine-kept sites, allowed/blocked under own
// headings, each changeable/forgettable.
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

    openMenuItem(QStringLiteral("settingsMenuButton"));
    click(find(QStringLiteral("sitePermissionsSettingsEntry")));
    QCOMPARE(currentPage()->objectName(), QStringLiteral("sitePermissionsPage"));
    click(find(QStringLiteral("notificationsPermissionRow")));
    QObject *page = currentPage();
    QCOMPARE(page->objectName(), QStringLiteral("notificationSettingsPage"));
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
    QCOMPARE(find(QStringLiteral("notificationSitesFooter"))->property("text").toString(),
             QStringLiteral("A site you forget asks again the next time it wants to send one."));

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

    permissions->remove(QLatin1String(ChatSite));
    QVERIFY(findAll(QStringLiteral("notificationSite")).isEmpty());
}

namespace {

// Engine permission list as qtmozembed delivers it.
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

// Site permissions: row per kind with default + exception count; tap changes default or opens
// exceptions. Cookies row only while tracking protection off or a site has cookie exception;
// TP-off sites under own heading. Choices saved live and sent to engine.
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
    QCOMPARE(lastToEngine().value(QStringLiteral("msg")).toString(), QStringLiteral("get-all"));

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
    QVERIFY(!shownIn(page, "cookiesPermissionRow"));
    QVERIFY(!shownIn(page, "trackingExceptionsSection"));
    QVERIFY(!shownIn(page, "trackingPermissionRow"));

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

// Cookies on Site permissions belong to TP while on: row appears when TP off or a site has
// cookie exception; choice = engine cookie behaviour while TP off. Choice-less rows navigate.
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
    privacy->setTrackingProtection(PrivacySettings::TrackingProtectionStandard);
    QCOMPARE(cookieBehavior(), 5);
    QVERIFY(!shownIn(page, "cookiesPermissionRow"));
    evaluate(scope, permissionList({{QStringLiteral("cookie"), QStringLiteral("https://c.example"),
                                     QStringLiteral("1")}}));
    QVERIFY(shownIn(page, "cookiesPermissionRow"));
    QCOMPARE(description("cookiesPermissionRow"), QStringLiteral("1 exception(s)"));

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

// Per-kind exceptions: allowed/blocked headings, row menu change/remove; pulley adds by
// address, removes all.
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
    QVERIFY(findAll(QStringLiteral("siteException")).isEmpty());
    QVERIFY(!shownIn(page, "siteExceptionsFooter"));
    QVERIFY(!shownIn(page, "removeAllMenuItem"));
    QCOMPARE(textOf(page, "addSiteMenuItem"), QStringLiteral("Add a site"));
    QCOMPARE(textOf(page, "removeAllMenuItem"), QStringLiteral("Remove all exceptions"));

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
    QCOMPARE(textOf(rows.at(0), "siteExceptionHost"), QStringLiteral("op.example"));
    QCOMPARE(textOf(rows.at(1), "siteExceptionHost"), QStringLiteral("iltalehti.example"));
    QCOMPARE(textOf(rows.at(2), "siteExceptionHost"), QStringLiteral("vr.example"));
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
    click(findObjects(rows.at(1), QStringLiteral("siteExceptionRemove")).first());
    QCOMPARE(lastToEngine().value(QStringLiteral("msg")).toString(), QStringLiteral("remove"));
    QCOMPARE(lastToEngine().value(QStringLiteral("uri")).toString(),
             QStringLiteral("https://op.example"));
    QCOMPARE(findAll(QStringLiteral("siteException")).count(), 2);

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

    // Allow default (Silica combo starts at first item); invalid site not added.
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

// Askable kind: ask-each-time sites under own heading; ask is choice in rows and add dialog.
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
    click(find(QStringLiteral("addSiteMenuItem")));
    QObject *askDialog = currentPage();
    QVERIFY(shownIn(askDialog, "siteExceptionAskChoice"));
    find(QStringLiteral("siteExceptionAddress"))
        ->setProperty("text", QStringLiteral("https://new.example"));
    find(QStringLiteral("siteExceptionDecision"))->setProperty("currentIndex", 2);
    QMetaObject::invokeMethod(askDialog, "accept");
    QCOMPARE(sites->decision(SitePermissions::Location, QStringLiteral("https://new.example")),
             int(SitePermissions::Ask));
    evaluate(scope, QStringLiteral("pageStack.pop(null, PageStackAction.Immediate)"));
    evaluate(scope, QStringLiteral("pageStack.pop(null, PageStackAction.Immediate)"));
}

// Sheet head opens site details (chevron after title); copy button keeps own tap; start page:
// no site, no entry.
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

    click(find(QStringLiteral("copyAddressButton")));
    QCOMPARE(currentPage()->objectName(), QStringLiteral("browserPage"));
    QVERIFY(!menu->property("open").toBool());

    tapBar(QStringLiteral("menu"));
    click(find(QStringLiteral("menuHeaderDetails")));
    QObject *details = currentPage();
    QCOMPARE(details->objectName(), QStringLiteral("siteDetailsPage"));
    QVERIFY(!menu->property("open").toBool());
    QCOMPARE(details->property("url").toString(), QLatin1String(FirstPage));
    QCOMPARE(details->property("title").toString(), QStringLiteral("Qwant"));
    QCOMPARE(details->property("view").value<QObject *>(), currentWebView());
    popPage();

    m_core->tabs()->newTab(QString());
    tapBar(QStringLiteral("menu"));
    QVERIFY(!shownIn(menu, "menuHeaderChevron"));
    QVERIFY(!findObjects(menu, QStringLiteral("menuHeaderDetails"))
                 .first()
                 ->property("enabled")
                 .toBool());
}

// Site details: secure or not, certificate issuer, engine certificate info.
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

    security()->setProperty("issuerDisplayName", QStringLiteral("Let's Encrypt (R11)"));
    security()->setProperty("subjectDisplayName", QStringLiteral("www.qwant.com"));
    security()->setProperty("expiryDate", QDateTime(QDate(2026, 12, 14), QTime(12, 0)));
    security()->setProperty("protocolVersion", 4);
    security()->setProperty("cipherName", QStringLiteral("TLS_AES_128_GCM_SHA256"));
    QObject *page = open();
    QCOMPARE(page->objectName(), QStringLiteral("siteDetailsPage"));
    const QVariantList sent = evaluate(scope, QStringLiteral("WebEngine.notifications")).toList();
    QCOMPARE(
        sent.last().toMap().value(QStringLiteral("value")).toMap().value(QStringLiteral("msg")),
        QVariant(QStringLiteral("get-all")));
    QCOMPARE(page->property("origin").toString(), QStringLiteral("https://www.qwant.com"));

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
    QVERIFY(!shownIn(page, "siteSecurityDetail"));
    security()->setProperty("issuerDisplayName", QStringLiteral("Let's Encrypt (R11)"));
    security()->setProperty("subjectDisplayName", QStringLiteral("www.qwant.com"));

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
    QVERIFY(shownIn(page, "siteConnectionDetails"));
    security()->setProperty("domainMismatch", true);
    QCOMPARE(textOf(page, "siteSecurityDetail"),
             QStringLiteral("The certificate has expired or is not yet valid"));
    security()->setProperty("notValidAtThisTime", false);
    QCOMPARE(textOf(page, "siteSecurityDetail"),
             QStringLiteral("The certificate is for another site"));
    security()->setProperty("domainMismatch", false);
    security()->setProperty("untrusted", true);
    QCOMPARE(textOf(page, "siteSecurityDetail"), QStringLiteral("The certificate is not trusted"));
    security()->setProperty("untrusted", false);
    QVERIFY(!shownIn(page, "siteSecurityDetail"));
    QCOMPARE(textOf(page, "siteSecurityTitle"), QStringLiteral("Connection is not secure"));
    security()->setProperty("validState", false);
    QCOMPARE(textOf(page, "siteSecurityTitle"), QStringLiteral("Connection is secure"));
    security()->setProperty("validState", true);
    security()->setProperty("allGood", true);
    QCOMPARE(textOf(page, "siteSecurityTitle"), QStringLiteral("Connection is secure"));
    popPage();

    typeAddress(QStringLiteral("http://plain.example/"));
    QCOMPARE(m_core->tabs()->activeUrl(), QStringLiteral("http://plain.example/"));
    page = open();
    QCOMPARE(page->objectName(), QStringLiteral("siteDetailsPage"));
    QCOMPARE(textOf(page, "siteSecurityTitle"), QStringLiteral("Connection is not secure"));
    QVERIFY(!shownIn(page, "siteSecurityDetail"));
    QVERIFY(shownIn(page, "siteSecurityWarning"));
    QCOMPARE(icon(page, "source").toString(), QStringLiteral("image://theme/icon-m-warning"));
    QVERIFY(!shownIn(page, "siteConnectionDetails"));
    QCOMPARE(page->property("origin").toString(), QStringLiteral("http://plain.example"));
    QCOMPARE(findAll(QStringLiteral("siteDecisionRow")).count(), 6);
    popPage();

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

// Per-site TP: switch adds/removes origin on engine allow list, reloads; off in Settings = off
// everywhere, said so.
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

    QVERIFY(toggle->property("checked").toBool());
    QVERIFY(toggle->property("enabled").toBool());
    const QString onLine =
        QStringLiteral("If something looks broken on this site, try turning this off.");
    QCOMPARE(toggle->property("description").toString(), onLine);
    privacy->setTrackingProtection(PrivacySettings::TrackingProtectionStrict);
    QCOMPARE(toggle->property("description").toString(), onLine);
    privacy->setTrackingProtection(PrivacySettings::TrackingProtectionStandard);

    auto *security = currentWebView()->property("security").value<QObject *>();
    QVERIFY(!shownIn(page, "siteTrackersBlocked"));
    security->setProperty("blockedTrackingContent", true);
    QVERIFY(shownIn(page, "siteTrackersBlocked"));
    QCOMPARE(textOf(page, "siteTrackersBlocked"),
             QStringLiteral("Trackers were blocked on this page"));

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
    QVERIFY(!shownIn(page, "siteTrackersBlocked"));
    QCOMPARE(sites->count(SitePermissions::TrackingProtection), 1);

    click(toggle);
    QCOMPARE(sites->decision(SitePermissions::TrackingProtection, site),
             int(SitePermissions::Default));
    QCOMPARE(lastToEngine().value(QStringLiteral("msg")).toString(), QStringLiteral("remove"));
    QCOMPARE(lastToEngine().value(QStringLiteral("type")).toString(),
             QStringLiteral("trackingprotection"));
    QCOMPARE(reloads(), before + 2);
    QVERIFY(toggle->property("checked").toBool());
    QVERIFY(shownIn(page, "siteTrackersBlocked"));

    privacy->setTrackingProtection(PrivacySettings::TrackingProtectionOff);
    QVERIFY(!toggle->property("checked").toBool());
    QVERIFY(!toggle->property("enabled").toBool());
    QCOMPARE(toggle->property("description").toString(), QStringLiteral("Off in Settings"));
    QVERIFY(!shownIn(page, "siteTrackersBlocked"));
    privacy->setTrackingProtection(PrivacySettings::TrackingProtectionStandard);
    QVERIFY(toggle->property("checked").toBool());

    page->setProperty("view", QVariant::fromValue<QObject *>(nullptr));
    click(find(QStringLiteral("siteTrackingSwitch")));
    QCOMPARE(sites->decision(SitePermissions::TrackingProtection, site),
             int(SitePermissions::Allow));
    QCOMPARE(reloads(), before + 2);
}

// Site's grants, row per kind, edited in place; cookies only while TP off (global or site) or
// site has cookie exception; clear-all button.
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
    const auto marked = [&value](int kind) {
        return value(kind).startsWith(QStringLiteral("Follow default: "));
    };
    const auto pulley = [this]() { return find(QStringLiteral("siteDetailsPulley")); };
    const auto pulleyShown = [&pulley]() {
        return qobject_cast<QQuickItem *>(pulley())->isVisible();
    };

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
    QVERIFY(!pulleyShown());
    QCOMPARE(find(QStringLiteral("siteSecurityHero"))->property("topPadding").toReal(),
             evaluate(page, QStringLiteral("Theme.paddingLarge * 2")).toReal());

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
    QVERIFY(m_core->notificationPermissions()->isAllowed(site));

    click(findObjects(row(SitePermissions::Location), QStringLiteral("siteDecisionBlock")).first());
    QCOMPARE(value(SitePermissions::Location), QStringLiteral("Blocked"));
    QCOMPARE(lastToEngine().value(QStringLiteral("permission")).toInt(), 2);
    click(findObjects(row(SitePermissions::Popups), QStringLiteral("siteDecisionAllow")).first());
    QCOMPARE(value(SitePermissions::Popups), QStringLiteral("Allowed"));
    QVERIFY(!marked(SitePermissions::Popups));
    QVERIFY(marked(SitePermissions::Camera));

    click(findObjects(row(SitePermissions::Popups), QStringLiteral("siteDecisionDefault")).first());
    QCOMPARE(sites->decision(SitePermissions::Popups, site), int(SitePermissions::Default));
    QCOMPARE(value(SitePermissions::Popups), QStringLiteral("Follow default: Block"));
    QVERIFY(marked(SitePermissions::Popups));
    QCOMPARE(lastToEngine().value(QStringLiteral("msg")).toString(), QStringLiteral("remove"));
    QCOMPARE(lastToEngine().value(QStringLiteral("type")).toString(), QStringLiteral("popup"));

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
    QCOMPARE(reloads(), before);

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

// Tutorial sketch cells (not real grid), layout order: by row, left to right.
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

// First start: tutorial over browsing page, first card: mark + name + blurb, start/skip.
// Shown once: next start page only.
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

    click(find(QStringLiteral("tutorialSkipButton")));
    QCOMPARE(currentPage()->objectName(), QStringLiteral("browserPage"));
    QVERIFY(find(QStringLiteral("startPageLayer"))->property("active").toBool());

    m_window.reset();
    m_engine.reset();
    QVERIFY(loadWindow());
    QTest::qWait(100);
    QCOMPARE(currentPage()->objectName(), QStringLiteral("browserPage"));
    QCOMPARE(pageStack()->property("depth").toInt(), 1);

    evaluate(m_window.data(), QStringLiteral("showTutorial()"));
    page = currentPage();
    QCOMPARE(page->property("step").toString(), QStringLiteral("welcome"));
    click(find(QStringLiteral("tutorialStartButton")));
    QCOMPARE(page->property("step").toString(), QStringLiteral("address"));
    QTRY_VERIFY(!find(QStringLiteral("tutorialWelcome"))->property("visible").toBool());
}

// Tutorial from Settings, straight into lessons like platform Tutorial: hint where finger goes,
// label at opposite end, waits for own gesture (others ignored). Address bar (go + search),
// menu; every step unique text.
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

    QStringList said;
    for (const QVariant &lesson : page->property("lessons").toList()) {
        said.append(
            evaluate(page, QStringLiteral("stepText('%1')").arg(lesson.toString())).toString());
        QVERIFY2(!said.last().isEmpty(), qPrintable(lesson.toString()));
    }
    QCOMPARE(said.count(), 9);
    QCOMPARE(QSet<QString>(said.begin(), said.end()).count(), 9);

    QCOMPARE(step(), QStringLiteral("address"));
    QVERIFY(!find(QStringLiteral("tutorialWelcome"))->property("visible").toBool());
    QVERIFY(page->property("hinting").toBool());
    QVERIFY(tapping());
    QVERIFY(!moving());
    QCOMPARE(centre(tapHint), point(bar, "addressCentre"));
    QVERIFY(labelAtTop());
    QCOMPARE(label->property("text").toString(), said.at(0));
    QCOMPARE(said.at(0), QStringLiteral("Tap the address bar to open a website or search."));
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

    evaluate(bar, QStringLiteral("activate('menu')"));
    evaluate(bar, QStringLiteral("dragStarted()"));
    evaluate(bar, QStringLiteral("dragMoved(%1)").arg(threshold + 1));
    evaluate(bar, QStringLiteral("dragFinished(%1)").arg(threshold + 1));
    QCOMPARE(step(), QStringLiteral("address"));
    QVERIFY(!deck->property("tabsOpen").toBool());
    QCOMPARE(deck->property("tabsOffset").toReal(), qreal(0));

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

// Tutorial grid: bar drag up (short release springs back); grid of four fake pages, tab closed,
// moved, moved to other group; pull down. Sketch only, no real tab touched. End card; close
// returns to opener.
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

    QCOMPARE(step(), QStringLiteral("closeTab"));
    QCOMPARE(cells().count(), 4);
    QVERIFY(cells().first()->property("highlighted").toBool());
    QVERIFY(QFile::exists(evaluate(cells().first(), QStringLiteral("model.thumbnail")).toString()));
    QCOMPARE(touchHint->property("direction"), enumValue("Left"));
    QCOMPARE(touchHint->property("interactionMode"), enumValue("Swipe"));
    // Placed at first row's final spot while deck still springing.
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
    QVERIFY(
        findObjects(recap, QStringLiteral("tutorialCheck")).first()->property("visible").toBool());
    QCOMPARE(
        findObjects(recap, QStringLiteral("tutorialCardText")).first()->property("text").toString(),
        QStringLiteral("You can open it again from Settings."));
    QVERIFY(!find(QStringLiteral("tutorialProgress"))->property("visible").toBool());

    QCOMPARE(m_core->tabs()->count(), tabCount);
    QCOMPARE(m_core->tabs()->activeTabId(), activeTab);
    QVERIFY(!find(QStringLiteral("browserPage"))->property("tabsOpen").toBool());

    click(find(QStringLiteral("tutorialCloseButton")));
    QCOMPARE(currentPage(), settings);
}

// Tutorial gestures by real finger from first card: bar/menu-side taps, drag up via bar's own
// gesture, cell slide/hold/carry, carry onto group through real grid cells, pull down = sketch
// grid overscroll.
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
    const auto count = [&]() {
        return find(QStringLiteral("tutorialGrid"))->property("count").toInt();
    };

    drag(&window, QPoint(across, onBar), QPoint(across, onBar) - up);
    QVERIFY(!deck->property("tabsOpen").toBool());
    click(find(QStringLiteral("tutorialStartButton")));
    QTRY_VERIFY(!find(QStringLiteral("tutorialWelcome"))->property("visible").toBool());

    auto *barItem = qobject_cast<QQuickItem *>(bar);
    tap(barItem->mapToScene(bar->property("addressCentre").toPointF()));
    QCOMPARE(step(), QStringLiteral("omnibar"));
    click(find(QStringLiteral("tutorialContinueButton")));
    tap(barItem->mapToScene(bar->property("menuCentre").toPointF()));
    QCOMPARE(step(), QStringLiteral("menuOpen"));
    tap(QPointF(across, window.height() / 4.0));
    QCOMPARE(step(), QStringLiteral("open"));

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

    const QPoint slide = centreOf(cells().at(1));
    drag(&window, slide, slide - QPoint(cells().at(1)->property("width").toInt() / 2, 0));
    QCOMPARE(count(), 3);
    QCOMPARE(step(), QStringLiteral("moveTab"));
    QTRY_COMPARE(cells().count(), 3);

    QObject *first = cells().first();
    const QPoint grab = centreOf(first);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, grab);
    QTRY_VERIFY(first->property("held").toBool());
    const QPoint neighbour = centreOf(cells().at(1));
    QTest::mouseMove(&window, (grab + neighbour) / 2);
    QTest::mouseMove(&window, neighbour);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, neighbour);
    QCOMPARE(step(), QStringLiteral("groupTab"));

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

    QTRY_VERIFY(!find(QStringLiteral("tutorialGridView"))->property("moving").toBool());
    const QPoint middle(across, window.height() / 2);
    drag(&window, middle, middle + up);
    QVERIFY(!deck->property("tabsOpen").toBool());
    QCOMPARE(step(), QStringLiteral("done"));
}

#include "tst_qmlload.moc"
