// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 tuuli contributors
#pragma once

#include <QObject>
#include <QSettings>
#include <QString>
#include <QStringList>

namespace Tuuli {

// User preferences, stored in the Sailjail-approved config location. Also owns the
// address-bar heuristics because "what does typed text mean" depends on the search engine.
class Settings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString homePage READ homePage WRITE setHomePage NOTIFY homePageChanged)
    Q_PROPERTY(
        QString searchEngine READ searchEngine WRITE setSearchEngine NOTIFY searchEngineChanged)
    Q_PROPERTY(int searchEngineIndex READ searchEngineIndex WRITE setSearchEngineIndex NOTIFY
                   searchEngineChanged)
    Q_PROPERTY(QStringList searchEngineNames READ searchEngineNames CONSTANT)
    Q_PROPERTY(bool desktopMode READ desktopMode WRITE setDesktopMode NOTIFY desktopModeChanged)
    Q_PROPERTY(bool cutoutGuard READ cutoutGuard WRITE setCutoutGuard NOTIFY cutoutGuardChanged)
    Q_PROPERTY(int coverStyle READ coverStyle WRITE setCoverStyle NOTIFY coverStyleChanged)

public:
    // How much of itself the cover shows; see docs/DECISIONS/0014-cover-is-the-tab-count.md.
    // The values are stored, so their numbers are part of the file format.
    enum CoverStyle
    {
        CoverIconOnly = 0,
        CoverLatestTab = 1,
        CoverEveryTab = 2
    };
    Q_ENUM(CoverStyle)

    explicit Settings(const QString &filePath, QObject *parent = nullptr);

    QString homePage() const;
    void setHomePage(const QString &url);

    QString searchEngine() const;
    void setSearchEngine(const QString &key);
    int searchEngineIndex() const;
    void setSearchEngineIndex(int index);
    QStringList searchEngineNames() const;
    QStringList searchEngineKeys() const;

    bool desktopMode() const;
    void setDesktopMode(bool desktopMode);

    // Whether this application keeps out of the display's own cutout. On by default:
    // a camera notch over the first line of a page is not a design decision.
    bool cutoutGuard() const;
    void setCutoutGuard(bool cutoutGuard);

    // Out-of-range values read back as the default rather than as a cover that draws
    // nothing: this comes from a file a user can edit.
    int coverStyle() const;
    void setCoverStyle(int style);

    Q_INVOKABLE QString searchUrl(const QString &query) const;
    // Typed address-bar text: a URL as-is, a host with a scheme added, or a search.
    Q_INVOKABLE QString urlForInput(const QString &input) const;
    // The other direction: the url as the bar shows it while it is not being edited.
    Q_INVOKABLE static QString displayAddress(const QString &url);

    static QString defaultHomePage();
    static QString defaultSearchEngine();

signals:
    void homePageChanged();
    void searchEngineChanged();
    void desktopModeChanged();
    void cutoutGuardChanged();
    void coverStyleChanged();

private:
    QSettings m_settings;
};

} // namespace Tuuli
