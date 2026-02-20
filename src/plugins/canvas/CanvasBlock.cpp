// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include "canvas/CanvasBlock.hpp"

#include "canvas/CanvasConstants.hpp"
#include "canvas/CanvasBlockContent.hpp"
#include "canvas/CanvasStyle.hpp"
#include "canvas/utils/CanvasGeometry.hpp"
#include "canvas/utils/CanvasAutoPorts.hpp"

#include <QtCore/QHash>
#include <QtGui/QPainter>
#include <algorithm>
#include <cmath>

namespace Canvas {

bool CanvasBlock::s_globalShowPorts = true;

std::unique_ptr<CanvasItem> CanvasBlock::clone() const
{
    auto blk = std::make_unique<CanvasBlock>(m_boundsScene, m_movable, m_label);
    blk->setSpecId(m_specId);
    blk->setPorts(m_ports);
    blk->m_showPorts = m_showPorts;
    blk->m_allowMultiplePorts = m_allowMultiplePorts;
    blk->m_hasAutoPortRole = m_hasAutoPortRole;
    blk->m_autoPortRole = m_autoPortRole;
    blk->m_autoOppositeProducerPort = m_autoOppositeProducerPort;
    blk->m_showPortLabels = m_showPortLabels;
    blk->m_autoPortLayout = m_autoPortLayout;
    blk->m_portSnapStep = m_portSnapStep;
    blk->m_isLinkHub = m_isLinkHub;
    blk->m_deletable = m_deletable;
    blk->m_contentPadding = m_contentPadding;
    if (m_content)
        blk->m_content = m_content->clone();
    blk->setKeepoutMargin(m_keepoutMarginScene);
    if (m_hasCustomColors)
        blk->setCustomColors(m_outlineColor, m_fillColor, m_labelColor);
    blk->setCornerRadius(m_cornerRadius);
    blk->setId(id());
    return blk;
}

void CanvasBlock::setPorts(std::vector<CanvasPort> ports)
{
    m_ports = std::move(ports);
}

static PortSide sideFromAngle(double angle)
{
    const double dx = std::cos(angle);
    const double dy = std::sin(angle);
    if (std::abs(dx) >= std::abs(dy))
        return dx >= 0.0 ? PortSide::Right : PortSide::Left;
    return dy >= 0.0 ? PortSide::Bottom : PortSide::Top;
}

static double tFromAngle(double angle, PortSide side)
{
    const double dx = std::cos(angle);
    const double dy = std::sin(angle);
    double t = 0.5;
    switch (side) {
        case PortSide::Left:
        case PortSide::Right:
            t = (dy + 1.0) * 0.5;
            break;
        case PortSide::Top:
        case PortSide::Bottom:
            t = (dx + 1.0) * 0.5;
            break;
    }
    return t;
}

static double snapToStep(double v, double step)
{
    if (step <= 0.0)
        return v;
    return std::llround(v / step) * step;
}

PortId CanvasBlock::addPort(PortSide side, double t, PortRole role, QString name)
{
    CanvasPort port;
    port.id = PortId::create();
    port.role = role;
    port.side = side;
    port.t = t;
    port.name = std::move(name);
    m_ports.push_back(port);
    return port.id;
}

PortId CanvasBlock::addPortToward(const QPointF& targetScene, PortRole role, QString name)
{
    const QPointF center = m_boundsScene.center();
    const QPointF d = targetScene - center;
    const double len2 = d.x() * d.x() + d.y() * d.y();
    double angle = 0.0;
    if (len2 > 1e-6)
        angle = std::atan2(d.y(), d.x());

    const PortSide side = sideFromAngle(angle);
    const double t = tFromAngle(angle, side);
    return addPort(side, t, role, std::move(name));
}

bool CanvasBlock::updatePort(PortId id, PortSide side, double t)
{
    for (auto& port : m_ports) {
        if (port.id == id) {
            port.side = side;
            port.t = t;
            return true;
        }
    }
    return false;
}

bool CanvasBlock::updatePortName(PortId id, QString name)
{
    for (auto& port : m_ports) {
        if (port.id == id) {
            if (port.name == name)
                return false;
            port.name = std::move(name);
            return true;
        }
    }
    return false;
}

std::optional<CanvasPort> CanvasBlock::removePort(PortId id, size_t* indexOut)
{
    for (size_t i = 0; i < m_ports.size(); ++i) {
        if (m_ports[i].id == id) {
            if (indexOut)
                *indexOut = i;
            CanvasPort removed = m_ports[i];
            m_ports.erase(m_ports.begin() + static_cast<std::ptrdiff_t>(i));
            return removed;
        }
    }
    return std::nullopt;
}

bool CanvasBlock::insertPort(size_t index, CanvasPort port)
{
    if (index > m_ports.size())
        index = m_ports.size();
    m_ports.insert(m_ports.begin() + static_cast<std::ptrdiff_t>(index), std::move(port));
    return true;
}

QPointF CanvasBlock::portAnchorScene(PortId id) const
{
    for (const auto& port : m_ports) {
        if (port.id != id) continue;

        const double t = Support::clampT(port.t);
        const QRectF r = m_boundsScene;
        const double step = m_portSnapStep;

        switch (port.side) {
            case PortSide::Left: {
                double y = r.top() + t * r.height();
                if (step > 0.0) {
                    const double minY = r.top() + step;
                    const double maxY = r.bottom() - step;
                    if (minY <= maxY)
                        y = std::clamp(snapToStep(y, step), minY, maxY);
                    else
                        y = (r.top() + r.bottom()) * 0.5;
                }
                return QPointF(r.left(), y);
            }
            case PortSide::Right: {
                double y = r.top() + t * r.height();
                if (step > 0.0) {
                    const double minY = r.top() + step;
                    const double maxY = r.bottom() - step;
                    if (minY <= maxY)
                        y = std::clamp(snapToStep(y, step), minY, maxY);
                    else
                        y = (r.top() + r.bottom()) * 0.5;
                }
                return QPointF(r.right(), y);
            }
            case PortSide::Top: {
                double x = r.left() + t * r.width();
                if (step > 0.0) {
                    const double minX = r.left() + step;
                    const double maxX = r.right() - step;
                    if (minX <= maxX)
                        x = std::clamp(snapToStep(x, step), minX, maxX);
                    else
                        x = (r.left() + r.right()) * 0.5;
                }
                return QPointF(x, r.top());
            }
            case PortSide::Bottom: {
                double x = r.left() + t * r.width();
                if (step > 0.0) {
                    const double minX = r.left() + step;
                    const double maxX = r.right() - step;
                    if (minX <= maxX)
                        x = std::clamp(snapToStep(x, step), minX, maxX);
                    else
                        x = (r.left() + r.right()) * 0.5;
                }
                return QPointF(x, r.bottom());
            }
        }
    }
    return QPointF();
}

void CanvasBlock::draw(QPainter& p, const CanvasRenderContext& ctx) const
{
    const double radius = (m_cornerRadius >= 0.0) ? m_cornerRadius : Constants::kBlockCornerRadius;
    if (m_hasCustomColors) {
        CanvasStyle::drawBlockFrame(p, m_boundsScene, ctx.zoom, m_outlineColor, m_fillColor,
                                    radius);
    } else {
        CanvasStyle::drawBlockFrame(p, m_boundsScene, ctx.zoom,
                                    QColor(Constants::kBlockOutlineColor),
                                    QColor(Constants::kBlockFillColor),
                                    radius);
    }
    if (ctx.selected(id()))
        CanvasStyle::drawBlockSelection(p, m_boundsScene, ctx.zoom);
    if (!m_label.isEmpty())
        CanvasStyle::drawBlockLabel(p, m_boundsScene, ctx.zoom, m_label,
                                    m_hasCustomColors ? m_labelColor : QColor(Constants::kBlockTextColor));
    if (m_content) {
        const QRectF contentRect = m_boundsScene.adjusted(m_contentPadding.left(),
                                                          m_contentPadding.top(),
                                                          -m_contentPadding.right(),
                                                          -m_contentPadding.bottom());
        m_content->layout(contentRect, ctx);
        m_content->draw(p, ctx);
    }

    if (m_showPorts && s_globalShowPorts) {
        QHash<QString, int> labelIndices;
        if (m_showPortLabels) {
            int nextIndex = 1;
            for (const auto& port : m_ports) {
                if (!Support::isPairedProducerPort(port))
                    continue;
                const auto key = Support::pairedPortKey(port);
                if (!key || key->isEmpty())
                    continue;
                if (!labelIndices.contains(*key))
                    labelIndices.insert(*key, nextIndex++);
            }
        }

        for (const auto& port : m_ports) {
            const QPointF a = portAnchorScene(port.id);
            const bool hovered = ctx.portHovered(id(), port.id);
            const bool selected = ctx.portSelected(id(), port.id);
            CanvasStyle::drawPort(p, a, port.side, port.role, ctx.zoom, hovered || selected);

            if (m_showPortLabels) {
                QString label;
                QString key;
                if (auto k = Support::pairedPortKey(port); k && !k->isEmpty()) {
                    key = *k;
                } else if (port.role == PortRole::Consumer) {
                    const QString legacyKey = port.id.toString();
                    if (labelIndices.contains(legacyKey))
                        key = legacyKey;
                }

                if (!key.isEmpty()) {
                    const int index = labelIndices.value(key, 0);
                    if (index > 0) {
                        const bool isProducer = (port.role == PortRole::Producer);
                        label = (isProducer ? QStringLiteral("OUT_%1")
                                            : QStringLiteral("IN_%1"))
                                    .arg(index);
                    }
                }

                if (!label.isEmpty()) {
                    const QColor labelColor = m_hasCustomColors ? m_labelColor
                                                                : QColor(Constants::kBlockTextColor);
                    CanvasStyle::drawPortLabel(p, a, port.side, ctx.zoom, label, labelColor);
                }
            }
        }
    }
}

void CanvasBlock::setCustomColors(const QColor& outline, const QColor& fill, const QColor& label)
{
    m_outlineColor = outline;
    m_fillColor = fill;
    m_labelColor = label;
    m_hasCustomColors = true;
}

void CanvasBlock::clearCustomColors()
{
    m_hasCustomColors = false;
    m_outlineColor = QColor();
    m_fillColor = QColor();
    m_labelColor = QColor();
}

QRectF CanvasBlock::keepoutSceneRect() const
{
	double m = (m_keepoutMarginScene >= 0.0) ? m_keepoutMarginScene : Constants::kBlockKeepoutMargin;
	const double step = Constants::kGridStep;
	if (step > 0.0)
		m = std::ceil(m / step) * step;
    return m_boundsScene.adjusted(-m, -m, m, m);
}

void CanvasBlock::setContent(std::unique_ptr<BlockContent> content)
{
    m_content = std::move(content);
}

} // namespace Canvas
