// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "projectexplorer/ProjectExplorerGlobal.hpp"

#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QSize>
#include <QtGui/QPixmap>

namespace ProjectExplorer::Internal {

class ProjectExplorerThumbnailService final : public QObject
{
    Q_OBJECT

public:
    explicit ProjectExplorerThumbnailService(QObject* parent = nullptr);

    void requestThumbnail(const QString& absolutePath, const QSize& targetSize);
    void clearCache();

signals:
    void thumbnailReady(const QString& path, const QPixmap& pixmap);

private:
    QHash<QString, QPixmap> m_cache;
    QSet<QString> m_pending;
};

} // namespace ProjectExplorer::Internal
