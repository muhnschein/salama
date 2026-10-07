// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include <QImage>
#include <QObject>
#include <QString>
#include <QThreadPool>

namespace Salama {

// Encodes + writes previews off GUI thread: PNG encode there stuttered grid-opening drag. One
// worker: writes land in order.
class ThumbnailWriter : public QObject
{
    Q_OBJECT

public:
    explicit ThumbnailWriter(QObject *parent = nullptr);
    // Waits for pending writes so none outlives target dir.
    ~ThumbnailWriter() override;

    void write(int tabId, const QImage &image, const QString &path);
    // written() may still be queued.
    void waitForDone();

signals:
    void written(int tabId, const QString &path, bool saved);

private:
    QThreadPool m_pool;
};

} // namespace Salama
