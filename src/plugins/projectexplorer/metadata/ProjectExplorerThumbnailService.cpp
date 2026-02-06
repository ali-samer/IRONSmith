#include "projectexplorer/metadata/ProjectExplorerThumbnailService.hpp"

#include <utils/async/AsyncTask.hpp>

#include <QtGui/QImageReader>

namespace ProjectExplorer::Internal {

namespace {

struct ThumbnailResult final {
    QString path;
    QImage image;
};

ThumbnailResult loadThumbnail(const QString& path, const QSize& targetSize)
{
    ThumbnailResult result;
    result.path = path;

    QImageReader reader(path);
    reader.setAutoTransform(true);
    const QImage image = reader.read();
    if (image.isNull())
        return result;

    if (targetSize.isValid())
        result.image = image.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    else
        result.image = image;

    return result;
}

} // namespace

ProjectExplorerThumbnailService::ProjectExplorerThumbnailService(QObject* parent)
    : QObject(parent)
{
}

void ProjectExplorerThumbnailService::requestThumbnail(const QString& absolutePath, const QSize& targetSize)
{
    const QString cleaned = absolutePath.trimmed();
    if (cleaned.isEmpty())
        return;

    if (const auto it = m_cache.find(cleaned); it != m_cache.end()) {
        emit thumbnailReady(cleaned, it.value());
        return;
    }

    if (m_pending.contains(cleaned))
        return;

    m_pending.insert(cleaned);

    Utils::Async::run<ThumbnailResult>(this,
                                       [cleaned, targetSize]() { return loadThumbnail(cleaned, targetSize); },
                                       [this](ThumbnailResult result) {
                                           m_pending.remove(result.path);
                                           if (result.image.isNull())
                                               return;
                                           QPixmap pixmap = QPixmap::fromImage(result.image);
                                           m_cache.insert(result.path, pixmap);
                                           emit thumbnailReady(result.path, pixmap);
                                       });
}

void ProjectExplorerThumbnailService::clearCache()
{
    m_cache.clear();
    m_pending.clear();
}

} // namespace ProjectExplorer::Internal
