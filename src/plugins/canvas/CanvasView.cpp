#include "canvas/CanvasView.hpp"

#include "canvas/CanvasScene.hpp"
#include "canvas/CanvasViewport.hpp"

#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QWheelEvent>
#include <QtGui/QResizeEvent>
#include <QtCore/QtGlobal>

#include <cmath>

namespace Canvas {

CanvasView::CanvasView(QWidget* parent)
	: QWidget(parent)
    , m_scene(new CanvasScene(this))
    , m_viewport(new CanvasViewport(this))
{
	setObjectName("CanvasView");
	setMouseTracking(true);
	setFocusPolicy(Qt::StrongFocus);
	setAttribute(Qt::WA_OpaquePaintEvent, true);

    connect(m_scene, &CanvasScene::requestUpdate,
            this, QOverload<>::of(&CanvasView::update));
    connect(m_scene, &CanvasScene::selectedItemsChanged,
            this, &CanvasView::selectedItemsChanged);
    connect(m_scene, &CanvasScene::selectedItemChanged,
            this, &CanvasView::selectedItemChanged);
    connect(m_scene, &CanvasScene::hoveredPortChanged,
            this, &CanvasView::hoveredPortChanged);
    connect(m_scene, &CanvasScene::hoveredPortCleared,
            this, &CanvasView::hoveredPortCleared);

    connect(m_viewport, &CanvasViewport::zoomChanged,
            this, [this](double zoom) {
                update();
                emit zoomChanged(zoom);
            });
    connect(m_viewport, &CanvasViewport::panChanged,
            this, [this](const QPointF& pan) { emit panChanged(pan); });
    connect(m_viewport, &CanvasViewport::panDeltaView,
            this, [this](const QPointF& deltaView) {
                const int dx = qRound(deltaView.x());
                const int dy = qRound(deltaView.y());
                if (dx == 0 && dy == 0) {
                    update();
                    return;
                }
                if (std::abs(dx) >= width() || std::abs(dy) >= height()) {
                    update();
                    return;
                }
                scroll(dx, dy);
            });
    connect(m_viewport, &CanvasViewport::displayZoomBaselineChanged,
            this, [this](double) {
                update();
                emit zoomChanged(zoom());
            });
    connect(m_viewport, &CanvasViewport::sizeChanged,
            this, [this]() { update(); });

    m_viewport->setSize(QSizeF(size()));
}

void CanvasView::setDocument(CanvasDocument* doc)
{
    m_scene->setDocument(doc);
}

void CanvasView::setController(CanvasController* controller)
{
    m_scene->setController(controller);
}

void CanvasView::setSelectionModel(CanvasSelectionModel* model)
{
    m_scene->setSelectionModel(model);
}

ObjectId CanvasView::selectedItem() const noexcept
{
    return m_scene ? m_scene->selectedItem() : ObjectId{};
}

const QSet<ObjectId>& CanvasView::selectedItems() const noexcept
{
    static const QSet<ObjectId> kEmpty;
    return m_scene ? m_scene->selectedItems() : kEmpty;
}

bool CanvasView::isSelected(ObjectId id) const noexcept
{
    return m_scene ? m_scene->isSelected(id) : false;
}

void CanvasView::setSelectedItem(ObjectId id)
{
    if (m_scene)
        m_scene->setSelectedItem(id);
}

void CanvasView::setSelectedItems(const QSet<ObjectId>& items)
{
    if (m_scene)
        m_scene->setSelectedItems(items);
}

void CanvasView::clearSelectedItems()
{
    if (m_scene)
        m_scene->clearSelectedItems();
}

void CanvasView::setSelectedPort(ObjectId itemId, PortId portId)
{
    if (m_scene)
        m_scene->setSelectedPort(itemId, portId);
}

void CanvasView::clearSelectedPort()
{
    if (m_scene)
        m_scene->clearSelectedPort();
}

void CanvasView::setHoveredPort(ObjectId itemId, PortId portId)
{
    if (m_scene)
        m_scene->setHoveredPort(itemId, portId);
}

void CanvasView::clearHoveredPort()
{
    if (m_scene)
        m_scene->clearHoveredPort();
}

void CanvasView::setHoveredEdge(ObjectId itemId, PortSide side, const QPointF& anchorScene)
{
    if (m_scene)
        m_scene->setHoveredEdge(itemId, side, anchorScene);
}

void CanvasView::clearHoveredEdge()
{
    if (m_scene)
        m_scene->clearHoveredEdge();
}

void CanvasView::setMarqueeRect(const QRectF& sceneRect)
{
    if (m_scene)
        m_scene->setMarqueeRect(sceneRect);
}

void CanvasView::clearMarqueeRect()
{
    if (m_scene)
        m_scene->clearMarqueeRect();
}

double CanvasView::zoom() const noexcept
{
    return m_viewport ? m_viewport->zoom() : 1.0;
}

double CanvasView::displayZoom() const noexcept
{
    return m_viewport ? m_viewport->displayZoom() : zoom();
}

double CanvasView::displayZoomBaseline() const noexcept
{
    return m_viewport ? m_viewport->displayZoomBaseline() : 1.0;
}

void CanvasView::setDisplayZoomBaseline(double baseline)
{
    if (m_viewport)
        m_viewport->setDisplayZoomBaseline(baseline);
}

void CanvasView::setZoom(double zoom)
{
    if (m_viewport)
        m_viewport->setZoom(zoom);
}

QPointF CanvasView::pan() const noexcept
{
    return m_viewport ? m_viewport->pan() : QPointF();
}

void CanvasView::setPan(const QPointF& pan)
{
    if (m_viewport)
        m_viewport->setPan(pan);
}

QPointF CanvasView::viewToScene(const QPointF& viewPos) const
{
    return m_viewport ? m_viewport->viewToScene(viewPos) : QPointF();
}

QPointF CanvasView::sceneToView(const QPointF& scenePos) const
{
    return m_viewport ? m_viewport->sceneToView(scenePos) : QPointF();
}

void CanvasView::paintEvent(QPaintEvent* e)
{
	Q_UNUSED(e);

	QPainter p(this);
	p.setRenderHints(QPainter::Antialiasing, true);

    if (!m_scene)
        return;
    CanvasScene::ViewState viewState;
    viewState.size = m_viewport ? m_viewport->size() : QSizeF(width(), height());
    viewState.pan = m_viewport ? m_viewport->pan() : QPointF();
    viewState.zoom = m_viewport ? m_viewport->zoom() : 1.0;
    m_scene->paint(p, viewState);
}

void CanvasView::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (m_viewport)
        m_viewport->setSize(QSizeF(event->size()));
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
