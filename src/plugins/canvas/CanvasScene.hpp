#pragma once

#include "canvas/CanvasGlobal.hpp"
#include "canvas/CanvasPorts.hpp"
#include "canvas/CanvasRenderContext.hpp"
#include "canvas/CanvasTypes.hpp"
#include "canvas/CanvasDocument.hpp"
#include "canvas/CanvasController.hpp"
#include "canvas/CanvasSelectionModel.hpp"

#include <QtCore/QPointer>
#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtCore/QSet>
#include <QtCore/QSizeF>

#include <QtCore/QObject>
#include <qnamespace.h>

QT_BEGIN_NAMESPACE
class QPainter;
QT_END_NAMESPACE

namespace Canvas {

class CANVAS_EXPORT CanvasScene final : public QObject
{
    Q_OBJECT

public:
    struct ViewState final {
        QSizeF size;
        QPointF pan;
        double zoom = 1.0;
    };

    explicit CanvasScene(QObject* parent = nullptr);

    void setDocument(CanvasDocument* doc);
    void setController(CanvasController* controller);
    void setSelectionModel(CanvasSelectionModel* model);

    CanvasDocument* document() const noexcept { return m_document; }
    CanvasController* controller() const noexcept { return m_controller; }
    CanvasSelectionModel* selectionModel() const noexcept { return m_selectionModel; }

    ObjectId selectedItem() const noexcept;
    const QSet<ObjectId>& selectedItems() const noexcept;
    bool isSelected(ObjectId id) const noexcept;
    bool isPortSelected(ObjectId itemId, PortId portId) const noexcept;
    void setSelectedItem(ObjectId id);
    void setSelectedItems(const QSet<ObjectId>& items);
    void clearSelectedItems();
    void setSelectedPort(ObjectId itemId, PortId portId);
    void clearSelectedPort();

    void setHoveredPort(ObjectId itemId, PortId portId);
    void clearHoveredPort();
    void setHoveredEdge(ObjectId itemId, PortSide side, const QPointF& anchorScene);
    void clearHoveredEdge();
    void setMarqueeRect(const QRectF& sceneRect);
    void clearMarqueeRect();

    void paint(QPainter& p, const ViewState& view) const;
    QRectF sceneRect(const ViewState& view) const;

signals:
    void requestUpdate();
    void selectedItemChanged(Canvas::ObjectId id);
    void selectedItemsChanged();
    void hoveredPortChanged(Canvas::ObjectId itemId, Canvas::PortId portId);
    void hoveredPortCleared();

private:
    void drawBackgroundLayer(QPainter& p) const;
    void applyViewTransform(QPainter& p, const ViewState& view) const;
    void drawGridFabric(QPainter& p, const QRectF& visibleScene) const;
    void drawContentLayer(QPainter& p, const QRectF& visibleScene, double zoom) const;
    void drawOverlayLayer(QPainter& p, const QRectF& visibleScene, double zoom) const;
    CanvasRenderContext buildRenderContext(const QRectF& sceneRect, bool includeHover, double zoom) const;

    QPointer<CanvasDocument> m_document;
    QPointer<CanvasController> m_controller;
    QPointer<CanvasSelectionModel> m_selectionModel;

    bool m_hasHoveredPort = false;
    ObjectId m_hoveredItem{};
    PortId m_hoveredPort{};
    bool m_hasHoveredEdge = false;
    ObjectId m_hoveredEdgeItem{};
    PortSide m_hoveredEdgeSide = PortSide::Left;
    QPointF m_hoveredEdgeAnchor{};
    bool m_hasMarquee = false;
    QRectF m_marqueeSceneRect;
};

} // namespace Canvas
