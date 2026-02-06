#pragma once

#include "projectexplorer/ProjectExplorerGlobal.hpp"

#include <QtCore/QDateTime>
#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QString>

namespace ProjectExplorer::Internal {

struct ProjectExplorerFileMetadata final {
    QString displayName;
    QString extension;
    qint64 sizeBytes = 0;
    QDateTime lastModified;
    bool valid = false;
};

class ProjectExplorerMetadataService final : public QObject
{
    Q_OBJECT

public:
    explicit ProjectExplorerMetadataService(QObject* parent = nullptr);

    void requestMetadata(const QString& absolutePath);
    void clearCache();

signals:
    void metadataReady(const QString& path, const ProjectExplorerFileMetadata& metadata);

private:
    QHash<QString, ProjectExplorerFileMetadata> m_cache;
    QSet<QString> m_pending;
};

} // namespace ProjectExplorer::Internal
