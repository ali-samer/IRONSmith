#pragma once

#include "canvas/CanvasConstants.hpp"
#include "canvas/CanvasItem.hpp"
#include "canvas/CanvasBlockContent.hpp"

#include <utility>
#include <vector>
#include <optional>

#include <QtGui/QColor>
#include <QtCore/QString>
#include <QtCore/QMarginsF>

namespace Canvas {

class BlockContent;

class CANVAS_EXPORT CanvasBlock final : public CanvasItem
{
public:
    CanvasBlock(QRectF boundsScene, bool movable, QString label = {})
        : m_boundsScene(boundsScene)
        , m_movable(movable)
        , m_label(std::move(label))
    {}

    QRectF boundsScene() const override { return m_boundsScene; }
    void setBoundsScene(const QRectF& r) { m_boundsScene = r; }

    bool isMovable() const { return m_movable; }
    void setMovable(bool v) { m_movable = v; }

    bool isDeletable() const { return m_deletable; }
    void setDeletable(bool v) { m_deletable = v; }

    const QString& label() const { return m_label; }
    void setLabel(QString s) { m_label = std::move(s); }

    void setPorts(std::vector<CanvasPort> ports);
    PortId addPort(PortSide side, double t, PortRole role = PortRole::Dynamic, QString name = {});
    PortId addPortToward(const QPointF& targetScene, PortRole role = PortRole::Dynamic, QString name = {});
    bool updatePort(PortId id, PortSide side, double t);
    std::optional<CanvasPort> removePort(PortId id, size_t* indexOut = nullptr);
    bool insertPort(size_t index, CanvasPort port);
    bool hasPorts() const override { return !m_ports.empty(); }
    const std::vector<CanvasPort>& ports() const override { return m_ports; }
    QPointF portAnchorScene(PortId id) const override;

    bool showPorts() const { return m_showPorts; }
    void setShowPorts(bool v) { m_showPorts = v; }
    static void setGlobalShowPorts(bool v) { s_globalShowPorts = v; }
    static bool globalShowPorts() { return s_globalShowPorts; }

    bool allowMultiplePorts() const { return m_allowMultiplePorts; }
    void setAllowMultiplePorts(bool v) { m_allowMultiplePorts = v; }

    bool autoPortLayout() const { return m_autoPortLayout; }
    void setAutoPortLayout(bool v) { m_autoPortLayout = v; }
    double portSnapStep() const { return m_portSnapStep; }
    void setPortSnapStep(double step) { m_portSnapStep = step; }

    bool isLinkHub() const { return m_isLinkHub; }
    void setLinkHub(bool v) { m_isLinkHub = v; }

    void setKeepoutMargin(double marginScene) { m_keepoutMarginScene = marginScene; }
    double keepoutMargin() const { return m_keepoutMarginScene; }

    void draw(QPainter& p, const CanvasRenderContext& ctx) const override;
    std::unique_ptr<CanvasItem> clone() const override;

    bool blocksFabric() const override { return true; }
    QRectF keepoutSceneRect() const override;

    void setContent(std::unique_ptr<BlockContent> content);
    BlockContent* content() const { return m_content.get(); }
    void clearContent() { m_content.reset(); }

    const QMarginsF& contentPadding() const { return m_contentPadding; }
    void setContentPadding(const QMarginsF& padding) { m_contentPadding = padding; }

    void setCustomColors(const QColor& outline, const QColor& fill, const QColor& label);
    void clearCustomColors();
    bool hasCustomColors() const { return m_hasCustomColors; }
    const QColor& outlineColor() const { return m_outlineColor; }
    const QColor& fillColor() const { return m_fillColor; }
    const QColor& labelColor() const { return m_labelColor; }

    void setCornerRadius(double radius) { m_cornerRadius = radius; }
    double cornerRadius() const { return m_cornerRadius; }

private:
    static bool s_globalShowPorts;

    QRectF m_boundsScene;
    bool m_movable = false;
    QString m_label;
    std::vector<CanvasPort> m_ports;
    bool m_showPorts = true;
    bool m_allowMultiplePorts = false;
    bool m_autoPortLayout = false;
    double m_portSnapStep = Constants::kGridStep;
    bool m_isLinkHub = false;
	double m_keepoutMarginScene = -1.0;
    std::unique_ptr<BlockContent> m_content;
    QMarginsF m_contentPadding{8.0, 8.0, 8.0, 8.0};
    bool m_deletable = true;

    bool m_hasCustomColors = false;
    QColor m_outlineColor;
    QColor m_fillColor;
    QColor m_labelColor;
    double m_cornerRadius = -1.0;
};

} // namespace Canvas
