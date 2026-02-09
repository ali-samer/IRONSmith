#include "canvas/CanvasController.hpp"

#include "canvas/CanvasDocument.hpp"
#include "canvas/CanvasView.hpp"
#include "canvas/CanvasConstants.hpp"
#include "canvas/Tools.hpp"
#include "canvas/CanvasCommands.hpp"
#include "canvas/CanvasWire.hpp"
#include "canvas/utils/CanvasGeometry.hpp"
#include "canvas/utils/CanvasPortUsage.hpp"
#include "canvas/utils/CanvasLinkHubStyle.hpp"
#include "canvas/utils/CanvasLinkWireStyle.hpp"
#include "canvas/CanvasSymbolContent.hpp"
#include "canvas/CanvasRenderContext.hpp"
#include "canvas/utils/CanvasGeometry.hpp"

#include "canvas/CanvasItem.hpp"
#include "canvas/CanvasBlock.hpp"

#include <QtCore/QLineF>
#include <QtCore/QRectF>
#include <QtCore/QSizeF>
#include <QtCore/QString>
#include <algorithm>
#include <cmath>
#include <utility>

namespace {

struct WireEndpointHit final {
    bool isA = true;
    Canvas::CanvasWire::Endpoint endpoint;
};

std::optional<WireEndpointHit> pickWireEndpoint(Canvas::CanvasWire* wire,
                                                const Canvas::CanvasRenderContext& ctx,
                                                const QPointF& scenePos,
                                                double tol);

Canvas::CanvasRenderContext buildRenderContext(const Canvas::CanvasDocument* doc,
                                              const Canvas::CanvasView* view);

struct EndpointCandidate final {
    Canvas::CanvasWire* wire = nullptr;
    WireEndpointHit hit;
};

std::optional<EndpointCandidate> pickEndpointCandidate(Canvas::CanvasDocument* doc,
                                                       const Canvas::CanvasView* view,
                                                       const QPointF& scenePos,
                                                       double tol)
{
    if (!doc || !view)
        return std::nullopt;

    const Canvas::CanvasRenderContext ctx = buildRenderContext(doc, view);
    for (auto it = doc->items().rbegin(); it != doc->items().rend(); ++it) {
        if (!*it)
            continue;
        auto* wire = dynamic_cast<Canvas::CanvasWire*>(it->get());
        if (!wire)
            continue;
        if (auto hit = pickWireEndpoint(wire, ctx, scenePos, tol))
            return EndpointCandidate{wire, *hit};
    }
    return std::nullopt;
}

bool isSpecialLinkingMode(Canvas::CanvasController::LinkingMode mode)
{
    return mode != Canvas::CanvasController::LinkingMode::Normal;
}

QString linkingModeLabel(Canvas::CanvasController::LinkingMode mode)
{
    using Mode = Canvas::CanvasController::LinkingMode;
    switch (mode) {
        case Mode::Split: return QStringLiteral("S");
        case Mode::Join: return QStringLiteral("J");
        case Mode::Broadcast: return QStringLiteral("B");
        case Mode::Normal: break;
    }
    return QString();
}

Canvas::Utils::LinkWireRole startWireRole(Canvas::CanvasController::LinkingMode mode)
{
    return mode == Canvas::CanvasController::LinkingMode::Join
        ? Canvas::Utils::LinkWireRole::Consumer
        : Canvas::Utils::LinkWireRole::Producer;
}

Canvas::Utils::LinkWireRole finishWireRole(Canvas::CanvasController::LinkingMode mode)
{
    return mode == Canvas::CanvasController::LinkingMode::Join
        ? Canvas::Utils::LinkWireRole::Producer
        : Canvas::Utils::LinkWireRole::Consumer;
}

Canvas::PortRole oppositePortRole(Canvas::Utils::LinkWireRole role)
{
    return role == Canvas::Utils::LinkWireRole::Producer ? Canvas::PortRole::Consumer
                                                        : Canvas::PortRole::Producer;
}

std::optional<Canvas::WireArrowPolicy> arrowPolicyFromPortRoles(const Canvas::CanvasDocument* doc,
                                                                const Canvas::PortRef& a,
                                                                const Canvas::PortRef& b)
{
    if (!doc)
        return std::nullopt;

    Canvas::CanvasPort aMeta;
    Canvas::CanvasPort bMeta;
    if (!doc->getPort(a.itemId, a.portId, aMeta) || !doc->getPort(b.itemId, b.portId, bMeta))
        return std::nullopt;

    const bool aConsumer = aMeta.role == Canvas::PortRole::Consumer;
    const bool bConsumer = bMeta.role == Canvas::PortRole::Consumer;

    if (aConsumer && !bConsumer)
        return Canvas::WireArrowPolicy::Start;
    if (bConsumer && !aConsumer)
        return Canvas::WireArrowPolicy::End;
    return std::nullopt;
}

QPointF wheelPanDeltaView(const QPoint& angleDelta, const QPoint& pixelDelta, Qt::KeyboardModifiers mods)
{
    QPoint delta = pixelDelta.isNull() ? angleDelta : pixelDelta;
    if (delta.isNull())
        return {};

    if (mods.testFlag(Qt::ShiftModifier)) {
        if (delta.x() == 0)
            delta.setX(delta.y());
        delta.setY(0);
    }

    return QPointF(delta);
}

Canvas::CanvasRenderContext buildRenderContext(const Canvas::CanvasDocument* doc,
                                              const Canvas::CanvasView* view)
{
    Canvas::CanvasRenderContext ctx;
    if (!doc || !view)
        return ctx;

    ctx.zoom = view->zoom();
    const QPointF tl = view->viewToScene(QPointF(0.0, 0.0));
    const QPointF br = view->viewToScene(QPointF(view->width(), view->height()));
    const double left   = std::min(tl.x(), br.x());
    const double right  = std::max(tl.x(), br.x());
    const double top    = std::min(tl.y(), br.y());
    const double bottom = std::max(tl.y(), br.y());
    ctx.visibleSceneRect = QRectF(QPointF(left, top), QPointF(right, bottom));
    ctx.computePortTerminal = &Canvas::CanvasDocument::computePortTerminalThunk;
    ctx.computePortTerminalUser = const_cast<Canvas::CanvasDocument*>(doc);
    ctx.isFabricBlocked = &Canvas::CanvasDocument::isFabricPointBlockedThunk;
    ctx.isFabricBlockedUser = const_cast<Canvas::CanvasDocument*>(doc);
    ctx.fabricStep = doc->fabric().config().step;
    return ctx;
}

Canvas::CanvasItem* hitTestCanvas(Canvas::CanvasDocument* doc,
                                  const Canvas::CanvasView* view,
                                  const QPointF& scenePos)
{
    if (!doc)
        return nullptr;
    if (!view)
        return doc->hitTest(scenePos);

    const Canvas::CanvasRenderContext ctx = buildRenderContext(doc, view);
    for (auto it = doc->items().rbegin(); it != doc->items().rend(); ++it) {
        Canvas::CanvasItem* item = it->get();
        if (!item)
            continue;
        if (auto* wire = dynamic_cast<Canvas::CanvasWire*>(item)) {
            if (wire->hitTest(scenePos, ctx))
                return item;
            continue;
        }
        if (item->hitTest(scenePos))
            return item;
    }
    return nullptr;
}

Canvas::CanvasBlock* hitTestBlock(Canvas::CanvasDocument* doc, const QPointF& scenePos)
{
    if (!doc)
        return nullptr;

    for (auto it = doc->items().rbegin(); it != doc->items().rend(); ++it) {
        Canvas::CanvasItem* item = it->get();
        if (!item)
            continue;
        if (auto* wire = dynamic_cast<Canvas::CanvasWire*>(item)) {
            Q_UNUSED(wire);
            continue;
        }
        if (auto* block = dynamic_cast<Canvas::CanvasBlock*>(item)) {
            if (block->hitTest(scenePos))
                return block;
        }
    }
    return nullptr;
}

std::optional<Canvas::EdgeCandidate> edgeCandidateAt(Canvas::CanvasDocument* doc,
                                                     Canvas::CanvasView* view,
                                                     const QPointF& scenePos)
{
    if (!doc)
        return std::nullopt;

    const double zoom = view ? view->zoom() : 1.0;
    const double threshold = Canvas::Constants::kPortActivationBandPx / std::max(zoom, 0.25);
    const double step = doc->fabric().config().step;

    for (auto it = doc->items().rbegin(); it != doc->items().rend(); ++it) {
        auto* block = dynamic_cast<Canvas::CanvasBlock*>(it->get());
        if (!block)
            continue;

        const QRectF expanded = block->boundsScene().adjusted(-threshold, -threshold, threshold, threshold);
        if (!expanded.contains(scenePos))
            continue;

        const auto hit = Canvas::Utils::edgeHitForRect(block->boundsScene(), scenePos, threshold, step);
        if (!hit)
            continue;

        Canvas::EdgeCandidate out;
        out.itemId = block->id();
        out.side = hit->side;
        out.t = hit->t;
        out.anchorScene = hit->anchorScene;
        return out;
    }

    return std::nullopt;
}

std::optional<Canvas::PortRef> ensureEdgePort(Canvas::CanvasDocument* doc,
                                              const Canvas::EdgeCandidate& candidate)
{
    if (!doc)
        return std::nullopt;

    auto* block = dynamic_cast<Canvas::CanvasBlock*>(doc->findItem(candidate.itemId));
    if (!block)
        return std::nullopt;

    const double tol = 0.05;
    const double baseT = Canvas::Utils::clampT(candidate.t);

    auto isPortNear = [&](const Canvas::CanvasPort& port, double t) {
        return port.side == candidate.side && std::abs(port.t - t) <= tol;
    };

    for (const auto& port : block->ports()) {
        if (!isPortNear(port, baseT))
            continue;
        if (Canvas::Utils::isPortAvailable(*doc, block->id(), port.id))
            return Canvas::PortRef{block->id(), port.id};
        if (!block->allowMultiplePorts())
            return Canvas::PortRef{block->id(), port.id};
        break;
    }

    double chosenT = baseT;
    if (block->allowMultiplePorts()) {
        const double step = 0.08;
        const double minT = 0.05;
        const double maxT = 0.95;

        auto isFreeT = [&](double t) {
            for (const auto& port : block->ports()) {
                if (isPortNear(port, t))
                    return false;
            }
            return true;
        };

        if (!isFreeT(chosenT)) {
            bool found = false;
            for (int i = 1; i <= 8 && !found; ++i) {
                const double offset = step * i;
                const double tPlus = chosenT + offset;
                if (tPlus <= maxT && isFreeT(tPlus)) {
                    chosenT = tPlus;
                    found = true;
                    break;
                }
                const double tMinus = chosenT - offset;
                if (tMinus >= minT && isFreeT(tMinus)) {
                    chosenT = tMinus;
                    found = true;
                    break;
                }
            }
        }
    }

    const Canvas::PortId portId = block->addPort(candidate.side, chosenT, Canvas::PortRole::Dynamic);
    if (portId.isNull())
        return std::nullopt;

    doc->notifyChanged();
    return Canvas::PortRef{block->id(), portId};
}

bool findPortIndex(const Canvas::CanvasBlock& block, Canvas::PortId portId, size_t& outIndex)
{
    const auto& ports = block.ports();
    for (size_t i = 0; i < ports.size(); ++i) {
        if (ports[i].id == portId) {
            outIndex = i;
            return true;
        }
    }
    return false;
}

Canvas::CanvasWire* hitTestWireEndpoint(Canvas::CanvasDocument* doc,
                                        Canvas::CanvasView* view,
                                        const QPointF& scenePos,
                                        double tol)
{
    if (!doc || !view)
        return nullptr;

    if (auto candidate = pickEndpointCandidate(doc, view, scenePos, tol))
        return candidate->wire;
    return nullptr;
}

std::optional<WireEndpointHit> pickWireEndpoint(Canvas::CanvasWire* wire,
                                               const Canvas::CanvasRenderContext& ctx,
                                               const QPointF& scenePos,
                                               double tol)
{
    if (!wire)
        return std::nullopt;

    auto resolveAnchor = [&](const Canvas::CanvasWire::Endpoint& e) -> QPointF {
        if (e.attached.has_value()) {
            const auto ref = *e.attached;
            QPointF anchor, border, fabric;
            if (ctx.portTerminal(ref.itemId, ref.portId, anchor, border, fabric))
                return anchor;
        }
        return e.freeScene;
    };

    const QPointF aAnchor = resolveAnchor(wire->a());
    const QPointF bAnchor = resolveAnchor(wire->b());

    const double distA = QLineF(scenePos, aAnchor).length();
    const double distB = QLineF(scenePos, bAnchor).length();

    if (distA > tol && distB > tol)
        return std::nullopt;

    WireEndpointHit hit;
    if (distA <= distB) {
        hit.isA = true;
        hit.endpoint = wire->a();
    } else {
        hit.isA = false;
        hit.endpoint = wire->b();
    }
    return hit;
}

int pickWireSegment(const std::vector<QPointF>& path,
                           const QPointF& scenePos,
                           double tol,
                           bool& outHorizontal)
{
    if (path.size() < 2)
        return -1;

    int bestIdx = -1;
    double bestDist = tol;
    for (size_t i = 1; i < path.size(); ++i) {
        const QPointF a = path[i - 1];
        const QPointF b = path[i];
        const QPointF ab = b - a;
        const double len2 = ab.x() * ab.x() + ab.y() * ab.y();
        if (len2 <= 1e-6)
            continue;

        const QPointF ap = scenePos - a;
        double t = (ap.x() * ab.x() + ap.y() * ab.y()) / len2;
        if (t < 0.0) t = 0.0;
        if (t > 1.0) t = 1.0;
        const QPointF proj(a.x() + t * ab.x(), a.y() + t * ab.y());
        const double d = QLineF(scenePos, proj).length();
        if (d <= bestDist) {
            bestDist = d;
            bestIdx = static_cast<int>(i - 1);
            outHorizontal = std::abs(ab.y()) <= std::abs(ab.x());
        }
    }
    return bestIdx;
}

bool isSegmentBlocked(const Canvas::CanvasDocument* doc,
                      bool horizontal,
                      int coord,
                      int spanMin,
                      int spanMax)
{
    if (!doc)
        return false;

    if (spanMin > spanMax)
        std::swap(spanMin, spanMax);

    for (int v = spanMin; v <= spanMax; ++v) {
        const int x = horizontal ? v : coord;
        const int y = horizontal ? coord : v;
        if (doc->isFabricPointBlocked(Canvas::FabricCoord{x, y}))
            return true;
    }
    return false;
}

int adjustSegmentCoord(const Canvas::CanvasDocument* doc,
                       bool horizontal,
                       int desired,
                       int spanMin,
                       int spanMax)
{
    if (!doc)
        return desired;

    if (!isSegmentBlocked(doc, horizontal, desired, spanMin, spanMax))
        return desired;

    for (int dist = 1; dist < 64; ++dist) {
        if (!isSegmentBlocked(doc, horizontal, desired - dist, spanMin, spanMax))
            return desired - dist;
        if (!isSegmentBlocked(doc, horizontal, desired + dist, spanMin, spanMax))
            return desired + dist;
    }
    return desired;
}

} // namespace

