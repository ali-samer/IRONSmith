#include "aieplugin/design/CanvasDocumentImporter.hpp"

#include "aieplugin/AieService.hpp"
#include "aieplugin/AieCanvasCoordinator.hpp"
#include "aieplugin/design/DesignStateCanvas.hpp"
#include "aieplugin/design/DesignStateJson.hpp"

#include "canvas/api/ICanvasHost.hpp"
#include "canvas/CanvasView.hpp"

#include <QtCore/QLoggingCategory>

Q_LOGGING_CATEGORY(aieimportlog, "ironsmith.aie.import")

namespace Aie::Internal {

CanvasDocumentImporter::CanvasDocumentImporter(AieService* service)
    : m_service(service)
{
}

Utils::Result CanvasDocumentImporter::importDesign(const DesignModel& model) const
{
    if (!m_service)
        return Utils::Result::failure(QStringLiteral("AIE service is not available."));

    qCDebug(aieimportlog) << "importDesign: setting profile" << model.deviceId;
    const Utils::Result profileResult = m_service->setProfileId(model.deviceId);
    if (!profileResult)
        return profileResult;

    if (auto* host = m_service->canvasHost())
        host->setCanvasActive(true);

    qCDebug(aieimportlog) << "importDesign: applyDesignState";
    return applyDesignState(model);
}

Utils::Result CanvasDocumentImporter::applyDesignState(const DesignModel& model) const
{
    if (!m_service)
        return Utils::Result::failure(QStringLiteral("AIE service is not available."));
    auto* host = m_service->canvasHost();
    if (!host || !host->document())
        return Utils::Result::failure(QStringLiteral("Canvas host is not available."));

    qCDebug(aieimportlog) << "applyDesignState: parsing design state";
    DesignState state;
    Utils::Result parseResult = parseDesignState(model.design, state);
    if (!parseResult)
        return parseResult;

    auto* view = qobject_cast<Canvas::CanvasView*>(host->viewWidget());
    qCDebug(aieimportlog) << "applyDesignState: applying to canvas";
    return applyDesignStateToCanvas(state, *host->document(), view);
}

} // namespace Aie::Internal
