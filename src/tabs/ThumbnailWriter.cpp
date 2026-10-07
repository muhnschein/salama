// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "ThumbnailWriter.h"

#include <QRunnable>
#include <utility>

namespace Salama {

namespace {

// QThreadPool::start(function) needs Qt 5.15; device has 5.6.
class WriteJob : public QRunnable
{
public:
    WriteJob(ThumbnailWriter *writer, int tabId, QImage image, QString path)
        : m_writer(writer)
        , m_tabId(tabId)
        , m_image(std::move(image))
        , m_path(std::move(path))
    {
    }

    // Writer waits for pool before dying, so still alive to signal.
    void run() override
    {
        const bool saved = m_image.save(m_path, "PNG");
        emit m_writer->written(m_tabId, m_path, saved);
    }

private:
    ThumbnailWriter *m_writer;
    int m_tabId;
    QImage m_image;
    QString m_path;
};

} // namespace

ThumbnailWriter::ThumbnailWriter(QObject *parent)
    : QObject(parent)
{
    m_pool.setMaxThreadCount(1);
}

ThumbnailWriter::~ThumbnailWriter()
{
    m_pool.waitForDone();
}

void ThumbnailWriter::write(int tabId, const QImage &image, const QString &path)
{
    m_pool.start(new WriteJob(this, tabId, image, path));
}

void ThumbnailWriter::waitForDone()
{
    m_pool.waitForDone();
}

} // namespace Salama