namespace Canvas {

CanvasController::CanvasController(CanvasDocument* doc, CanvasView* view, QObject* parent)
	: QObject(parent)
	, m_doc(doc)
	, m_view(view)
{
}

void CanvasController::setMode(Mode mode)
{
	if (m_mode == mode)
		return;

	m_mode = mode;
	if (m_mode != Mode::Linking) {
		if (m_linkingMode != LinkingMode::Normal) {
			m_linkingMode = LinkingMode::Normal;
			emit linkingModeChanged(m_linkingMode);
		}
		resetLinkingSession();
	}
	emit modeChanged(m_mode);
}

void CanvasController::setLinkingMode(LinkingMode mode)
{
	if (m_linkingMode == mode)
		return;
	m_linkingMode = mode;
	emit linkingModeChanged(m_linkingMode);
	resetLinkingSession();
	if (m_view)
		m_view->update();
}

void CanvasController::resetLinkingSession()
{
	m_wiring = false;
	m_wireStartItem = ObjectId{};
	m_wireStartPort = PortId{};
	m_linkHubId = ObjectId{};
	if (m_view)
		m_view->clearHoveredPort();
	if (m_view)
		m_view->clearHoveredEdge();
	m_hoverEdge.reset();
}


QPointF CanvasController::sceneToView(const QPointF& scenePos) const
{
    if (!m_view)
        return {};
    return Tools::sceneToView(scenePos, m_view->pan(), m_view->zoom());
}

void CanvasController::beginPanning(const QPointF& viewPos)
{
    if (!m_view)
        return;

    m_panning = true;
    m_lastViewPos = viewPos;
    m_panStart = m_view->pan();
    m_modeBeforePan = m_mode;
    setMode(Mode::Panning);

    clearTransientDragState();
}

void CanvasController::updatePanning(const QPointF& viewPos)
{
    if (!m_view || !m_panning)
        return;

    const QPointF delta = viewPos - m_lastViewPos;
    const double zoom = m_view->zoom();
    if (zoom <= 0.0)
        return;
    const QPointF deltaScene(delta.x() / zoom, delta.y() / zoom);
    m_view->setPan(m_view->pan() + deltaScene);
    m_lastViewPos = viewPos;
    m_view->update();
}

void CanvasController::endPanning()
{
    if (!m_panning)
        return;

    m_panning = false;
    setMode(m_modeBeforePan);
}

void CanvasController::selectItem(ObjectId id)
{
    if (!id) {
        clearSelection();
        return;
    }
    QSet<ObjectId> next;
    next.insert(id);
    setSelection(next);
}

void CanvasController::selectPort(const PortRef& port)
{
    m_hasSelectedPort = true;
    m_selectedPort = port;
    if (m_view)
        m_view->setSelectedPort(port.itemId, port.portId);
    clearSelection();
}

void CanvasController::clearSelectedPort()
{
    if (!m_hasSelectedPort)
        return;
    m_hasSelectedPort = false;
    m_selectedPort = PortRef{};
    if (m_view)
        m_view->clearSelectedPort();
}

void CanvasController::setSelection(const QSet<ObjectId>& ids)
{
    if (m_selectedItems == ids)
        return;
    m_selectedItems = ids;
    if (m_view)
        m_view->setSelectedItems(m_selectedItems);
    if (!m_selectedItems.isEmpty())
        clearSelectedPort();
}

void CanvasController::clearSelection()
{
    if (m_selectedItems.isEmpty())
        return;
    m_selectedItems.clear();
    if (m_view)
        m_view->clearSelectedItems();
}

void CanvasController::addToSelection(ObjectId id)
{
    if (!id)
        return;
    if (m_selectedItems.contains(id))
        return;
    QSet<ObjectId> next = m_selectedItems;
    next.insert(id);
    setSelection(next);
}

void CanvasController::toggleSelection(ObjectId id)
{
    if (!id)
        return;
    QSet<ObjectId> next = m_selectedItems;
    if (next.contains(id))
        next.remove(id);
    else
        next.insert(id);
    setSelection(next);
}

bool CanvasController::isSelected(ObjectId id) const
{
    return m_selectedItems.contains(id);
}

void CanvasController::clearTransientDragState()
{
    m_dragWire = false;
    m_dragWireId = ObjectId{};
    m_dragWireSeg = -1;
    m_dragWirePath.clear();

    m_dragEndpoint = false;
    m_dragEndpointWireId = ObjectId{};
    m_dragEndpointIsA = false;
    m_dragEndpointOriginal = CanvasWire::Endpoint{};
    m_dragEndpointPortDynamic = false;
    m_dragEndpointPortShared = false;
    m_dragEndpointPort = PortRef{};
    m_dragEndpointPortMeta = CanvasPort{};
    m_dragEndpointPortIndex = 0;
    m_pendingEndpoint = false;
    m_pendingEndpointWireId = ObjectId{};
    m_pendingEndpointPort.reset();
    m_pendingEndpointPressScene = QPointF();
    m_pendingEndpointPressView = QPointF();

    m_dragBlocks.clear();
    m_dragPrimary = nullptr;
    m_dragPrimaryStartTopLeft = QPointF();

    clearMarqueeSelection();
}

bool CanvasController::handleLinkingPress(const QPointF& scenePos)
{
    if (!m_view || !m_doc)
        return false;
    if (m_mode != Mode::Linking)
        return false;

    const double radiusScene = Canvas::Constants::kPortHitRadiusPx / std::max(m_view->zoom(), 0.25);
    std::optional<PortRef> resolvedPort = m_doc->hitTestPort(scenePos, radiusScene);
    if (!resolvedPort) {
        if (auto edge = edgeCandidateAt(m_doc, m_view, scenePos)) {
            resolvedPort = ensureEdgePort(m_doc, *edge);
        }
    }

    if (resolvedPort && !Utils::isPortAvailable(*m_doc, resolvedPort->itemId, resolvedPort->portId)) {
        CanvasPort meta;
        auto* block = dynamic_cast<CanvasBlock*>(m_doc->findItem(resolvedPort->itemId));
        if (block && block->allowMultiplePorts() && m_doc->getPort(resolvedPort->itemId, resolvedPort->portId, meta)) {
            Canvas::EdgeCandidate candidate;
            candidate.itemId = resolvedPort->itemId;
            candidate.side = meta.side;
            candidate.t = meta.t;
            candidate.anchorScene = block->portAnchorScene(meta.id);
            resolvedPort = ensureEdgePort(m_doc, candidate);
        } else {
            return true;
        }
    }

    if (!resolvedPort) {
        if (isSpecialLinkingMode(m_linkingMode)) {
            setLinkingMode(LinkingMode::Normal);
            m_view->update();
            return true;
        }

        if (m_wiring) {
            resetLinkingSession();
            m_view->update();
            return true;
        }

        CanvasItem* hit = hitTestCanvas(m_doc, m_view, scenePos);
        clearSelectedPort();
        selectItem(hit ? hit->id() : ObjectId{});
        return true;
    }

    if (isSpecialLinkingMode(m_linkingMode))
        return handleLinkingHubPress(scenePos, *resolvedPort);

    if (!m_wiring) {
        m_wiring = true;
        m_wireStartItem = resolvedPort->itemId;
        m_wireStartPort = resolvedPort->portId;
        m_wirePreviewScene = scenePos;

        clearSelectedPort();
        selectItem(resolvedPort->itemId);
        clearTransientDragState();

        m_view->update();
        return true;
    }

    if (resolvedPort->itemId == m_wireStartItem && resolvedPort->portId == m_wireStartPort) {
        m_wirePreviewScene = scenePos;
        return true;
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

        m_doc->commands().execute(std::make_unique<CreateItemCommand>(std::move(w)));

        resetLinkingSession();
        m_view->clearHoveredPort();
        m_view->update();
        return true;
    }

    return true;
}

void CanvasController::updateLinkingHoverAndPreview(const QPointF& scenePos)
{
    if (!m_view || !m_doc)
        return;
    if (m_mode != Mode::Linking || m_panning || m_dragEndpoint)
        return;

    const double radiusScene = Canvas::Constants::kPortHitRadiusPx / std::max(m_view->zoom(), 0.25);
    if (auto hitPort = m_doc->hitTestPort(scenePos, radiusScene)) {
        m_view->setHoveredPort(hitPort->itemId, hitPort->portId);
        m_view->clearHoveredEdge();
        m_hoverEdge.reset();
    } else if (auto edge = edgeCandidateAt(m_doc, m_view, scenePos)) {
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

bool CanvasController::handleLinkingHubPress(const QPointF& scenePos, const PortRef& hitPort)
{
    if (!m_view || !m_doc)
        return false;

    if (m_linkHubId.isNull() && !m_wiring)
        return beginLinkingFromPort(hitPort, scenePos);

    if (!m_linkHubId.isNull())
        return connectToExistingHub(scenePos, hitPort);

    if (hitPort.itemId == m_wireStartItem && hitPort.portId == m_wireStartPort) {
        m_wirePreviewScene = scenePos;
        return true;
    }

    return createHubAndWires(scenePos, hitPort);
}

bool CanvasController::beginLinkingFromPort(const PortRef& hitPort, const QPointF& scenePos)
{
    m_wiring = true;
    m_wireStartItem = hitPort.itemId;
    m_wireStartPort = hitPort.portId;
    m_wirePreviewScene = scenePos;

    clearSelectedPort();
    selectItem(hitPort.itemId);
    clearTransientDragState();
    m_view->update();
    return true;
}

bool CanvasController::resolvePortTerminal(const PortRef& port,
                                           QPointF& outAnchor,
                                           QPointF& outBorder,
                                           QPointF& outFabric) const
{
    return m_doc && m_doc->computePortTerminal(port.itemId, port.portId,
                                               outAnchor, outBorder, outFabric);
}

std::unique_ptr<CanvasWire> CanvasController::buildWire(const PortRef& a, const PortRef& b) const
{
    const CanvasWire::Endpoint start{a, QPointF()};
    const CanvasWire::Endpoint end{b, QPointF()};
    auto wire = std::make_unique<CanvasWire>(start, end);
    if (auto policy = arrowPolicyFromPortRoles(m_doc, a, b))
        wire->setArrowPolicy(*policy);
    return wire;
}

CanvasBlock* CanvasController::findLinkHub() const
{
    if (!m_doc || m_linkHubId.isNull())
        return nullptr;
    return dynamic_cast<CanvasBlock*>(m_doc->findItem(m_linkHubId));
}

bool CanvasController::connectToExistingHub(const QPointF& scenePos, const PortRef& hitPort)
{
    if (hitPort.itemId == m_linkHubId) {
        m_wirePreviewScene = scenePos;
        return true;
    }

    CanvasBlock* hub = findLinkHub();
    if (!hub) {
        setLinkingMode(LinkingMode::Normal);
        m_view->update();
        return true;
    }

    QPointF endAnchor, endBorder, endFabric;
    if (!resolvePortTerminal(hitPort, endAnchor, endBorder, endFabric))
        return true;

    const auto finishRole = finishWireRole(m_linkingMode);
    const PortId hubPort = hub->addPortToward(endAnchor, oppositePortRole(finishRole));
    auto w = buildWire(PortRef{hub->id(), hubPort}, hitPort);
    w->setId(m_doc->allocateId());
    const auto finishStyle = Utils::linkWireStyle(finishWireRole(m_linkingMode));
    w->setColorOverride(finishStyle.color);
    w->setArrowPolicy(finishRole == Utils::LinkWireRole::Consumer
                          ? WireArrowPolicy::End
                          : WireArrowPolicy::None);
    m_doc->commands().execute(std::make_unique<CreateItemCommand>(std::move(w)));

    m_wiring = true;
    m_wireStartItem = hub->id();
    m_wireStartPort = hubPort;
    m_wirePreviewScene = scenePos;
    m_view->clearHoveredPort();
    m_view->update();
    return true;
}

bool CanvasController::createHubAndWires(const QPointF& scenePos, const PortRef& hitPort)
{
    QPointF startAnchor, startBorder, startFabric;
    QPointF endAnchor, endBorder, endFabric;
    if (!resolvePortTerminal(PortRef{m_wireStartItem, m_wireStartPort},
                             startAnchor, startBorder, startFabric) ||
        !resolvePortTerminal(hitPort, endAnchor, endBorder, endFabric)) {
        resetLinkingSession();
        m_view->update();
        return true;
    }

    const double size = Constants::kLinkHubSize;
    QPointF hubCenter = (startFabric + endFabric) * 0.5;
    const double step = m_doc->fabric().config().step;
    if (step > 0.0)
        hubCenter = Utils::snapPointToGrid(hubCenter, step);

    const QPointF topLeft(hubCenter.x() - size * 0.5, hubCenter.y() - size * 0.5);
    auto hub = std::make_unique<CanvasBlock>(QRectF(topLeft, QSizeF(size, size)), true, QString());
    hub->setShowPorts(false);
    hub->setAutoPortLayout(true);
    hub->setPortSnapStep(Constants::kGridStep);
    hub->setLinkHub(true);
    hub->setKeepoutMargin(0.0);
    hub->setContentPadding(QMarginsF(0.0, 0.0, 0.0, 0.0));
    hub->setId(m_doc->allocateId());

    const Utils::LinkHubKind kind =
        (m_linkingMode == LinkingMode::Split) ? Utils::LinkHubKind::Split :
        (m_linkingMode == LinkingMode::Join) ? Utils::LinkHubKind::Join :
                                               Utils::LinkHubKind::Broadcast;
    const auto style = Utils::linkHubStyle(kind);
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
    const auto startStyle = Utils::linkWireStyle(startRole);
    w0->setColorOverride(startStyle.color);
    w0->setArrowPolicy(startRole == Utils::LinkWireRole::Consumer
                           ? WireArrowPolicy::Start
                           : WireArrowPolicy::None);
    m_doc->commands().execute(std::make_unique<CreateItemCommand>(std::move(w0)));

    auto w1 = buildWire(PortRef{hubId, hubPortB}, hitPort);
    w1->setId(m_doc->allocateId());
    const auto finishStyle = Utils::linkWireStyle(finishRole);
    w1->setColorOverride(finishStyle.color);
    w1->setArrowPolicy(finishRole == Utils::LinkWireRole::Consumer
                           ? WireArrowPolicy::End
                           : WireArrowPolicy::None);
    m_doc->commands().execute(std::make_unique<CreateItemCommand>(std::move(w1)));

    m_linkHubId = hubId;
    m_wiring = true;
    m_wireStartItem = hubId;
    m_wireStartPort = hubPortB;
    m_wirePreviewScene = scenePos;
    m_view->clearHoveredPort();
    m_view->update();
    return true;
}

void CanvasController::beginWireSegmentDrag(CanvasWire* wire, const QPointF& scenePos)
{
    if (!m_view || !m_doc || !wire)
        return;

    const CanvasRenderContext ctx = buildRenderContext(m_doc, m_view);
    const std::vector<QPointF> path = wire->resolvedPathScene(ctx);

    bool horizontal = false;
    const double tol = 6.0 / m_view->zoom();
    const int seg = pickWireSegment(path, scenePos, tol, horizontal);
    if (seg < 0)
        return;

    m_dragWire = true;
    m_dragWireId = wire->id();
    m_dragWireSeg = seg;
    m_dragWireSegHorizontal = horizontal;

    const double axisCoord = horizontal ? path[seg].y() : path[seg].x();
    m_dragWireOffset = horizontal ? (scenePos.y() - axisCoord) : (scenePos.x() - axisCoord);
    m_dragWirePath = wire->resolvedPathCoords(ctx);

    m_dragBlocks.clear();
    m_dragPrimary = nullptr;
}

void CanvasController::updateWireSegmentDrag(const QPointF& scenePos)
{
    if (!m_dragWire || !m_doc || !m_view)
        return;

    const double step = m_doc->fabric().config().step;
    if (step <= 0.0 || m_dragWireSeg < 0 || m_dragWireSeg + 1 >= static_cast<int>(m_dragWirePath.size()))
        return;

    const double raw = (m_dragWireSegHorizontal ? scenePos.y() : scenePos.x()) - m_dragWireOffset;
    int newCoord = static_cast<int>(std::llround(raw / step));

    int spanMin = 0;
    int spanMax = 0;
    if (m_dragWireSegHorizontal) {
        spanMin = std::min(m_dragWirePath[m_dragWireSeg].x, m_dragWirePath[m_dragWireSeg + 1].x);
        spanMax = std::max(m_dragWirePath[m_dragWireSeg].x, m_dragWirePath[m_dragWireSeg + 1].x);
    } else {
        spanMin = std::min(m_dragWirePath[m_dragWireSeg].y, m_dragWirePath[m_dragWireSeg + 1].y);
        spanMax = std::max(m_dragWirePath[m_dragWireSeg].y, m_dragWirePath[m_dragWireSeg + 1].y);
    }

    newCoord = adjustSegmentCoord(m_doc, m_dragWireSegHorizontal, newCoord, spanMin, spanMax);

    auto next = m_dragWirePath;
    if (m_dragWireSegHorizontal) {
        next[m_dragWireSeg].y = newCoord;
        next[m_dragWireSeg + 1].y = newCoord;
    } else {
        next[m_dragWireSeg].x = newCoord;
        next[m_dragWireSeg + 1].x = newCoord;
    }

    for (auto& it : m_doc->items()) {
        if (it && it->id() == m_dragWireId) {
            if (auto* w = dynamic_cast<CanvasWire*>(it.get())) {
                w->setRouteOverride(std::move(next));
                m_dragWirePath = w->routeOverride();
                m_view->update();
            }
            break;
        }
    }
}

void CanvasController::endWireSegmentDrag(const QPointF&)
{
    m_dragWire = false;
    m_dragWireId = ObjectId{};
    m_dragWireSeg = -1;
    m_dragWirePath.clear();
}

bool CanvasController::beginEndpointDrag(CanvasWire* wire, const QPointF& scenePos)
{
    if (!m_view || !m_doc || !wire)
        return false;

    const CanvasRenderContext ctx = buildRenderContext(m_doc, m_view);
    const double tol = Canvas::Constants::kEndpointHitRadiusPx / std::max(m_view->zoom(), 0.25);
    const auto hit = pickWireEndpoint(wire, ctx, scenePos, tol);
    if (!hit)
        return false;

    m_dragEndpoint = true;
    m_dragEndpointWireId = wire->id();
    m_dragEndpointIsA = hit->isA;
    m_dragEndpointOriginal = hit->endpoint;
    m_dragEndpointPortDynamic = false;
    m_dragEndpointPortShared = false;
    m_dragEndpointPort = PortRef{};
    m_dragEndpointPortMeta = CanvasPort{};
    m_dragEndpointPortIndex = 0;

    if (hit->endpoint.attached.has_value()) {
        const PortRef ref = *hit->endpoint.attached;
        CanvasPort meta;
        if (m_doc->getPort(ref.itemId, ref.portId, meta)) {
            m_dragEndpointPort = ref;
            m_dragEndpointPortMeta = meta;
            m_dragEndpointPortDynamic = (meta.role == PortRole::Dynamic);
            m_dragEndpointPortShared = Utils::countPortAttachments(*m_doc, ref.itemId, ref.portId, wire->id()) > 0;

            if (auto* block = dynamic_cast<CanvasBlock*>(m_doc->findItem(ref.itemId))) {
                findPortIndex(*block, ref.portId, m_dragEndpointPortIndex);
            }
        }
    }

    CanvasWire::Endpoint next = hit->endpoint;
    next.attached.reset();
    const double step = m_doc->fabric().config().step;
    next.freeScene = Canvas::Utils::snapPointToGrid(scenePos, step);

    if (hit->isA)
        wire->setEndpointA(next);
    else
        wire->setEndpointB(next);

    wire->clearRouteOverride();
    m_doc->notifyChanged();
    m_view->update();
    return true;
}

void CanvasController::updateEndpointDrag(const QPointF& scenePos)
{
    if (!m_dragEndpoint || !m_doc || !m_view)
        return;

    CanvasWire* wire = nullptr;
    for (const auto& it : m_doc->items()) {
        if (it && it->id() == m_dragEndpointWireId) {
            wire = dynamic_cast<CanvasWire*>(it.get());
            break;
        }
    }
    if (!wire)
        return;

    CanvasWire::Endpoint next = m_dragEndpointIsA ? wire->a() : wire->b();
    next.attached.reset();
    const double step = m_doc->fabric().config().step;
    next.freeScene = Canvas::Utils::snapPointToGrid(scenePos, step);

    if (m_dragEndpointIsA)
        wire->setEndpointA(next);
    else
        wire->setEndpointB(next);

    wire->clearRouteOverride();
    if (auto edge = edgeCandidateAt(m_doc, m_view, scenePos))
        m_view->setHoveredEdge(edge->itemId, edge->side, edge->anchorScene);
    else
        m_view->clearHoveredEdge();
    m_view->update();
}

void CanvasController::endEndpointDrag(const QPointF& scenePos)
{
    if (!m_dragEndpoint || !m_doc || !m_view)
        return;

    CanvasWire* wire = nullptr;
    for (const auto& it : m_doc->items()) {
        if (it && it->id() == m_dragEndpointWireId) {
            wire = dynamic_cast<CanvasWire*>(it.get());
            break;
        }
    }
    if (!wire)
        return;

    const double radiusScene = Canvas::Constants::kPortHitRadiusPx / std::max(m_view->zoom(), 0.25);
    std::optional<PortRef> target = m_doc->hitTestPort(scenePos, radiusScene);
    std::optional<EdgeCandidate> edge = std::nullopt;
    if (!target)
        edge = edgeCandidateAt(m_doc, m_view, scenePos);

    bool attached = false;
    bool movedPort = false;

    if (target && !Utils::isPortAvailable(*m_doc, target->itemId, target->portId, wire->id())) {
        target.reset();
    }

    if (target) {
        CanvasWire::Endpoint next;
        next.attached = *target;
        next.freeScene = scenePos;
        if (m_dragEndpointIsA)
            wire->setEndpointA(next);
        else
            wire->setEndpointB(next);
        attached = true;
    } else if (edge.has_value()) {
        auto* targetBlock = dynamic_cast<CanvasBlock*>(m_doc->findItem(edge->itemId));
        if (targetBlock) {
            if (m_dragEndpointPortDynamic && !m_dragEndpointPortShared && m_dragEndpointPort.itemId) {
                CanvasPort moved = m_dragEndpointPortMeta;
                moved.side = edge->side;
                moved.t = Canvas::Utils::clampT(edge->t);

                auto* sourceBlock = dynamic_cast<CanvasBlock*>(m_doc->findItem(m_dragEndpointPort.itemId));
                if (sourceBlock) {
                    if (sourceBlock->id() == targetBlock->id()) {
                        sourceBlock->updatePort(m_dragEndpointPort.portId, moved.side, moved.t);
                        CanvasWire::Endpoint next;
                        next.attached = PortRef{sourceBlock->id(), m_dragEndpointPort.portId};
                        next.freeScene = scenePos;
                        if (m_dragEndpointIsA)
                            wire->setEndpointA(next);
                        else
                            wire->setEndpointB(next);
                        attached = true;
                        movedPort = true;
                    } else {
                        const bool removed = sourceBlock->removePort(m_dragEndpointPort.portId).has_value();
                        if (removed) {
                            targetBlock->insertPort(targetBlock->ports().size(), moved);
                            CanvasWire::Endpoint next;
                            next.attached = PortRef{targetBlock->id(), m_dragEndpointPort.portId};
                            next.freeScene = scenePos;
                            if (m_dragEndpointIsA)
                                wire->setEndpointA(next);
                            else
                                wire->setEndpointB(next);
                            attached = true;
                            movedPort = true;
                        }
                    }
                }
            }

            if (!attached) {
                const PortId newPort = targetBlock->addPort(edge->side, edge->t, PortRole::Dynamic);
                if (newPort) {
                    CanvasWire::Endpoint next;
                    next.attached = PortRef{targetBlock->id(), newPort};
                    next.freeScene = scenePos;
                    if (m_dragEndpointIsA)
                        wire->setEndpointA(next);
                    else
                        wire->setEndpointB(next);
                    attached = true;
                }
            }
        }
    }

    if (!attached) {
        if (m_dragEndpointIsA)
            wire->setEndpointA(m_dragEndpointOriginal);
        else
            wire->setEndpointB(m_dragEndpointOriginal);
        } else if (!movedPort && m_dragEndpointPortDynamic && m_dragEndpointPort.itemId) {
        if (Utils::countPortAttachments(*m_doc, m_dragEndpointPort.itemId, m_dragEndpointPort.portId, wire->id()) == 0) {
            if (auto* block = dynamic_cast<CanvasBlock*>(m_doc->findItem(m_dragEndpointPort.itemId))) {
                block->removePort(m_dragEndpointPort.portId);
            }
        }
    }

    wire->clearRouteOverride();
    m_doc->notifyChanged();
    m_view->clearHoveredEdge();

    m_dragEndpoint = false;
    m_dragEndpointWireId = ObjectId{};
    m_dragEndpointIsA = false;
    m_dragEndpointOriginal = CanvasWire::Endpoint{};
    m_dragEndpointPortDynamic = false;
    m_dragEndpointPortShared = false;
    m_dragEndpointPort = PortRef{};
    m_dragEndpointPortMeta = CanvasPort{};
    m_dragEndpointPortIndex = 0;
    m_view->update();
}

void CanvasController::beginBlockDrag(CanvasBlock* blk, const QPointF& scenePos)
{
    if (!blk)
        return;

    m_dragBlocks.clear();
    m_dragPrimary = blk;
    m_dragPrimaryStartTopLeft = blk->boundsScene().topLeft();
    m_dragOffset = scenePos - m_dragPrimaryStartTopLeft;

    const bool useSelection = isSelected(blk->id()) && m_selectedItems.size() > 1;
    if (useSelection && m_doc) {
        for (const auto& id : m_selectedItems) {
            auto* item = m_doc->findItem(id);
            auto* block = dynamic_cast<CanvasBlock*>(item);
            if (!block || !block->isMovable())
                continue;
            m_dragBlocks.push_back(DragBlockState{block, block->boundsScene().topLeft()});
        }
    }

    if (m_dragBlocks.empty() && blk->isMovable())
        m_dragBlocks.push_back(DragBlockState{blk, blk->boundsScene().topLeft()});
}

void CanvasController::updateBlockDrag(const QPointF& scenePos)
{
    if (!m_dragPrimary || m_dragBlocks.empty() || !m_view || !m_doc)
        return;

    const QPointF newTopLeft = scenePos - m_dragOffset;
    const double step = m_doc->fabric().config().step;

    const QPointF snappedPrimary = Utils::snapPointToGrid(newTopLeft, step);
    const QPointF delta = snappedPrimary - m_dragPrimaryStartTopLeft;

    for (auto& state : m_dragBlocks) {
        if (!state.block)
            continue;
        QRectF newBounds = state.block->boundsScene();
        newBounds.moveTopLeft(state.startTopLeft + delta);
        state.block->setBoundsScene(newBounds);
    }

    m_view->update();
}

void CanvasController::endBlockDrag()
{
    if (!m_doc || m_dragBlocks.empty()) {
        m_dragBlocks.clear();
        m_dragPrimary = nullptr;
        return;
    }

    auto batch = std::make_unique<CompositeCommand>(QStringLiteral("Move Items"));
    for (const auto& state : m_dragBlocks) {
        if (!state.block)
            continue;
        const QPointF endTopLeft = state.block->boundsScene().topLeft();
        if (endTopLeft == state.startTopLeft)
            continue;
        batch->add(std::make_unique<MoveItemCommand>(state.block->id(), state.startTopLeft, endTopLeft));
    }
    if (!batch->empty())
        m_doc->commands().execute(std::move(batch));

    m_dragBlocks.clear();
    m_dragPrimary = nullptr;
}

void CanvasController::beginMarqueeSelection(const QPointF& scenePos, Qt::KeyboardModifiers mods)
{
    if (!m_view || !m_doc)
        return;

    m_marqueeActive = true;
    m_marqueeStartScene = scenePos;
    m_marqueeStartView = sceneToView(scenePos);
    m_marqueeRectScene = QRectF(scenePos, scenePos);
    m_marqueeMods = mods;
    m_marqueeBaseSelection = m_selectedItems;

    if (!mods.testFlag(Qt::ShiftModifier) && !mods.testFlag(Qt::ControlModifier))
        m_marqueeBaseSelection.clear();

    clearSelectedPort();
    m_view->setMarqueeRect(m_marqueeRectScene);
    updateMarqueeSelection(scenePos);
}

void CanvasController::updateMarqueeSelection(const QPointF& scenePos)
{
    if (!m_marqueeActive || !m_view || !m_doc)
        return;

    m_marqueeRectScene = QRectF(m_marqueeStartScene, scenePos).normalized();
    m_view->setMarqueeRect(m_marqueeRectScene);

    const QSet<ObjectId> hits = collectItemsInRect(m_marqueeRectScene);
    QSet<ObjectId> next = m_marqueeBaseSelection;

    if (m_marqueeMods.testFlag(Qt::ControlModifier)) {
        for (const auto& id : hits) {
            if (next.contains(id))
                next.remove(id);
            else
                next.insert(id);
        }
    } else if (m_marqueeMods.testFlag(Qt::ShiftModifier)) {
        next.unite(hits);
    } else {
        next = hits;
    }

    setSelection(next);
}

void CanvasController::endMarqueeSelection(const QPointF& scenePos)
{
    if (!m_marqueeActive)
        return;

    const QPointF endView = sceneToView(scenePos);
    const double dist = QLineF(m_marqueeStartView, endView).length();
    if (dist < Canvas::Constants::kMarqueeDragThresholdPx) {
        if (m_marqueeMods.testFlag(Qt::ShiftModifier) || m_marqueeMods.testFlag(Qt::ControlModifier)) {
            setSelection(m_marqueeBaseSelection);
        } else {
            clearSelection();
        }
    } else {
        updateMarqueeSelection(scenePos);
    }

    m_marqueeActive = false;
    if (m_view)
        m_view->clearMarqueeRect();
}

void CanvasController::clearMarqueeSelection()
{
    if (!m_marqueeActive)
        return;
    m_marqueeActive = false;
    m_marqueeRectScene = QRectF();
    if (m_view)
        m_view->clearMarqueeRect();
}

QSet<ObjectId> CanvasController::collectItemsInRect(const QRectF& sceneRect) const
{
    QSet<ObjectId> ids;
    if (!m_doc)
        return ids;

    const QRectF rect = sceneRect.normalized();
    for (const auto& it : m_doc->items()) {
        if (!it)
            continue;
        if (rect.intersects(it->boundsScene()))
            ids.insert(it->id());
    }
    return ids;
}


void CanvasController::onCanvasMousePressed(const QPointF& scenePos, Qt::MouseButtons buttons, Qt::KeyboardModifiers mods)
{
    if (!m_view)
        return;

    const QPointF viewPos = sceneToView(scenePos);

    if (buttons.testFlag(Qt::MiddleButton)) {
        beginPanning(viewPos);
        return;
    }
    if (m_mode == Mode::Panning && buttons.testFlag(Qt::LeftButton)) {
        beginPanning(viewPos);
        return;
    }

    if (!buttons.testFlag(Qt::LeftButton) || !m_doc)
        return;

    const double tol = Canvas::Constants::kEndpointHitRadiusPx / std::max(m_view->zoom(), 0.25);
    if (auto candidate = pickEndpointCandidate(m_doc, m_view, scenePos, tol)) {
        m_pendingEndpoint = true;
        m_pendingEndpointWireId = candidate->wire->id();
        m_pendingEndpointPort = candidate->hit.endpoint.attached;
        m_pendingEndpointPressScene = scenePos;
        m_pendingEndpointPressView = viewPos;
        return;
    }

    if (m_mode == Mode::Normal) {
        const double radiusScene = Canvas::Constants::kPortHitRadiusPx / std::max(m_view->zoom(), 0.25);
        if (auto hitPort = m_doc->hitTestPort(scenePos, radiusScene)) {
            selectPort(*hitPort);
            return;
        }
    }

    if (handleLinkingPress(scenePos))
        return;

    CanvasItem* hit = hitTestCanvas(m_doc, m_view, scenePos);

    if (m_mode == Mode::Normal && !hit) {
        beginMarqueeSelection(scenePos, mods);
        return;
    }

    if (hit) {
        clearSelectedPort();
        if (mods.testFlag(Qt::ControlModifier))
            toggleSelection(hit->id());
        else if (mods.testFlag(Qt::ShiftModifier))
            addToSelection(hit->id());
        else
            selectItem(hit->id());
    } else {
        if (!mods.testFlag(Qt::ControlModifier) && !mods.testFlag(Qt::ShiftModifier))
            clearSelection();
    }

    if (auto* wire = dynamic_cast<CanvasWire*>(hit)) {
        if (!mods.testFlag(Qt::ControlModifier) && !mods.testFlag(Qt::ShiftModifier)) {
            beginWireSegmentDrag(wire, scenePos);
            if (m_dragWire)
                return;
        }
    }

    auto* blk = dynamic_cast<CanvasBlock*>(hit);
    if (blk && blk->isMovable() && !mods.testFlag(Qt::ControlModifier) && !mods.testFlag(Qt::ShiftModifier))
        beginBlockDrag(blk, scenePos);
}

void CanvasController::onCanvasMouseMoved(const QPointF& scenePos, Qt::MouseButtons buttons, Qt::KeyboardModifiers mods)
{
    Q_UNUSED(mods);

    if (!m_view)
        return;

    if (m_pendingEndpoint && buttons.testFlag(Qt::LeftButton)) {
        const QPointF viewPos = sceneToView(scenePos);
        const double dist = QLineF(viewPos, m_pendingEndpointPressView).length();
        if (dist >= Canvas::Constants::kEndpointDragThresholdPx) {
            CanvasWire* wire = nullptr;
            if (m_doc) {
                for (const auto& it : m_doc->items()) {
                    if (it && it->id() == m_pendingEndpointWireId) {
                        wire = dynamic_cast<CanvasWire*>(it.get());
                        break;
                    }
                }
            }
            if (wire && beginEndpointDrag(wire, m_pendingEndpointPressScene)) {
                m_pendingEndpoint = false;
                m_pendingEndpointWireId = ObjectId{};
                m_pendingEndpointPort.reset();
                updateEndpointDrag(scenePos);
                return;
            }
        } else {
            return;
        }
    }

    if (m_marqueeActive && buttons.testFlag(Qt::LeftButton)) {
        updateMarqueeSelection(scenePos);
        return;
    }

    updateLinkingHoverAndPreview(scenePos);

    if (m_panning) {
        const bool allowPan = buttons.testFlag(Qt::MiddleButton) ||
                              (m_mode == Mode::Panning && buttons.testFlag(Qt::LeftButton));
        if (allowPan)
            updatePanning(sceneToView(scenePos));
        else
            endPanning();
        return;
    }

    if (m_dragWire) {
        updateWireSegmentDrag(scenePos);
        return;
    }

    if (m_dragEndpoint) {
        updateEndpointDrag(scenePos);
        return;
    }

    if (m_mode == Mode::Linking)
        return;

    if (!m_dragBlocks.empty() && buttons.testFlag(Qt::LeftButton))
        updateBlockDrag(scenePos);
}

void CanvasController::onCanvasMouseReleased(const QPointF& scenePos, Qt::MouseButtons buttons, Qt::KeyboardModifiers mods)
{
    Q_UNUSED(mods);

    if (!m_view)
        return;

    if (m_panning) {
        const bool allowPan = buttons.testFlag(Qt::MiddleButton) ||
                              (m_mode == Mode::Panning && buttons.testFlag(Qt::LeftButton));
        if (!allowPan)
            endPanning();
    }

    if (!m_doc) {
        m_wiring = false;
        clearTransientDragState();
        return;
    }

    if (m_pendingEndpoint) {
        if (m_mode == Mode::Normal) {
            if (m_pendingEndpointPort.has_value())
                selectPort(*m_pendingEndpointPort);
            else
                clearSelectedPort();
        } else if (m_mode == Mode::Linking) {
            handleLinkingPress(scenePos);
        }
        m_pendingEndpoint = false;
        m_pendingEndpointWireId = ObjectId{};
        m_pendingEndpointPort.reset();
        return;
    }

    if (m_marqueeActive) {
        endMarqueeSelection(scenePos);
        return;
    }

    if (m_dragWire) {
        endWireSegmentDrag(scenePos);
        return;
    }

    if (m_dragEndpoint) {
        endEndpointDrag(scenePos);
        return;
    }

    if (!m_dragBlocks.empty())
        endBlockDrag();
}


void CanvasController::onCanvasWheel(const QPointF& scenePos, const QPoint& angleDelta, const QPoint& pixelDelta, Qt::KeyboardModifiers mods)
{
	if (!m_view)
		return;

	if (mods.testFlag(Qt::ControlModifier)) {
		int dy = angleDelta.y();
		if (dy == 0)
			dy = pixelDelta.y();
		if (dy == 0)
			return;

		const double factor = dy > 0 ? Constants::kZoomStep : (1.0 / Constants::kZoomStep);

		const double oldZoom = m_view->zoom();
		const QPointF oldPan = m_view->pan();

		const double newZoom = Tools::clampZoom(oldZoom * factor);
		m_view->setZoom(newZoom);

		const QPointF panNew = ((scenePos + oldPan) * oldZoom / newZoom) - scenePos;
		m_view->setPan(panNew);
		return;
	}

	const QPointF deltaView = wheelPanDeltaView(angleDelta, pixelDelta, mods);
	if (deltaView.isNull())
		return;

	const double zoom = m_view->zoom();
	if (zoom <= 0.0)
		return;

	m_view->setPan(m_view->pan() + QPointF(deltaView.x() / zoom, deltaView.y() / zoom));
}

void CanvasController::onCanvasKeyPressed(int key, Qt::KeyboardModifiers mods)
{
    if (!m_doc)
        return;

    if (key == Qt::Key_Escape) {
		if (m_panning) {
			m_modeBeforePan = Mode::Normal;
			return;
		}
		if (m_mode == Mode::Linking && isSpecialLinkingMode(m_linkingMode)) {
			setLinkingMode(LinkingMode::Normal);
			return;
		}
		setMode(Mode::Normal);
		return;
	}

	if (mods.testFlag(Qt::ControlModifier) && mods.testFlag(Qt::ShiftModifier) && key == Qt::Key_L) {
		if (m_panning) {
			m_modeBeforePan = Mode::Linking;
			return;
		}
		setMode(Mode::Linking);
		return;
	}

    if (mods.testFlag(Qt::ControlModifier)) {
		if (m_mode == Mode::Linking) {
			if (key == Qt::Key_S) {
				setLinkingMode(LinkingMode::Split);
				return;
			}
			if (key == Qt::Key_J) {
				setLinkingMode(LinkingMode::Join);
				return;
			}
			if (key == Qt::Key_B) {
				setLinkingMode(LinkingMode::Broadcast);
				return;
			}
		}
        if (key == Qt::Key_Z) {
            if (mods.testFlag(Qt::ShiftModifier))
                m_doc->commands().redo();
            else
                m_doc->commands().undo();
            return;
        }
        if (key == Qt::Key_Y) {
            m_doc->commands().redo();
            return;
        }
    }

    if (key == Qt::Key_Delete || key == Qt::Key_Backspace) {
        if (m_hasSelectedPort) {
            if (m_doc->commands().execute(std::make_unique<DeletePortCommand>(m_selectedPort.itemId,
                                                                             m_selectedPort.portId))) {
                clearSelectedPort();
            }
            return;
        }
        if (m_selectedItems.isEmpty())
            return;

        QSet<ObjectId> deletion = m_selectedItems;
        for (const auto& id : m_selectedItems) {
            auto* item = m_doc->findItem(id);
            if (!item) {
                deletion.remove(id);
                continue;
            }
            if (auto* block = dynamic_cast<CanvasBlock*>(item)) {
                if (!block->isDeletable()) {
                    deletion.remove(id);
                    continue;
                }
                if (block->isLinkHub()) {
                    for (const auto& it : m_doc->items()) {
                        auto* wire = dynamic_cast<CanvasWire*>(it.get());
                        if (wire && wire->attachesTo(id))
                            deletion.remove(wire->id());
                    }
                }
            }
        }

        if (deletion.isEmpty())
            return;

        std::vector<ObjectId> ordered;
        ordered.reserve(deletion.size());
        for (const auto& id : deletion)
            ordered.push_back(id);
        std::sort(ordered.begin(), ordered.end());

        auto batch = std::make_unique<CompositeCommand>(QStringLiteral("Delete Items"));
        for (const auto& id : ordered)
            batch->add(std::make_unique<DeleteItemCommand>(id));

        if (!batch->empty() && m_doc->commands().execute(std::move(batch)))
            clearSelection();
    }
}

} // namespace Canvas
