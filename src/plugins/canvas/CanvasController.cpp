#include "canvas/CanvasController.hpp"

#include "canvas/CanvasDocument.hpp"
#include "canvas/CanvasView.hpp"
#include "canvas/CanvasConstants.hpp"
#include "canvas/Tools.hpp"
#include "canvas/CanvasCommands.hpp"
#include "canvas/CanvasWire.hpp"
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
    m_selected = id;
    if (m_view)
        m_view->setSelectedItem(m_selected);
}

void CanvasController::clearTransientDragState()
{
    m_dragWire = false;
    m_dragWireId = ObjectId{};
    m_dragWireSeg = -1;
    m_dragWirePath.clear();

    m_dragBlock = nullptr;
}

bool CanvasController::handleLinkingPress(const QPointF& scenePos)
{
    if (!m_view || !m_doc)
        return false;
    if (m_mode != Mode::Linking)
        return false;

    const double radiusScene = 6.0 / m_view->zoom();
    const auto hitPort = m_doc->hitTestPort(scenePos, radiusScene);
    if (!hitPort) {
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
        selectItem(hit ? hit->id() : ObjectId{});
        return true;
    }

    if (isSpecialLinkingMode(m_linkingMode))
        return handleLinkingHubPress(scenePos, *hitPort);

    if (!m_wiring) {
        m_wiring = true;
        m_wireStartItem = hitPort->itemId;
        m_wireStartPort = hitPort->portId;
        m_wirePreviewScene = scenePos;

        selectItem(hitPort->itemId);
        clearTransientDragState();

        m_view->update();
        return true;
    }

    if (hitPort->itemId == m_wireStartItem && hitPort->portId == m_wireStartPort) {
        m_wirePreviewScene = scenePos;
        return true;
    }

    CanvasPort startMeta;
    CanvasPort endMeta;
    if (m_doc->getPort(m_wireStartItem, m_wireStartPort, startMeta) &&
        m_doc->getPort(hitPort->itemId, hitPort->portId, endMeta))
    {
        const CanvasWire::Endpoint a{PortRef{m_wireStartItem, m_wireStartPort}, QPointF()};
        const CanvasWire::Endpoint b{PortRef{hitPort->itemId, hitPort->portId}, QPointF()};

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
    if (m_mode != Mode::Linking || m_panning)
        return;

    const double radiusScene = 6.0 / m_view->zoom();
    if (auto hitPort = m_doc->hitTestPort(scenePos, radiusScene))
        m_view->setHoveredPort(hitPort->itemId, hitPort->portId);
    else
        m_view->clearHoveredPort();

    if (m_wiring && m_wirePreviewScene != scenePos) {
        m_wirePreviewScene = scenePos;
        m_view->update();
    }
}

bool CanvasController::handleLinkingHubPress(const QPointF& scenePos, const PortRef& hitPort)
{
    if (!m_view || !m_doc)
        return false;

    if (!m_linkHubId.isValid() && !m_wiring)
        return beginLinkingFromPort(hitPort, scenePos);

    if (m_linkHubId.isValid())
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
    return std::make_unique<CanvasWire>(start, end);
}

CanvasBlock* CanvasController::findLinkHub() const
{
    if (!m_doc || !m_linkHubId.isValid())
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

    const PortId hubPort = hub->addPortToward(endAnchor);
    auto w = buildWire(PortRef{hub->id(), hubPort}, hitPort);
    w->setId(m_doc->allocateId());
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
    auto hub = std::make_unique<CanvasBlock>(QRectF(topLeft, QSizeF(size, size)), true,
                                             linkingModeLabel(m_linkingMode));
    hub->setShowPorts(false);
    hub->setAutoPortLayout(true);
    hub->setPortSnapStep(Constants::kGridStep);
    hub->setLinkHub(true);
    hub->setKeepoutMargin(0.0);
    hub->setId(m_doc->allocateId());

    const PortId hubPortA = hub->addPortToward(startAnchor);
    const PortId hubPortB = hub->addPortToward(endAnchor);
    const ObjectId hubId = hub->id();

    m_doc->commands().execute(std::make_unique<CreateItemCommand>(std::move(hub)));

    auto w0 = buildWire(PortRef{m_wireStartItem, m_wireStartPort}, PortRef{hubId, hubPortA});
    w0->setId(m_doc->allocateId());
    m_doc->commands().execute(std::make_unique<CreateItemCommand>(std::move(w0)));

    auto w1 = buildWire(PortRef{hubId, hubPortB}, hitPort);
    w1->setId(m_doc->allocateId());
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

    m_dragBlock = nullptr;
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

void CanvasController::beginBlockDrag(CanvasBlock* blk, const QPointF& scenePos)
{
    if (!blk)
        return;

    m_dragBlock = blk;
    m_dragStartTopLeft = blk->boundsScene().topLeft();
    m_dragOffset = scenePos - blk->boundsScene().topLeft();
}

void CanvasController::updateBlockDrag(const QPointF& scenePos)
{
    if (!m_dragBlock || !m_view || !m_doc)
        return;

    const QPointF newTopLeft = scenePos - m_dragOffset;
    const double step = m_doc->fabric().config().step;

    QRectF newBounds = m_dragBlock->boundsScene();
    newBounds.moveTopLeft(Utils::snapPointToGrid(newTopLeft, step));
    m_dragBlock->setBoundsScene(newBounds);

    m_view->update();
}

void CanvasController::endBlockDrag()
{
    if (!m_doc || !m_dragBlock) {
        m_dragBlock = nullptr;
        return;
    }

    const QPointF endTopLeft = m_dragBlock->boundsScene().topLeft();
    if (endTopLeft != m_dragStartTopLeft) {
        m_doc->commands().execute(std::make_unique<MoveItemCommand>(
            m_dragBlock->id(), m_dragStartTopLeft, endTopLeft));
    }

    m_dragBlock = nullptr;
}


void CanvasController::onCanvasMousePressed(const QPointF& scenePos, Qt::MouseButtons buttons, Qt::KeyboardModifiers mods)
{
    Q_UNUSED(mods);

    if (!m_view)
        return;

    const QPointF viewPos = sceneToView(scenePos);

    if (buttons.testFlag(Qt::MiddleButton)) {
        beginPanning(viewPos);
        return;
    }

    if (!buttons.testFlag(Qt::LeftButton) || !m_doc)
        return;

    if (handleLinkingPress(scenePos))
        return;

    CanvasItem* hit = hitTestCanvas(m_doc, m_view, scenePos);
    selectItem(hit ? hit->id() : ObjectId{});

    if (auto* wire = dynamic_cast<CanvasWire*>(hit)) {
        beginWireSegmentDrag(wire, scenePos);
        if (m_dragWire)
            return;
    }

    auto* blk = dynamic_cast<CanvasBlock*>(hit);
    if (blk && blk->isMovable())
        beginBlockDrag(blk, scenePos);
}

void CanvasController::onCanvasMouseMoved(const QPointF& scenePos, Qt::MouseButtons buttons, Qt::KeyboardModifiers mods)
{
    Q_UNUSED(mods);

    if (!m_view)
        return;

    updateLinkingHoverAndPreview(scenePos);

    if (m_panning) {
        if (buttons.testFlag(Qt::MiddleButton))
            updatePanning(sceneToView(scenePos));
        else
            endPanning();
        return;
    }

    if (m_dragWire) {
        updateWireSegmentDrag(scenePos);
        return;
    }

    if (m_mode == Mode::Linking)
        return;

    if (m_dragBlock && buttons.testFlag(Qt::LeftButton))
        updateBlockDrag(scenePos);
}

void CanvasController::onCanvasMouseReleased(const QPointF& scenePos, Qt::MouseButtons buttons, Qt::KeyboardModifiers mods)
{
    Q_UNUSED(scenePos);
    Q_UNUSED(mods);

    if (!m_view)
        return;

    if (m_panning && !buttons.testFlag(Qt::MiddleButton))
        endPanning();

    if (!m_doc) {
        m_wiring = false;
        clearTransientDragState();
        return;
    }

    if (m_dragWire) {
        endWireSegmentDrag(scenePos);
        return;
    }

    if (m_dragBlock) {
        endBlockDrag();
    }
}


void CanvasController::onCanvasWheel(const QPointF& scenePos, const QPoint& angleDelta, Qt::KeyboardModifiers mods)
{
	if (!m_view)
		return;

	if (!mods.testFlag(Qt::ControlModifier))
		return;

	const int dy = angleDelta.y();
	if (dy == 0)
		return;

	const double factor = dy > 0 ? Constants::kZoomStep : (1.0 / Constants::kZoomStep);

	const double oldZoom = m_view->zoom();
	const QPointF oldPan = m_view->pan();

	const double newZoom = Tools::clampZoom(oldZoom * factor);
	m_view->setZoom(newZoom);

	const QPointF panNew = ((scenePos + oldPan) * oldZoom / newZoom) - scenePos;
	m_view->setPan(panNew);
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
        if (!m_selected)
            return;
        if (m_doc->commands().execute(std::make_unique<DeleteItemCommand>(m_selected))) {
            m_selected = ObjectId{};
            if (m_view)
                m_view->setSelectedItem(ObjectId{});
        }
    }
}

} // namespace Canvas
