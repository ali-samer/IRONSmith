#include "aieplugin/design/DesignPersistenceController.hpp"

#include "aieplugin/design/DesignStateCanvas.hpp"
#include "aieplugin/design/DesignStateJson.hpp"
#include "aieplugin/state/AieLayoutSettings.hpp"
#include "aieplugin/AieCanvasCoordinator.hpp"

#include "canvas/CanvasDocument.hpp"
#include "canvas/CanvasView.hpp"
#include "canvas/api/ICanvasHost.hpp"

#include <utils/DocumentBundle.hpp>

#include <QtCore/QLoggingCategory>

Q_LOGGING_CATEGORY(aiepersistlog, "ironsmith.aie.persistence")

namespace Aie::Internal {

namespace {

constexpr int kSaveDebounceMs = 500;

QJsonObject extractMetadata(const QJsonObject& designJson)
{
    const QJsonValue metadataVal = designJson.value(QStringLiteral("metadata"));
    return metadataVal.isObject() ? metadataVal.toObject() : QJsonObject{};
}

QJsonObject extractLayout(const QJsonObject& designJson)
{
    const QJsonValue layoutVal = designJson.value(QStringLiteral("layout"));
    return layoutVal.isObject() ? layoutVal.toObject() : QJsonObject{};
}

} // namespace

DesignPersistenceController::DesignPersistenceController(QObject* parent)
    : QObject(parent)
    , m_saveDebounce(this)
{
    m_saveDebounce.setDelayMs(kSaveDebounceMs);
    m_saveDebounce.setAction([this]() { saveNow(); });
}

void DesignPersistenceController::setCanvasHost(Canvas::Api::ICanvasHost* host)
{
    if (m_host == host)
        return;

    detachFromDocument();
    m_host = host;
    attachToDocument();
}

void DesignPersistenceController::setCoordinator(AieCanvasCoordinator* coordinator)
{
    if (m_coordinator == coordinator)
        return;

    if (m_coordinator)
        disconnect(m_coordinator, nullptr, this, nullptr);

    m_coordinator = coordinator;
    if (!m_coordinator)
        return;

    connect(m_coordinator, &AieCanvasCoordinator::horizontalSpacingChanged,
            this, &DesignPersistenceController::scheduleSave);
    connect(m_coordinator, &AieCanvasCoordinator::verticalSpacingChanged,
            this, &DesignPersistenceController::scheduleSave);
    connect(m_coordinator, &AieCanvasCoordinator::outwardSpreadChanged,
            this, &DesignPersistenceController::scheduleSave);
    connect(m_coordinator, &AieCanvasCoordinator::autoCellSizeChanged,
            this, &DesignPersistenceController::scheduleSave);
    connect(m_coordinator, &AieCanvasCoordinator::cellSizeChanged,
            this, &DesignPersistenceController::scheduleSave);
    connect(m_coordinator, &AieCanvasCoordinator::keepoutMarginChanged,
            this, &DesignPersistenceController::scheduleSave);
}

void DesignPersistenceController::setActiveBundle(const QString& bundlePath, const QJsonObject& designJson)
{
    m_bundlePath = bundlePath.trimmed();
    m_metadata = extractMetadata(designJson);
    m_layoutOverride = extractLayout(designJson);
    m_hasLayoutOverride = !m_layoutOverride.isEmpty();
    m_defaultLayout = loadDefaultLayout();
    m_hasDefaultLayout = true;
    m_saveDebounce.cancel();
    if (m_coordinator) {
        if (m_hasLayoutOverride) {
            const LayoutSettings override = layoutFromJson(m_layoutOverride, m_defaultLayout);
            applyLayout(*m_coordinator, override);
        } else {
            applyLayout(*m_coordinator, m_defaultLayout);
        }
    }
    attachToDocument();
}

void DesignPersistenceController::clearActiveBundle()
{
    m_bundlePath.clear();
    m_metadata = QJsonObject{};
    m_layoutOverride = QJsonObject{};
    m_hasLayoutOverride = false;
    m_hasDefaultLayout = false;
    m_saveDebounce.cancel();
    detachFromDocument();
}

void DesignPersistenceController::flush()
{
    if (m_suspended)
        return;
    m_saveDebounce.cancel();
    saveNow();
}

void DesignPersistenceController::suspend()
{
    m_suspended = true;
    m_saveDebounce.cancel();
}

void DesignPersistenceController::resume()
{
    m_suspended = false;
}

void DesignPersistenceController::attachToDocument()
{
    if (!m_host)
        return;

    Canvas::CanvasDocument* doc = m_host->document();
    Canvas::CanvasView* view = qobject_cast<Canvas::CanvasView*>(m_host->viewWidget());

    if (m_document == doc && m_view == view)
        return;

    detachFromDocument();
    m_document = doc;
    m_view = view;

    if (m_document) {
        connect(m_document, &Canvas::CanvasDocument::changed,
                this, &DesignPersistenceController::scheduleSave);
    }
    if (m_view) {
        connect(m_view, &Canvas::CanvasView::zoomChanged,
                this, &DesignPersistenceController::scheduleSave);
        connect(m_view, &Canvas::CanvasView::panChanged,
                this, &DesignPersistenceController::scheduleSave);
    }
}

void DesignPersistenceController::detachFromDocument()
{
    if (m_document)
        disconnect(m_document, nullptr, this, nullptr);
    if (m_view)
        disconnect(m_view, nullptr, this, nullptr);
    m_document = nullptr;
    m_view = nullptr;
}

void DesignPersistenceController::scheduleSave()
{
    if (m_suspended || m_bundlePath.isEmpty() || !m_document)
        return;
    m_saveDebounce.trigger();
}

void DesignPersistenceController::saveNow()
{
    if (m_suspended || m_bundlePath.isEmpty() || !m_document)
        return;

    DesignState state;
    const Utils::Result buildResult =
        buildDesignStateFromCanvas(*m_document, m_view, m_metadata, state);
    if (!buildResult) {
        qCWarning(aiepersistlog).noquote()
            << "Failed to build design state:" << buildResult.errors.join("\n");
        return;
    }

    const QJsonObject json = serializeDesignState(state);
    QJsonObject output = json;

    if (m_coordinator) {
        const LayoutSettings current = layoutFromCoordinator(*m_coordinator);
        const LayoutSettings defaults = m_hasDefaultLayout ? m_defaultLayout : loadDefaultLayout();
        if (!layoutEquals(current, defaults)) {
            output.insert(QStringLiteral("layout"), layoutToJson(current));
            m_hasLayoutOverride = true;
        } else {
            output.remove(QStringLiteral("layout"));
            m_hasLayoutOverride = false;
        }
    }
    const Utils::Result writeResult = Utils::DocumentBundle::writeDesign(m_bundlePath, output);
    if (!writeResult) {
        qCWarning(aiepersistlog).noquote()
            << "Failed to write design state:" << writeResult.errors.join("\n");
    }
}

} // namespace Aie::Internal
