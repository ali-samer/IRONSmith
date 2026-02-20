// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include "projectexplorer/metadata/ProjectExplorerMetadataService.hpp"

#include <utils/async/AsyncTask.hpp>

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QXmlStreamReader>

namespace ProjectExplorer::Internal {

namespace {

ProjectExplorerFileMetadata loadMetadata(const QString& path)
{
    ProjectExplorerFileMetadata meta;
    QFileInfo info(path);
    if (!info.exists() || !info.isFile())
        return meta;

    meta.valid = true;
    meta.displayName = info.completeBaseName();
    meta.extension = info.suffix().toLower();
    meta.sizeBytes = info.size();
    meta.lastModified = info.lastModified();

    if (meta.extension == QStringLiteral("json") || meta.extension == QStringLiteral("irondesign")) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            if (doc.isObject()) {
                const auto obj = doc.object();
                const QString name = obj.value(QStringLiteral("name")).toString();
                if (!name.isEmpty())
                    meta.displayName = name;
            }
        }
    } else if (meta.extension == QStringLiteral("graphml") || meta.extension == QStringLiteral("xml")) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            QXmlStreamReader reader(&file);
            if (reader.readNextStartElement()) {
                const QString rootName = reader.name().toString();
                if (!rootName.isEmpty())
                    meta.displayName = rootName;
            }
        }
    }

    return meta;
}

} // namespace

ProjectExplorerMetadataService::ProjectExplorerMetadataService(QObject* parent)
    : QObject(parent)
{
}

void ProjectExplorerMetadataService::requestMetadata(const QString& absolutePath)
{
    const QString cleaned = absolutePath.trimmed();
    if (cleaned.isEmpty())
        return;

    if (const auto it = m_cache.find(cleaned); it != m_cache.end()) {
        emit metadataReady(cleaned, it.value());
        return;
    }

    if (m_pending.contains(cleaned))
        return;

    m_pending.insert(cleaned);

    Utils::Async::run<ProjectExplorerFileMetadata>(this,
                                                   [cleaned]() { return loadMetadata(cleaned); },
                                                   [this, cleaned](ProjectExplorerFileMetadata meta) {
                                                       m_pending.remove(cleaned);
                                                       m_cache.insert(cleaned, meta);
                                                       emit metadataReady(cleaned, meta);
                                                   });
}

void ProjectExplorerMetadataService::clearCache()
{
    m_cache.clear();
    m_pending.clear();
}

} // namespace ProjectExplorer::Internal
