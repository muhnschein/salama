// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "ThumbnailWriter.h"

#include <QRgb>
#include <QRunnable>
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
    const QImage argb = image.convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < argb.height(); ++y) {
        const auto *line = reinterpret_cast<const QRgb *>(argb.constScanLine(y));
        for (int x = 0; x < argb.width(); ++x) {
            if (qAlpha(line[x]) < 255) {
                return true;
            }
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
    // to encode, on core Gecko paints with. Reader finds PNG by content despite .jpg name.
    void run() override
    {
        const bool saved =
            seeThrough(m_image) ? m_image.save(m_path, "PNG") : m_image.save(m_path, "JPEG", 85);
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
