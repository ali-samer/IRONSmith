#include "canvas/CanvasView.hpp"

#include "canvas/CanvasDocument.hpp"
#include "canvas/CanvasController.hpp"
#include "canvas/CanvasConstants.hpp"
#include "canvas/CanvasPorts.hpp"
#include "canvas/CanvasStyle.hpp"
#include "canvas/Tools.hpp"

#include "canvas/CanvasRenderContext.hpp"
#include "canvas/CanvasItem.hpp"

#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QWheelEvent>

#include <algorithm>

namespace Canvas {

static bool isSelectedThunk(void* user, ObjectId id)
{
    const auto* view = static_cast<const CanvasView*>(user);
    return view && view->selectedItem() == id;
}

CanvasView::CanvasView(QWidget* parent)
	: QWidget(parent)
{
	setObjectName("CanvasView");
	setMouseTracking(true);
	setFocusPolicy(Qt::StrongFocus);
	setAttribute(Qt::WA_OpaquePaintEvent, true);
}

void CanvasView::setDocument(CanvasDocument* doc)
{
	if (m_document == doc)
		return;

	if (m_document)
		disconnect(m_document, nullptr, this, nullptr);

	m_document = doc;

	if (m_document) {
		connect(m_document, &CanvasDocument::changed, this, QOverload<>::of(&CanvasView::update));
	}

	update();
}

void CanvasView::setController(CanvasController* controller)
{
	m_controller = controller;
}

void CanvasView::setSelectedItem(ObjectId id)
{
    if (m_selected == id)
        return;
    m_selected = id;
    update();
    emit selectedItemChanged(m_selected);
}

void CanvasView::setSelectedPort(ObjectId itemId, PortId portId)
{
    if (m_hasSelectedPort && m_selectedPortItem == itemId && m_selectedPortId == portId)
        return;
    m_hasSelectedPort = true;
    m_selectedPortItem = itemId;
    m_selectedPortId = portId;
    update();
}

void CanvasView::clearSelectedPort()
{
    if (!m_hasSelectedPort)
        return;
    m_hasSelectedPort = false;
    m_selectedPortItem = ObjectId{};
    m_selectedPortId = PortId{};
    update();
}

void CanvasView::setHoveredPort(ObjectId itemId, PortId portId)
{
	if (m_hasHoveredPort && m_hoveredItem == itemId && m_hoveredPort == portId)
		return;
	m_hasHoveredPort = true;
	m_hoveredItem = itemId;
	m_hoveredPort = portId;
	update();
	emit hoveredPortChanged(m_hoveredItem, m_hoveredPort);
}

void CanvasView::clearHoveredPort()
{
	if (!m_hasHoveredPort)
		return;
	m_hasHoveredPort = false;
	m_hoveredItem = ObjectId{};
	m_hoveredPort = PortId{};
	update();
	emit hoveredPortCleared();
}

void CanvasView::setHoveredEdge(ObjectId itemId, PortSide side, const QPointF& anchorScene)
{
	if (m_hasHoveredEdge && m_hoveredEdgeItem == itemId && m_hoveredEdgeSide == side && m_hoveredEdgeAnchor == anchorScene)
		return;
	m_hasHoveredEdge = true;
	m_hoveredEdgeItem = itemId;
	m_hoveredEdgeSide = side;
	m_hoveredEdgeAnchor = anchorScene;
	update();
}

void CanvasView::clearHoveredEdge()
{
	if (!m_hasHoveredEdge)
		return;
	m_hasHoveredEdge = false;
	m_hoveredEdgeItem = ObjectId{};
	m_hoveredEdgeSide = PortSide::Left;
	m_hoveredEdgeAnchor = QPointF();
	update();
}

void CanvasView::setZoom(double zoom)
{
	const double clamped = std::clamp(zoom, Canvas::Constants::kMinZoom, Canvas::Constants::kMaxZoom);
	if (qFuzzyCompare(m_zoom, clamped))
		return;

	m_zoom = clamped;
	update();
}

void CanvasView::setPan(const QPointF& pan)
{
	if (m_pan == pan)
		return;

	m_pan = pan;
	update();
}

QPointF CanvasView::viewToScene(const QPointF& viewPos) const
{
	return Tools::viewToScene(viewPos, m_pan, m_zoom);
}

QPointF CanvasView::sceneToView(const QPointF& scenePos) const
{
	return Tools::sceneToView(scenePos, m_pan, m_zoom);
}

void CanvasView::paintEvent(QPaintEvent* e)
{
	Q_UNUSED(e);

	QPainter p(this);
	p.setRenderHints(QPainter::Antialiasing, true);

	drawBackgroundLayer(p);

	p.save();
	applyViewTransform(p);
	drawGridFabric(p);
	drawContentLayer(p);
	drawOverlayLayer(p);
	p.restore();
}

void CanvasView::drawBackgroundLayer(QPainter &p) const {
	p.fillRect(rect(), QColor(Constants::CANVAS_BACKGROUND_COLOR));
}

void CanvasView::applyViewTransform(QPainter &p) const {
	p.scale(m_zoom, m_zoom);
	p.translate(m_pan.x(), m_pan.y());
}

void CanvasView::drawGridFabric(QPainter& p) const
{
	if (!m_document)
		return;

	m_document->fabric().draw(p, sceneRect(), &CanvasDocument::isFabricPointBlockedThunk, m_document);
}

void CanvasView::drawContentLayer(QPainter &p) const {
	if (!m_document)
		return;

	const QRectF scene = sceneRect();
	CanvasRenderContext ctx = buildRenderContext(scene, true);

	for (const auto& item : m_document->items()) {
		if (item)
			item->draw(p, ctx);
	}
}

void CanvasView::drawOverlayLayer(QPainter &p) const {
	if (!m_document || !m_controller)
		return;

	if (m_hasHoveredEdge && (m_controller->mode() == CanvasController::Mode::Linking ||
	                         m_controller->isEndpointDragActive())) {
		p.save();
		CanvasStyle::drawPort(p, m_hoveredEdgeAnchor, m_hoveredEdgeSide, PortRole::Dynamic, m_zoom, true);
		p.restore();
	}

	if (m_controller->mode() != CanvasController::Mode::Linking || !m_controller->isLinkingInProgress())
		return;

	const QRectF scene = sceneRect();
	CanvasRenderContext ctx = buildRenderContext(scene, false);

	QPointF aAnchor, aBorder, aFabric;
	QPointF start = m_controller->linkPreviewScene();
	if (ctx.portTerminal(m_controller->linkStartItem(), m_controller->linkStartPort(),
	                     aAnchor, aBorder, aFabric)) {
		start = aAnchor;
	}

	const QPointF end = m_controller->linkPreviewScene();

	p.save();
	QPen pen{QColor(Constants::kWireColor)};
	pen.setWidthF(1.5 / std::clamp(m_zoom, 0.25, 8.0));
	pen.setStyle(Qt::DashLine);
	pen.setCapStyle(Qt::RoundCap);
	p.setPen(pen);
	p.setBrush(Qt::NoBrush);
	p.setOpacity(0.55);
	p.drawLine(start, end);
	p.restore();
}

QRectF CanvasView::sceneRect() const
{
	const QPointF tl = viewToScene(QPointF(0.0, 0.0));
	const QPointF br = viewToScene(QPointF(width(), height()));
	const double left   = std::min(tl.x(), br.x());
	const double right  = std::max(tl.x(), br.x());
	const double top    = std::min(tl.y(), br.y());
	const double bottom = std::max(tl.y(), br.y());
	return QRectF(QPointF(left, top), QPointF(right, bottom));
}

CanvasRenderContext CanvasView::buildRenderContext(const QRectF& sceneRect, bool includeHover) const
{
	CanvasRenderContext ctx;
	ctx.zoom = m_zoom;
	ctx.visibleSceneRect = sceneRect;
	ctx.isSelected = &isSelectedThunk;
	ctx.isSelectedUser = const_cast<CanvasView*>(this);
	ctx.computePortTerminal = &CanvasDocument::computePortTerminalThunk;
	ctx.computePortTerminalUser = m_document;
	ctx.isFabricBlocked = &CanvasDocument::isFabricPointBlockedThunk;
	ctx.isFabricBlockedUser = m_document;
	ctx.fabricStep = m_document ? m_document->fabric().config().step : 0.0;

	if (includeHover) {
		ctx.hasHoveredPort = m_hasHoveredPort;
		ctx.hoveredPortItem = m_hoveredItem;
		ctx.hoveredPortId = m_hoveredPort;
	}
	ctx.hasSelectedPort = m_hasSelectedPort;
	ctx.selectedPortItem = m_selectedPortItem;
	ctx.selectedPortId = m_selectedPortId;
	return ctx;
}

void CanvasView::mousePressEvent(QMouseEvent* event)
{
	emit canvasMousePressed(viewToScene(event->position()), event->buttons(), event->modifiers());
	event->accept();
}

void CanvasView::mouseMoveEvent(QMouseEvent* event)
{
	emit canvasMouseMoved(viewToScene(event->position()), event->buttons(), event->modifiers());
	event->accept();
}

void CanvasView::mouseReleaseEvent(QMouseEvent* event)
{
	emit canvasMouseReleased(viewToScene(event->position()), event->buttons(), event->modifiers());
	event->accept();
}

void CanvasView::wheelEvent(QWheelEvent* event)
{
	emit canvasWheel(viewToScene(event->position()), event->angleDelta(), event->pixelDelta(), event->modifiers());
	event->accept();
}

void CanvasView::keyPressEvent(QKeyEvent* event)
{
    emit canvasKeyPressed(event->key(), event->modifiers());
    event->accept();
}

} // namespace Canvas
