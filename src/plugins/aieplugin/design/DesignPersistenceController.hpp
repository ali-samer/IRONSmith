// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QtCore/QJsonObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QObject>

#include "utils/async/DebouncedInvoker.hpp"
#include "aieplugin/state/AieLayoutSettings.hpp"

namespace Canvas {
class CanvasDocument;
class CanvasView;

namespace Api {
class ICanvasHost;
}
}

namespace Aie {
class AieCanvasCoordinator;
}

namespace Aie::Internal {

class DesignPersistenceController final : public QObject
{
    Q_OBJECT

public:
    explicit DesignPersistenceController(QObject* parent = nullptr);

    void setCanvasHost(Canvas::Api::ICanvasHost* host);
    void setCoordinator(AieCanvasCoordinator* coordinator);
    void setActiveBundle(const QString& bundlePath, const QJsonObject& designJson);
    void clearActiveBundle();
    void flush();
    void suspend();
    void resume();

private:
    void attachToDocument();
    void detachFromDocument();
    void scheduleSave();
    void saveNow();

    QPointer<Canvas::Api::ICanvasHost> m_host;
    QPointer<AieCanvasCoordinator> m_coordinator;
    QPointer<Canvas::CanvasDocument> m_document;
    QPointer<Canvas::CanvasView> m_view;

    QString m_bundlePath;
    QJsonObject m_metadata;
    bool m_hasLayoutOverride = false;
    QJsonObject m_layoutOverride;
    LayoutSettings m_defaultLayout;
    bool m_hasDefaultLayout = false;

    Utils::Async::DebouncedInvoker m_saveDebounce;
    bool m_suspended = false;
};

} // namespace Aie::Internal
