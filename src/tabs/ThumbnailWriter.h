// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include <QImage>
#include <QObject>
#include <QString>
#include <QThreadPool>

namespace Salama {

// Encodes a tab's preview and writes it to disk away from the GUI thread. A preview is
// taken as the finger goes down on the bar, and the PNG encode of a picture half the
// screen in each direction, and the write, used to run on the GUI thread inside the
// first frames of the drag that opens the grid -- the stutter at its very start
// (docs/DECISIONS/0008-tab-previews.md). One worker, so writes land in the order they
// were asked for.
class ThumbnailWriter : public QObject
{
    Q_OBJECT

public:
    explicit ThumbnailWriter(QObject *parent = nullptr);
    // Waits for the writes under way, so none outlives the directory it writes into.
    ~ThumbnailWriter() override;

    void write(int tabId, const QImage &image, const QString &path);
    // Every write asked for so far has finished and said so to its worker; written()
    // is still queued for this object's thread.
    void waitForDone();

signals:
    // On this object's thread, once the file is complete or has failed.
    void written(int tabId, const QString &path, bool saved);

private:
    QThreadPool m_pool;
};

} // namespace Salama
