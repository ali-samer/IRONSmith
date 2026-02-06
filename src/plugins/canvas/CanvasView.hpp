#pragma once

#include "canvas/CanvasGlobal.hpp"
#include "canvas/CanvasRenderContext.hpp"
#include "canvas/CanvasTypes.hpp"

#include <QtCore/QPointer>
#include <QtCore/QPointF>
#include <QtCore/QPoint>
#include <QtCore/QRectF>

#include <QtWidgets/QWidget>

class QPainter;

namespace Canvas {

class CanvasDocument;
class CanvasController;

class CANVAS_EXPORT CanvasView final : public QWidget
{
	Q_OBJECT

	Q_PROPERTY(double zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
	Q_PROPERTY(QPointF pan READ pan WRITE setPan NOTIFY panChanged)

public:
	explicit CanvasView(QWidget* parent = nullptr);

	void setDocument(CanvasDocument* doc);
	void setController(CanvasController* controller);

	double zoom() const { return m_zoom; }
	void setZoom(double zoom);

	QPointF pan() const { return m_pan; }
	void setPan(const QPointF& pan);

	QPointF viewToScene(const QPointF& viewPos) const;
	QPointF sceneToView(const QPointF& scenePos) const;

	ObjectId selectedItem() const noexcept { return m_selected; }
	void setSelectedItem(ObjectId id);

	void setHoveredPort(ObjectId itemId, PortId portId);
	void clearHoveredPort();

signals:
	void zoomChanged(double zoom);
	void panChanged(QPointF pan);
	void canvasMousePressed(const QPointF& scenePos, Qt::MouseButtons buttons, Qt::KeyboardModifiers mods);
	void canvasMouseMoved(const QPointF& scenePos, Qt::MouseButtons buttons, Qt::KeyboardModifiers mods);
	void canvasMouseReleased(const QPointF& scenePos, Qt::MouseButtons buttons, Qt::KeyboardModifiers mods);
	void canvasWheel(const QPointF& scenePos, const QPoint& angleDelta, const QPoint& pixelDelta, Qt::KeyboardModifiers mods);
	void canvasKeyPressed(int key, Qt::KeyboardModifiers mods);

protected:
	void paintEvent(QPaintEvent* event) override;

	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;
	void keyPressEvent(QKeyEvent* event) override;

private:
	void drawBackgroundLayer(QPainter& p) const;
	void applyViewTransform(QPainter& p) const;
	void drawGridFabric(QPainter& p) const;
	void drawContentLayer(QPainter& p) const;
	void drawOverlayLayer(QPainter& p) const;
	QRectF sceneRect() const;
	CanvasRenderContext buildRenderContext(const QRectF& sceneRect, bool includeHover) const;

	QPointer<CanvasDocument> m_document;
	QPointer<CanvasController> m_controller;

	double  m_zoom = 1.0;
	QPointF m_pan = {0.0, 0.0};
	ObjectId m_selected{};
	bool m_hasHoveredPort = false;
	ObjectId m_hoveredItem{};
	PortId m_hoveredPort{};
};

} // namespace Canvas
