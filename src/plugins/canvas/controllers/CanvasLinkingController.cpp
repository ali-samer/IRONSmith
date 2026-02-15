// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include "canvas/controllers/CanvasLinkingController.hpp"

#include "canvas/controllers/CanvasDragController.hpp"
#include "canvas/controllers/CanvasInteractionHelpers.hpp"
#include "canvas/controllers/CanvasSelectionController.hpp"
#include "canvas/services/CanvasHitTestService.hpp"

#include "canvas/CanvasBlock.hpp"
#include "canvas/CanvasCommands.hpp"
#include "canvas/CanvasConstants.hpp"
#include "canvas/CanvasDocument.hpp"
#include "canvas/CanvasPorts.hpp"
#include "canvas/CanvasSymbolContent.hpp"
#include "canvas/CanvasView.hpp"
#include "canvas/CanvasWire.hpp"
#include "canvas/utils/CanvasGeometry.hpp"
#include "canvas/utils/CanvasAutoPorts.hpp"
#include "canvas/utils/CanvasLinkHubStyle.hpp"
#include "canvas/utils/CanvasLinkWireStyle.hpp"
#include "canvas/utils/CanvasPortUsage.hpp"
#include "canvas/CanvasSymbolContent.hpp"

#include <algorithm>
#include <cmath>

