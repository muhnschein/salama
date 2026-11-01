// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "ThumbnailWriter.h"

#include <QRunnable>
#include <algorithm>
#include <utility>

namespace Salama {

namespace {

// Start page picture is see-through (ambience shows through in grid). JPEG drops alpha: tiles
// came out black, colours saturated. Scan on worker thread; page pictures stay JPEG.
bool seeThrough(const QImage &image)
{
    if (!image.hasAlphaChannel()) {
        return false;
    }
    const QImage alpha = image.convertToFormat(QImage::Format_Alpha8);
    for (int y = 0; y < alpha.height(); ++y) {
        const uchar *line = alpha.constScanLine(y);
        if (std::any_of(line, line + alpha.width(), [](uchar a) { return a < 255; })) {
            return true;
        }
    }
    return false;
}

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

    // Writer waits for pool before dying, so still alive to signal. JPEG: PNG took 8x as long
    // to encode, on core Gecko paints with. Suffix follows format: Qt 5.6 Image picks decoder
    // by suffix, PNG named .jpg never loaded (empty start page tile on device).
    void run() override
    {
        if (!seeThrough(m_image)) {
            emit m_writer->written(m_tabId, m_path, m_path, m_image.save(m_path, "JPEG", 85));
            return;
        }
        QString path = m_path;
        if (path.endsWith(QLatin1String(".jpg"))) {
            path.replace(path.size() - 3, 3, QStringLiteral("png"));
        }
        emit m_writer->written(m_tabId, m_path, path, m_image.save(path, "PNG"));
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
