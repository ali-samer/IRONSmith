// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "canvas/CanvasPorts.hpp"
#include "canvas/CanvasTypes.hpp"
#include "canvas/CanvasWire.hpp"

#include <QtCore/QPointF>
#include <QtCore/Qt>

#include <optional>
#include <vector>

namespace Canvas {
class CanvasBlock;
class CanvasDocument;
class CanvasView;
class CanvasWire;
struct PortRef;
}

namespace Canvas::Controllers {

class CanvasSelectionController;

class CanvasDragController final
{
public:
    CanvasDragController(CanvasDocument* doc,
                         CanvasView* view,
                         CanvasSelectionController* selection);

    bool isWireSegmentDragActive() const noexcept { return m_dragWire; }
    bool isEndpointDragActive() const noexcept { return m_dragEndpoint; }
    bool isBlockDragActive() const noexcept { return !m_dragBlocks.empty(); }

    bool hasPendingEndpoint() const noexcept { return m_pendingEndpoint; }
    const std::optional<PortRef>& pendingEndpointPort() const noexcept { return m_pendingEndpointPort; }

    void clearTransientState();

    bool beginPendingEndpoint(const QPointF& scenePos, const QPointF& viewPos);
    bool updatePendingEndpoint(const QPointF& scenePos, Qt::MouseButtons buttons);
    void clearPendingEndpoint();

    void beginWireSegmentDrag(CanvasWire* wire, const QPointF& scenePos);
    void updateWireSegmentDrag(const QPointF& scenePos);
    void endWireSegmentDrag();

    bool beginEndpointDrag(CanvasWire* wire, const QPointF& scenePos);
    void updateEndpointDrag(const QPointF& scenePos);
    void endEndpointDrag(const QPointF& scenePos);

    void beginBlockDrag(CanvasBlock* blk, const QPointF& scenePos);
    void updateBlockDrag(const QPointF& scenePos);
    void endBlockDrag();

private:
    struct DragBlockState final {
        CanvasBlock* block = nullptr;
        QPointF startTopLeft;
    };

    CanvasWire* findWire(ObjectId wireId) const;

    CanvasDocument* m_doc = nullptr;
    CanvasView* m_view = nullptr;
    CanvasSelectionController* m_selection = nullptr;

    bool m_dragWire = false;
    ObjectId m_dragWireId{};
    int m_dragWireSeg = -1;
    bool m_dragWireSegHorizontal = false;
    double m_dragWireOffset = 0.0;
    std::vector<FabricCoord> m_dragWirePath;

    bool m_dragEndpoint = false;
    ObjectId m_dragEndpointWireId{};
    bool m_dragEndpointIsA = false;
    CanvasWire::Endpoint m_dragEndpointOriginal;
    bool m_dragEndpointPortDynamic = false;
    bool m_dragEndpointPortShared = false;
    bool m_dragEndpointPortPaired = false;
    PortRef m_dragEndpointPort{};
    CanvasPort m_dragEndpointPortMeta{};
    size_t m_dragEndpointPortIndex = 0;

    bool m_pendingEndpoint = false;
    ObjectId m_pendingEndpointWireId{};
    std::optional<PortRef> m_pendingEndpointPort;
    QPointF m_pendingEndpointPressScene;
    QPointF m_pendingEndpointPressView;

    std::vector<DragBlockState> m_dragBlocks;
    CanvasBlock* m_dragPrimary = nullptr;
    QPointF m_dragOffset;
    QPointF m_dragPrimaryStartTopLeft;
};

} // namespace Canvas::Controllers