namespace Canvas::Controllers {

namespace {

bool isSpecialLinkingMode(CanvasController::LinkingMode mode)
{
    return mode != CanvasController::LinkingMode::Normal;
}

Support::LinkWireRole startWireRole(CanvasController::LinkingMode mode)
{
    return mode == CanvasController::LinkingMode::Join
        ? Support::LinkWireRole::Consumer
        : Support::LinkWireRole::Producer;
}

Support::LinkWireRole finishWireRole(CanvasController::LinkingMode mode)
{
    return mode == CanvasController::LinkingMode::Join
        ? Support::LinkWireRole::Producer
        : Support::LinkWireRole::Consumer;
}

PortRole oppositePortRole(Support::LinkWireRole role)
{
    return role == Support::LinkWireRole::Producer ? PortRole::Consumer
                                                   : PortRole::Producer;
}

std::optional<WireArrowPolicy> arrowPolicyFromPortRoles(const CanvasDocument* doc,
                                                        const PortRef& a,
                                                        const PortRef& b)
{
    if (!doc)
        return std::nullopt;

    CanvasPort aMeta;
    CanvasPort bMeta;
    if (!doc->getPort(a.itemId, a.portId, aMeta) || !doc->getPort(b.itemId, b.portId, bMeta))
        return std::nullopt;

    const bool aConsumer = aMeta.role == PortRole::Consumer;
    const bool bConsumer = bMeta.role == PortRole::Consumer;

    if (aConsumer && !bConsumer)
        return WireArrowPolicy::Start;
    if (bConsumer && !aConsumer)
        return WireArrowPolicy::End;
    return std::nullopt;
}

std::optional<Support::LinkWireRole> linkWireRoleFromHubEndpoint(const CanvasDocument* doc,
                                                                 const PortRef& endpoint)
{
    if (!doc)
        return std::nullopt;
    auto* block = dynamic_cast<CanvasBlock*>(doc->findItem(endpoint.itemId));
    if (!block || !block->isLinkHub())
        return std::nullopt;

    CanvasPort meta;
    if (!doc->getPort(endpoint.itemId, endpoint.portId, meta))
        return std::nullopt;

    switch (meta.role) {
        case PortRole::Producer:
            return Support::LinkWireRole::Consumer;
        case PortRole::Consumer:
            return Support::LinkWireRole::Producer;
        case PortRole::Dynamic:
            break;
    }
    return std::nullopt;
}

std::optional<Support::LinkHubKind> hubKindFromBlock(CanvasBlock* block)
{
    if (!block)
        return std::nullopt;
    auto* content = dynamic_cast<BlockContentSymbol*>(block->content());
    if (!content)
        return std::nullopt;

    const QString symbol = content->symbol().trimmed();
    if (symbol == Support::linkHubStyle(Support::LinkHubKind::Split).symbol)
        return Support::LinkHubKind::Split;
    if (symbol == Support::linkHubStyle(Support::LinkHubKind::Join).symbol)
        return Support::LinkHubKind::Join;
    if (symbol == Support::linkHubStyle(Support::LinkHubKind::Broadcast).symbol)
        return Support::LinkHubKind::Broadcast;
    return std::nullopt;
}

std::optional<Support::LinkWireRole> wireRoleForHubConnection(CanvasBlock* hub, bool hubIsStart)
{
    const auto kind = hubKindFromBlock(hub);
    if (!kind)
        return std::nullopt;

    switch (*kind) {
        case Support::LinkHubKind::Join:
            return hubIsStart ? Support::LinkWireRole::Producer
                              : Support::LinkWireRole::Consumer;
        case Support::LinkHubKind::Split:
        case Support::LinkHubKind::Broadcast:
            return hubIsStart ? Support::LinkWireRole::Consumer
                              : Support::LinkWireRole::Producer;
    }
    return std::nullopt;
}

} // namespace

CanvasLinkingController::CanvasLinkingController(CanvasDocument* doc,
                                                 CanvasView* view,
                                                 CanvasSelectionController* selection,
                                                 CanvasDragController* drag)
    : m_doc(doc)
    , m_view(view)
    , m_selection(selection)
    , m_drag(drag)
{}

void CanvasLinkingController::setLinkingMode(CanvasController::LinkingMode mode)
{
    if (m_linkingMode == mode)
        return;
    m_linkingMode = mode;
    resetLinkingSession();
    if (m_view)
        m_view->update();
}

void CanvasLinkingController::resetLinkingSession()
{
    m_wiring = false;
    m_wireStartItem = ObjectId{};
    m_wireStartPort = PortId{};
    m_linkHubId = ObjectId{};
    if (m_view) {
        m_view->clearHoveredPort();
        m_view->clearHoveredEdge();
    }
    m_hoverEdge.reset();
}

CanvasLinkingController::LinkingPressResult
CanvasLinkingController::handleLinkingPress(const QPointF& scenePos,
                                            CanvasController::Mode mode)
{
    if (!m_view || !m_doc)
        return LinkingPressResult::NotHandled;
    if (mode != CanvasController::Mode::Linking)
        return LinkingPressResult::NotHandled;

    const double radiusScene = Constants::kPortHitRadiusPx / std::max(m_view->zoom(), 0.25);
    std::optional<PortRef> resolvedPort = m_doc->hitTestPort(scenePos, radiusScene);
    if (!resolvedPort) {
        if (auto edge = Detail::edgeCandidateAt(m_doc, m_view, scenePos))
            resolvedPort = Detail::ensureEdgePort(m_doc, *edge);
    }

    if (resolvedPort && !Support::isPortAvailable(*m_doc, resolvedPort->itemId, resolvedPort->portId)) {
        CanvasPort meta;
        auto* block = dynamic_cast<CanvasBlock*>(m_doc->findItem(resolvedPort->itemId));
        if (block && block->allowMultiplePorts() && m_doc->getPort(resolvedPort->itemId, resolvedPort->portId, meta)) {
            EdgeCandidate candidate;
            candidate.itemId = resolvedPort->itemId;
            candidate.side = meta.side;
            candidate.t = meta.t;
            candidate.anchorScene = block->portAnchorScene(meta.id);
            resolvedPort = Detail::ensureEdgePort(m_doc, candidate);
        } else {
            return LinkingPressResult::Handled;
        }
    }

    if (!resolvedPort) {
        if (isSpecialLinkingMode(m_linkingMode))
            return LinkingPressResult::RequestLinkingModeReset;

        if (m_wiring) {
            resetLinkingSession();
            m_view->update();
            return LinkingPressResult::Handled;
        }

        CanvasItem* hit = nullptr;
        if (m_view) {
            const CanvasRenderContext ctx = Detail::buildRenderContext(m_doc, m_view);
            hit = Services::hitTestItem(*m_doc, scenePos, &ctx);
        } else {
            hit = Services::hitTestItem(*m_doc, scenePos, nullptr);
        }

        if (m_selection) {
            m_selection->clearSelectedPort();
            m_selection->selectItem(hit ? hit->id() : ObjectId{});
        }
        return LinkingPressResult::Handled;
    }

    if (isSpecialLinkingMode(m_linkingMode))
        return handleLinkingHubPress(scenePos, *resolvedPort);

    if (!m_wiring) {
        m_wiring = true;
        m_wireStartItem = resolvedPort->itemId;
        m_wireStartPort = resolvedPort->portId;
        m_wirePreviewScene = scenePos;

        if (m_selection) {
            m_selection->clearSelectedPort();
            m_selection->selectItem(resolvedPort->itemId);
            m_selection->clearMarqueeSelection();
        }
        if (m_drag)
            m_drag->clearTransientState();

        m_view->update();
        return LinkingPressResult::Handled;
    }

    if (resolvedPort->itemId == m_wireStartItem && resolvedPort->portId == m_wireStartPort) {
        m_wirePreviewScene = scenePos;
        return LinkingPressResult::Handled;
    }

    CanvasPort startMeta;
    CanvasPort endMeta;
    if (m_doc->getPort(m_wireStartItem, m_wireStartPort, startMeta) &&
        m_doc->getPort(resolvedPort->itemId, resolvedPort->portId, endMeta))
    {
        const CanvasWire::Endpoint a{PortRef{m_wireStartItem, m_wireStartPort}, QPointF()};
        const CanvasWire::Endpoint b{PortRef{resolvedPort->itemId, resolvedPort->portId}, QPointF()};

        auto w = std::make_unique<CanvasWire>(a, b);
        w->setId(m_doc->allocateId());
        std::optional<Support::LinkWireRole> hubRole;
        if (auto* startBlock = dynamic_cast<CanvasBlock*>(m_doc->findItem(m_wireStartItem));
            startBlock && startBlock->isLinkHub()) {
            hubRole = wireRoleForHubConnection(startBlock, true);
        }
        if (!hubRole) {
            if (auto* endBlock = dynamic_cast<CanvasBlock*>(m_doc->findItem(resolvedPort->itemId));
                endBlock && endBlock->isLinkHub()) {
                hubRole = wireRoleForHubConnection(endBlock, false);
            }
        }
        if (hubRole) {
            const auto style = Support::linkWireStyle(*hubRole);
            w->setColorOverride(style.color);
        }

        m_doc->commands().execute(std::make_unique<CreateItemCommand>(std::move(w)));
        Support::ensureOppositeProducerPort(*m_doc, resolvedPort->itemId, resolvedPort->portId);

        resetLinkingSession();
        m_view->clearHoveredPort();
        m_view->update();
        return LinkingPressResult::Handled;
    }

    return LinkingPressResult::Handled;
}

void CanvasLinkingController::updateLinkingHoverAndPreview(const QPointF& scenePos,
                                                           CanvasController::Mode mode,
                                                           bool panning,
                                                           bool dragEndpoint)
{
    if (!m_view || !m_doc)
        return;
    if (mode != CanvasController::Mode::Linking || panning || dragEndpoint)
        return;

    const double radiusScene = Constants::kPortHitRadiusPx / std::max(m_view->zoom(), 0.25);
    if (auto hitPort = m_doc->hitTestPort(scenePos, radiusScene)) {
        m_view->setHoveredPort(hitPort->itemId, hitPort->portId);
        m_view->clearHoveredEdge();
        m_hoverEdge.reset();
    } else if (auto edge = Detail::edgeCandidateAt(m_doc, m_view, scenePos)) {
        m_view->clearHoveredPort();
        m_view->setHoveredEdge(edge->itemId, edge->side, edge->anchorScene);
        m_hoverEdge = edge;
    } else {
        m_view->clearHoveredPort();
        m_view->clearHoveredEdge();
        m_hoverEdge.reset();
    }

    const QPointF preview = (m_hoverEdge.has_value() ? m_hoverEdge->anchorScene : scenePos);
    if (m_wiring && m_wirePreviewScene != preview) {
        m_wirePreviewScene = preview;
        m_view->update();
    }
}

CanvasLinkingController::LinkingPressResult
CanvasLinkingController::handleLinkingHubPress(const QPointF& scenePos, const PortRef& hitPort)
{
    if (!m_view || !m_doc)
        return LinkingPressResult::NotHandled;

    if (m_linkHubId.isNull() && !m_wiring) {
        beginLinkingFromPort(hitPort, scenePos);
        return LinkingPressResult::Handled;
    }

    if (!m_linkHubId.isNull()) {
        if (!connectToExistingHub(scenePos, hitPort))
            return LinkingPressResult::RequestLinkingModeReset;
        return LinkingPressResult::Handled;
    }

    if (hitPort.itemId == m_wireStartItem && hitPort.portId == m_wireStartPort) {
        m_wirePreviewScene = scenePos;
        return LinkingPressResult::Handled;
    }

    createHubAndWires(scenePos, hitPort);
    return LinkingPressResult::Handled;
}

bool CanvasLinkingController::beginLinkingFromPort(const PortRef& hitPort, const QPointF& scenePos)
{
    m_wiring = true;
    m_wireStartItem = hitPort.itemId;
    m_wireStartPort = hitPort.portId;
    m_wirePreviewScene = scenePos;

    if (m_selection) {
        m_selection->clearSelectedPort();
        m_selection->selectItem(hitPort.itemId);
        m_selection->clearMarqueeSelection();
    }
    if (m_drag)
        m_drag->clearTransientState();

    if (m_view)
        m_view->update();
    return true;
}

bool CanvasLinkingController::resolvePortTerminal(const PortRef& port,
                                                  QPointF& outAnchor,
                                                  QPointF& outBorder,
                                                  QPointF& outFabric) const
{
    return m_doc && m_doc->computePortTerminal(port.itemId, port.portId,
                                               outAnchor, outBorder, outFabric);
}

std::unique_ptr<CanvasWire> CanvasLinkingController::buildWire(const PortRef& a, const PortRef& b) const
{
    const CanvasWire::Endpoint start{a, QPointF()};
    const CanvasWire::Endpoint end{b, QPointF()};
    auto wire = std::make_unique<CanvasWire>(start, end);
    if (auto policy = arrowPolicyFromPortRoles(m_doc, a, b))
        wire->setArrowPolicy(*policy);
    return wire;
}

CanvasBlock* CanvasLinkingController::findLinkHub() const
{
    if (!m_doc || m_linkHubId.isNull())
        return nullptr;
    return dynamic_cast<CanvasBlock*>(m_doc->findItem(m_linkHubId));
}

bool CanvasLinkingController::connectToExistingHub(const QPointF& scenePos, const PortRef& hitPort)
{
    if (hitPort.itemId == m_linkHubId) {
        m_wirePreviewScene = scenePos;
        return true;
    }

    CanvasBlock* hub = findLinkHub();
    if (!hub)
        return false;

    QPointF endAnchor, endBorder, endFabric;
    if (!resolvePortTerminal(hitPort, endAnchor, endBorder, endFabric))
        return true;

    const auto finishRole = finishWireRole(m_linkingMode);
    const PortId hubPort = hub->addPortToward(endAnchor, oppositePortRole(finishRole));
    auto w = buildWire(PortRef{hub->id(), hubPort}, hitPort);
    w->setId(m_doc->allocateId());
    const auto finishStyle = Support::linkWireStyle(finishWireRole(m_linkingMode));
    w->setColorOverride(finishStyle.color);
    w->setArrowPolicy(finishRole == Support::LinkWireRole::Consumer
                          ? WireArrowPolicy::End
                          : WireArrowPolicy::None);
    m_doc->commands().execute(std::make_unique<CreateItemCommand>(std::move(w)));
    Support::ensureOppositeProducerPort(*m_doc, hitPort.itemId, hitPort.portId);

    m_wiring = true;
    m_wireStartItem = hub->id();
    m_wireStartPort = hubPort;
    m_wirePreviewScene = scenePos;
    m_view->clearHoveredPort();
    m_view->update();
    return true;
}

bool CanvasLinkingController::createHubAndWires(const QPointF& scenePos, const PortRef& hitPort)
{
    QPointF startAnchor, startBorder, startFabric;
    QPointF endAnchor, endBorder, endFabric;
    if (!resolvePortTerminal(PortRef{m_wireStartItem, m_wireStartPort},
                             startAnchor, startBorder, startFabric) ||
        !resolvePortTerminal(hitPort, endAnchor, endBorder, endFabric)) {
        resetLinkingSession();
        if (m_view)
            m_view->update();
        return true;
    }

    const double size = Constants::kLinkHubSize;
    QPointF hubCenter = (startFabric + endFabric) * 0.5;
    const double step = m_doc->fabric().config().step;
    if (step > 0.0)
        hubCenter = Support::snapPointToGrid(hubCenter, step);

    const QPointF topLeft(hubCenter.x() - size * 0.5, hubCenter.y() - size * 0.5);
    auto hub = std::make_unique<CanvasBlock>(QRectF(topLeft, QSizeF(size, size)), true, QString());
    hub->setShowPorts(false);
    hub->setAutoPortLayout(false);
    hub->setPortSnapStep(Constants::kGridStep);
    hub->setLinkHub(true);
    hub->setKeepoutMargin(0.0);
    hub->setContentPadding(QMarginsF(0.0, 0.0, 0.0, 0.0));
    hub->setId(m_doc->allocateId());

    const Support::LinkHubKind kind =
        (m_linkingMode == CanvasController::LinkingMode::Split) ? Support::LinkHubKind::Split :
        (m_linkingMode == CanvasController::LinkingMode::Join) ? Support::LinkHubKind::Join :
                                                                 Support::LinkHubKind::Broadcast;
    const auto style = Support::linkHubStyle(kind);
    hub->setCustomColors(style.outline, style.fill, style.text);

    SymbolContentStyle symbolStyle;
    symbolStyle.text = style.text;
    auto content = std::make_unique<BlockContentSymbol>(style.symbol, symbolStyle);
    hub->setContent(std::move(content));

    const auto startRole = startWireRole(m_linkingMode);
    const auto finishRole = finishWireRole(m_linkingMode);
    const PortId hubPortA = hub->addPortToward(startAnchor, oppositePortRole(startRole));
    const PortId hubPortB = hub->addPortToward(endAnchor, oppositePortRole(finishRole));
    const ObjectId hubId = hub->id();

    m_doc->commands().execute(std::make_unique<CreateItemCommand>(std::move(hub)));

    auto w0 = buildWire(PortRef{m_wireStartItem, m_wireStartPort}, PortRef{hubId, hubPortA});
    w0->setId(m_doc->allocateId());
    const auto startStyle = Support::linkWireStyle(startRole);
    w0->setColorOverride(startStyle.color);
    w0->setArrowPolicy(startRole == Support::LinkWireRole::Consumer
                           ? WireArrowPolicy::Start
                           : WireArrowPolicy::None);
    m_doc->commands().execute(std::make_unique<CreateItemCommand>(std::move(w0)));

    auto w1 = buildWire(PortRef{hubId, hubPortB}, hitPort);
    w1->setId(m_doc->allocateId());
    const auto finishStyle = Support::linkWireStyle(finishRole);
    w1->setColorOverride(finishStyle.color);
    w1->setArrowPolicy(finishRole == Support::LinkWireRole::Consumer
                           ? WireArrowPolicy::End
                           : WireArrowPolicy::None);
    m_doc->commands().execute(std::make_unique<CreateItemCommand>(std::move(w1)));
    Support::ensureOppositeProducerPort(*m_doc, hitPort.itemId, hitPort.portId);

    m_linkHubId = hubId;
    m_wiring = true;
    m_wireStartItem = hubId;
    m_wireStartPort = hubPortB;
    m_wirePreviewScene = scenePos;
    m_view->clearHoveredPort();
    m_view->update();
    return true;
}

} // namespace Canvas::Controllers
