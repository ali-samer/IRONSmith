#pragma once

#include "canvas/CanvasTypes.hpp"

#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtCore/QSet>
#include <QtCore/Qt>

namespace Canvas {
class CanvasDocument;
class CanvasSelectionModel;
class CanvasView;
struct PortRef;
}

namespace Canvas::Controllers {

class CanvasSelectionController final
{
public:
    CanvasSelectionController(CanvasDocument* doc,
                              CanvasView* view,
                              CanvasSelectionModel* selection);

    const QSet<ObjectId>& selectedItems() const noexcept;
    bool isSelected(ObjectId id) const noexcept;

    bool hasSelectedPort() const noexcept;
    ObjectId selectedPortItem() const noexcept;
    PortId selectedPortId() const noexcept;

    void selectItem(ObjectId id);
    void selectPort(const PortRef& port);
    void clearSelectedPort();

    void setSelection(const QSet<ObjectId>& ids);
    void clearSelection();
    void addToSelection(ObjectId id);
    void toggleSelection(ObjectId id);

    bool isMarqueeActive() const noexcept { return m_marqueeActive; }
    void beginMarqueeSelection(const QPointF& scenePos, Qt::KeyboardModifiers mods);
    void updateMarqueeSelection(const QPointF& scenePos);
    void endMarqueeSelection(const QPointF& scenePos);
    void clearMarqueeSelection();

private:
    QSet<ObjectId> collectItemsInRect(const QRectF& sceneRect) const;

    CanvasDocument* m_doc = nullptr;
    CanvasView* m_view = nullptr;
    CanvasSelectionModel* m_selection = nullptr;

    bool m_marqueeActive = false;
    QPointF m_marqueeStartScene;
    QPointF m_marqueeStartView;
    QRectF m_marqueeRectScene;
    Qt::KeyboardModifiers m_marqueeMods;
    QSet<ObjectId> m_marqueeBaseSelection;
};

} // namespace Canvas::Controllers
