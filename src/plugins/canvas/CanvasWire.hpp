#pragma once

#include "canvas/CanvasItem.hpp"

#include <QtGui/QColor>
#include <optional>

namespace Canvas {

struct CANVAS_EXPORT PortRef final {
    ObjectId itemId{};
    PortId   portId{};
};

class CANVAS_EXPORT CanvasWire final : public CanvasItem
{
public:
    struct Endpoint final {
        std::optional<PortRef> attached;
        QPointF freeScene{0.0, 0.0};
    };

    CanvasWire(Endpoint a, Endpoint b)
        : m_a(std::move(a))
        , m_b(std::move(b))
    {}

    const Endpoint& a() const { return m_a; }
    const Endpoint& b() const { return m_b; }

    void setEndpointA(Endpoint e) { m_a = std::move(e); }
    void setEndpointB(Endpoint e) { m_b = std::move(e); }

    WireArrowPolicy arrowPolicy() const noexcept { return m_arrowPolicy; }
    void setArrowPolicy(WireArrowPolicy policy) { m_arrowPolicy = policy; }

    bool hasColorOverride() const noexcept { return m_hasColorOverride; }
    const QColor& colorOverride() const { return m_colorOverride; }
    void setColorOverride(const QColor& color);
    void clearColorOverride();

    void draw(QPainter& p, const CanvasRenderContext& ctx) const override;
    QRectF boundsScene() const override;
    std::unique_ptr<CanvasItem> clone() const override;

    bool hitTest(const QPointF& scenePos) const override;
    bool hitTest(const QPointF& scenePos, const CanvasRenderContext& ctx) const;

    bool hasRouteOverride() const noexcept { return !m_routeOverride.empty(); }
    const std::vector<FabricCoord>& routeOverride() const noexcept { return m_routeOverride; }
    void setRouteOverride(std::vector<FabricCoord> path);
    void clearRouteOverride();

    bool attachesTo(ObjectId itemId) const;

    std::vector<QPointF> resolvedPathScene(const CanvasRenderContext& ctx) const;
    std::vector<FabricCoord> resolvedPathCoords(const CanvasRenderContext& ctx) const;

private:
    Endpoint m_a;
    Endpoint m_b;
    std::vector<FabricCoord> m_routeOverride;
    FabricCoord m_overrideStart{};
    FabricCoord m_overrideEnd{};
    mutable bool m_overrideStale = false;
    WireArrowPolicy m_arrowPolicy = WireArrowPolicy::End;
    bool m_hasColorOverride = false;
    QColor m_colorOverride;
};

} // namespace Canvas
