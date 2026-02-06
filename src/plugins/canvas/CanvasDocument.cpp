#include "canvas/CanvasDocument.hpp"

#include "canvas/CanvasConstants.hpp"

#include "canvas/CanvasBlock.hpp"
#include "canvas/CanvasPorts.hpp"
#include "canvas/CanvasWire.hpp"
#include "canvas/CanvasRenderContext.hpp"
#include "canvas/utils/CanvasGeometry.hpp"
#include "canvas/utils/CanvasPortHitTest.hpp"

#include <QtCore/QPointF>
#include <QtCore/QtGlobal>

#include <vector>
#include <cmath>
#include <array>
#include <algorithm>
#include <utility>

namespace Canvas {

namespace {

int sideIndex(PortSide side)
{
    switch (side) {
        case PortSide::Left: return 0;
        case PortSide::Right: return 1;
        case PortSide::Top: return 2;
        case PortSide::Bottom: return 3;
    }
    return 0;
}

PortSide sideFromDelta(const QPointF& d)
{
    if (std::abs(d.x()) >= std::abs(d.y()))
        return d.x() >= 0.0 ? PortSide::Right : PortSide::Left;
    return d.y() >= 0.0 ? PortSide::Bottom : PortSide::Top;
}

void stepOutCoord(FabricCoord& coord, PortSide side)
{
    switch (side) {
        case PortSide::Left:   coord.x -= 1; break;
        case PortSide::Right:  coord.x += 1; break;
        case PortSide::Top:    coord.y -= 1; break;
        case PortSide::Bottom: coord.y += 1; break;
    }
}

struct PortConn final {
    PortId id{};
    PortSide side{PortSide::Left};
    double key = 0.0;
};

void addEndpointConnection(const CanvasDocument& doc,
                           CanvasBlock& block,
                           std::array<std::vector<PortConn>, 4>& groups,
                           const QPointF& center,
                           const CanvasWire::Endpoint& endpoint,
                           const CanvasWire::Endpoint& other)
{
    if (!endpoint.attached || endpoint.attached->itemId != block.id())
        return;
    if (other.attached && other.attached->itemId == block.id())
        return;

    QPointF target = other.freeScene;
    if (other.attached) {
        QPointF a, b, f;
        if (doc.computePortTerminal(other.attached->itemId,
                                    other.attached->portId,
                                    a, b, f))
            target = a;
    }

    const QPointF d = target - center;
    const PortSide side = sideFromDelta(d);
    const double key = (side == PortSide::Left || side == PortSide::Right)
                           ? target.y()
                           : target.x();
    groups[sideIndex(side)].push_back(PortConn{endpoint.attached->portId, side, key});
}

void collectPortGroups(const CanvasDocument& doc,
                       CanvasBlock& block,
                       std::array<std::vector<PortConn>, 4>& groups)
{
    const QPointF center = block.boundsScene().center();

    for (const auto& it : doc.items()) {
        if (!it)
            continue;
        auto* wire = dynamic_cast<CanvasWire*>(it.get());
        if (!wire)
            continue;
        addEndpointConnection(doc, block, groups, center, wire->a(), wire->b());
        addEndpointConnection(doc, block, groups, center, wire->b(), wire->a());
    }
}

bool resizeBlockForPorts(const CanvasDocument& doc,
                         CanvasBlock& block,
                         const std::array<std::vector<PortConn>, 4>& groups,
                         double step,
                         QRectF& bounds)
{
    bounds = block.boundsScene();
    const size_t countLeft = groups[sideIndex(PortSide::Left)].size();
    const size_t countRight = groups[sideIndex(PortSide::Right)].size();
    const size_t countTop = groups[sideIndex(PortSide::Top)].size();
    const size_t countBottom = groups[sideIndex(PortSide::Bottom)].size();

    const size_t maxVertical = std::max(countLeft, countRight);
    const size_t maxHorizontal = std::max(countTop, countBottom);
    const double requiredHeight = std::max(bounds.height(),
                                           (static_cast<double>(maxVertical) + 1.0) * step);
    const double requiredWidth = std::max(bounds.width(),
                                          (static_cast<double>(maxHorizontal) + 1.0) * step);
    const double size = std::max(requiredWidth, requiredHeight);

    if (size <= bounds.width() && size <= bounds.height())
        return false;

    const QPointF center = bounds.center();
    bounds = QRectF(center.x() - size * 0.5, center.y() - size * 0.5, size, size);
    block.setBoundsScene(bounds);

    for (const auto& it : doc.items()) {
        auto* wire = dynamic_cast<CanvasWire*>(it.get());
        if (wire && wire->hasRouteOverride() && wire->attachesTo(block.id()))
            wire->clearRouteOverride();
    }
    return true;
}

void layoutPortsOnSide(CanvasBlock& block,
                       const QRectF& bounds,
                       double step,
                       std::vector<PortConn>& list)
{
    if (list.empty())
        return;

    const PortSide side = list.front().side;
    std::sort(list.begin(), list.end(),
              [](const PortConn& a, const PortConn& b) { return a.key < b.key; });

    const double length = (side == PortSide::Left || side == PortSide::Right)
                              ? bounds.height()
                              : bounds.width();
    if (length <= 1e-6)
        return;

    const double start = (side == PortSide::Left || side == PortSide::Right)
                             ? bounds.top()
                             : bounds.left();

    for (size_t i = 0; i < list.size(); ++i) {
        const double pos = start + step * (static_cast<double>(i) + 1.0);
        const double t = (pos - start) / length;
        block.updatePort(list[i].id, side, t);
    }
}
} // namespace

CanvasBlock* CanvasDocument::createBlock(const QRectF& boundsScene, bool movable)
{
    const double step = m_fabric.config().step;
    const QRectF snapped = Utils::snapBoundsToGrid(boundsScene, step);

    auto blk = std::make_unique<CanvasBlock>(snapped, movable);
    blk->setId(nextId());

    CanvasBlock* raw = blk.get();
    m_items.push_back(std::move(blk));
    notifyChanged();
    return raw;
}


CanvasItem* CanvasDocument::hitTest(const QPointF& scenePos) const
{
	for (auto it = m_items.rbegin(); it != m_items.rend(); ++it) {
		CanvasItem* item = it->get();
		if (!item)
			continue;
		if (auto* wire = dynamic_cast<CanvasWire*>(item)) {
			CanvasRenderContext ctx;
			ctx.computePortTerminal = &CanvasDocument::computePortTerminalThunk;
			ctx.computePortTerminalUser = const_cast<CanvasDocument*>(this);
			ctx.isFabricBlocked = &CanvasDocument::isFabricPointBlockedThunk;
			ctx.isFabricBlockedUser = const_cast<CanvasDocument*>(this);
			ctx.fabricStep = m_fabric.config().step;
			if (wire->hitTest(scenePos, ctx))
				return item;
			continue;
		}
		if (item->hitTest(scenePos))
			return item;
	}
	return nullptr;
}

std::optional<PortRef> CanvasDocument::hitTestPort(const QPointF& scenePos, double radiusScene) const
{
    ensureAutoPortLayout();
    for (auto it = m_items.rbegin(); it != m_items.rend(); ++it) {
        const CanvasItem* item = it->get();
        if (!item || !item->hasPorts())
            continue;
        for (const auto& port : item->ports()) {
            const QPointF a = item->portAnchorScene(port.id);
            if (Utils::hitTestPortGeometry(a, port.side, scenePos, radiusScene))
                return PortRef{item->id(), port.id};
        }
    }
    return std::nullopt;
}

bool CanvasDocument::getPort(ObjectId itemId, PortId portId, CanvasPort& out) const
{
    const CanvasItem* item = findItem(itemId);
    if (!item || !item->hasPorts())
        return false;
    for (const auto& p : item->ports()) {
        if (p.id == portId) { out = p; return true; }
    }
    return false;
}

std::optional<CanvasDocument::RemovedItem> CanvasDocument::removeItem(ObjectId itemId)
{
    for (size_t i = 0; i < m_items.size(); ++i) {
        auto& ptr = m_items[i];
        if (ptr && ptr->id() == itemId) {
            RemovedItem out;
            out.index = i;
            out.item = std::move(ptr);
            m_items.erase(m_items.begin() + static_cast<std::ptrdiff_t>(i));
            notifyChanged();
            return out;
        }
    }
    return std::nullopt;
}

bool CanvasDocument::insertItem(size_t index, std::unique_ptr<CanvasItem> item)
{
    if (!item)
        return false;
    if (index > m_items.size())
        index = m_items.size();
    m_items.insert(m_items.begin() + static_cast<std::ptrdiff_t>(index), std::move(item));
    notifyChanged();
    return true;
}

ObjectId CanvasDocument::nextId()
{
    return ObjectId::create();
}

CanvasItem* CanvasDocument::findItem(ObjectId id) const
{
    if (id.isNull())
        return nullptr;

    for (const auto& it : m_items) {
        if (it && it->id() == id)
            return it.get();
    }
    return nullptr;
}

void CanvasDocument::notifyChanged()
{
    if (!m_inAutoPortLayout)
        scheduleAutoPortLayout();
    emit changed();
}

void CanvasDocument::scheduleAutoPortLayout()
{
    if (m_autoPortLayoutPending)
        return;
    m_autoPortLayoutPending = true;
    m_autoPortLayoutTimer.start();
}

void CanvasDocument::applyAutoPortLayout()
{
    if (!m_autoPortLayoutPending)
        return;

    m_autoPortLayoutPending = false;
    m_autoPortLayoutTimer.stop();

    bool changed_ = false;
    m_inAutoPortLayout = true;
    for (const auto& it : m_items) {
        auto* block = dynamic_cast<CanvasBlock*>(it.get());
        if (!block || !block->autoPortLayout() || !block->hasPorts())
            continue;
        changed_ = arrangeAutoPorts(*block) || changed_;
    }
    m_inAutoPortLayout = false;

    if (changed_)
        emit changed();
}

void CanvasDocument::ensureAutoPortLayout() const
{
    if (!m_autoPortLayoutPending)
        return;
    auto* self = const_cast<CanvasDocument*>(this);
    self->applyAutoPortLayout();
}

bool CanvasDocument::setItemTopLeftImpl(CanvasItem* item, const QPointF& newTopLeftScene, bool emitChanged)
{
    auto* block = dynamic_cast<CanvasBlock*>(item);
    if (!block)
        return false;
    if (!block->isMovable())
        return false;

    const double step = m_fabric.config().step;
    const QPointF snappedTopLeft(Utils::snapCoord(newTopLeftScene.x(), step),
                                 Utils::snapCoord(newTopLeftScene.y(), step));

    QRectF r = block->boundsScene();
    r.moveTopLeft(snappedTopLeft);
    if (r == block->boundsScene())
        return true;

    block->setBoundsScene(r);
    for (const auto& it : m_items) {
        auto* wire = dynamic_cast<CanvasWire*>(it.get());
        if (wire && wire->hasRouteOverride() && wire->attachesTo(item->id()))
            wire->clearRouteOverride();
    }
    if (emitChanged)
        notifyChanged();
    return true;
}

bool CanvasDocument::setItemTopLeft(ObjectId itemId, const QPointF& newTopLeftScene)
{
    return setItemTopLeftImpl(findItem(itemId), newTopLeftScene, true);
}

bool CanvasDocument::previewSetItemTopLeft(ObjectId itemId, const QPointF& newTopLeftScene)
{
    return setItemTopLeftImpl(findItem(itemId), newTopLeftScene, true);
}

CanvasDocument::CanvasDocument(QObject* parent)
    : QObject(parent)
    , m_commands(this)
{
    m_autoPortLayoutTimer.setSingleShot(true);
    m_autoPortLayoutTimer.setInterval(0);
    connect(&m_autoPortLayoutTimer, &QTimer::timeout, this, &CanvasDocument::applyAutoPortLayout);

	CanvasFabric::Config cfg;
	cfg.step = Constants::kGridStep;
	m_fabric.setConfig(cfg);
}

QString CanvasDocument::statusText() const
{
	return m_statusText;
}

void CanvasDocument::setStatusText(QString text)
{
	if (m_statusText == text)
		return;

	m_statusText = std::move(text);
	notifyChanged();
}

bool CanvasDocument::isFabricPointBlocked(const FabricCoord& coord) const
{
	const double step = m_fabric.config().step;
	const QPointF p(coord.x * step, coord.y * step);

	for (const auto& it : m_items) {
		if (!it || !it->blocksFabric())
			continue;
		if (it->keepoutSceneRect().contains(p))
			return true;
	}
	return false;
}

bool CanvasDocument::computePortTerminal(ObjectId itemId, PortId portId,
                                        QPointF& outAnchorScene,
                                        QPointF& outBorderScene,
                                        QPointF& outFabricScene) const
{
    ensureAutoPortLayout();
    const CanvasItem* item = findItem(itemId);
    if (!item || !item->hasPorts())
        return false;

    std::optional<CanvasPort> meta;
    for (const auto& p : item->ports()) {
        if (p.id == portId) { meta = p; break; }
    }
    if (!meta.has_value())
        return false;

    outAnchorScene = item->portAnchorScene(portId);

    const QRectF keepout = item->blocksFabric() ? item->keepoutSceneRect() : item->boundsScene();

    outBorderScene = outAnchorScene;
    if (meta->side == PortSide::Left)   outBorderScene.setX(keepout.left());
    if (meta->side == PortSide::Right)  outBorderScene.setX(keepout.right());
    if (meta->side == PortSide::Top)    outBorderScene.setY(keepout.top());
    if (meta->side == PortSide::Bottom) outBorderScene.setY(keepout.bottom());

    const auto& cfg = m_fabric.config();
    const double step = cfg.step;
    if (step <= 0.0)
        return false;

    FabricCoord c = Utils::toFabricCoord(outBorderScene, step);
    int guard = 0;
    while (isFabricPointBlocked(c) && guard++ < 64) {
        stepOutCoord(c, meta->side);
    }

    outFabricScene = Utils::toScenePoint(c, step);
    return true;
}

bool CanvasDocument::arrangeAutoPorts(CanvasBlock& block) const
{
    if (!block.autoPortLayout() || !block.hasPorts())
        return false;

    const auto beforePorts = block.ports();

    std::array<std::vector<PortConn>, 4> groups;
    collectPortGroups(*this, block, groups);

    const double step = m_fabric.config().step;
    if (step <= 0.0)
        return false;

    QRectF bounds;
    const bool boundsChanged = resizeBlockForPorts(*this, block, groups, step, bounds);

    for (auto& list : groups)
        layoutPortsOnSide(block, bounds, step, list);

    const auto& afterPorts = block.ports();
    bool portsChanged = beforePorts.size() != afterPorts.size();
    if (!portsChanged) {
        for (size_t i = 0; i < beforePorts.size(); ++i) {
            const auto& a = beforePorts[i];
            const auto& b = afterPorts[i];
            if (a.id != b.id || a.side != b.side || !qFuzzyCompare(a.t, b.t)) {
                portsChanged = true;
                break;
            }
        }
    }

    return boundsChanged || portsChanged;
}

bool CanvasDocument::computePortTerminalThunk(void* user, ObjectId itemId, PortId portId,
                                             QPointF& outAnchorScene,
                                             QPointF& outBorderScene,
                                             QPointF& outFabricScene)
{
    auto* doc = static_cast<const CanvasDocument*>(user);
    return doc->computePortTerminal(itemId, portId, outAnchorScene, outBorderScene, outFabricScene);
}

bool CanvasDocument::isFabricPointBlockedThunk(const FabricCoord& coord, void* user)
{
	auto* doc = static_cast<const CanvasDocument*>(user);
	return doc ? doc->isFabricPointBlocked(coord) : false;
}

} // namespace Canvas
