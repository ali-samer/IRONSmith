#include "canvas/CanvasScene.hpp"

#include "canvas/CanvasConstants.hpp"
#include "canvas/CanvasController.hpp"
#include "canvas/CanvasDocument.hpp"
#include "canvas/CanvasItem.hpp"
#include "canvas/CanvasSelectionModel.hpp"
#include "canvas/CanvasStyle.hpp"
#include "canvas/Tools.hpp"
#include "canvas/utils/CanvasRenderContextBuilder.hpp"

#include <QtGui/QPainter>
#include <QtGui/QPen>
#include <QtGui/QColor>

#include <algorithm>

namespace Canvas {

namespace {

bool isSelectedThunk(void* user, ObjectId id)
{
    const auto* scene = static_cast<const CanvasScene*>(user);
    return scene && scene->isSelected(id);
}

} // namespace

CanvasScene::CanvasScene(QObject* parent)
    : QObject(parent)
{
}

void CanvasScene::setDocument(CanvasDocument* doc)
{
    if (m_document == doc)
        return;

    if (m_document)
        disconnect(m_document, nullptr, this, nullptr);

    m_document = doc;
    if (m_document) {
        connect(m_document, &CanvasDocument::changed, this,
                [this]() { emit requestUpdate(); });
    }
    emit requestUpdate();
}

void CanvasScene::setController(CanvasController* controller)
{
    if (m_controller == controller)
        return;
    m_controller = controller;
    emit requestUpdate();
}

void CanvasScene::setSelectionModel(CanvasSelectionModel* model)
{
    if (m_selectionModel == model)
        return;

    if (m_selectionModel)
        disconnect(m_selectionModel, nullptr, this, nullptr);

    m_selectionModel = model;
    if (m_selectionModel) {
        connect(m_selectionModel, &CanvasSelectionModel::selectedItemsChanged, this,
                [this]() {
                    emit requestUpdate();
                    emit selectedItemsChanged();
                });
        connect(m_selectionModel, &CanvasSelectionModel::selectedItemChanged, this,
                [this](ObjectId id) {
                    emit requestUpdate();
                    emit selectedItemChanged(id);
                });
        connect(m_selectionModel, &CanvasSelectionModel::selectedPortChanged, this,
                [this](ObjectId, PortId) { emit requestUpdate(); });
        connect(m_selectionModel, &CanvasSelectionModel::selectedPortCleared, this,
                [this]() { emit requestUpdate(); });
    }
    emit requestUpdate();
}

ObjectId CanvasScene::selectedItem() const noexcept
{
    return m_selectionModel ? m_selectionModel->selectedItem() : ObjectId{};
}

const QSet<ObjectId>& CanvasScene::selectedItems() const noexcept
{
    static const QSet<ObjectId> kEmpty;
    return m_selectionModel ? m_selectionModel->selectedItems() : kEmpty;
}

bool CanvasScene::isSelected(ObjectId id) const noexcept
{
    return m_selectionModel ? m_selectionModel->isSelected(id) : false;
}

void CanvasScene::setSelectedItem(ObjectId id)
{
    if (!m_selectionModel)
        return;
    m_selectionModel->setSelectedItem(id);
}

void CanvasScene::setSelectedItems(const QSet<ObjectId>& items)
{
    if (!m_selectionModel)
        return;
    m_selectionModel->setSelectedItems(items);
}

void CanvasScene::clearSelectedItems()
{
    if (!m_selectionModel)
        return;
    m_selectionModel->clearSelectedItems();
}

void CanvasScene::setSelectedPort(ObjectId itemId, PortId portId)
{
    if (!m_selectionModel)
        return;
    m_selectionModel->setSelectedPort(itemId, portId);
}

void CanvasScene::clearSelectedPort()
{
    if (!m_selectionModel)
        return;
    m_selectionModel->clearSelectedPort();
}

void CanvasScene::setHoveredPort(ObjectId itemId, PortId portId)
{
    if (m_hasHoveredPort && m_hoveredItem == itemId && m_hoveredPort == portId)
        return;
    m_hasHoveredPort = true;
    m_hoveredItem = itemId;
    m_hoveredPort = portId;
    emit requestUpdate();
    emit hoveredPortChanged(m_hoveredItem, m_hoveredPort);
}

void CanvasScene::clearHoveredPort()
{
    if (!m_hasHoveredPort)
        return;
    m_hasHoveredPort = false;
    m_hoveredItem = ObjectId{};
    m_hoveredPort = PortId{};
    emit requestUpdate();
    emit hoveredPortCleared();
}

void CanvasScene::setHoveredEdge(ObjectId itemId, PortSide side, const QPointF& anchorScene)
{
    if (m_hasHoveredEdge && m_hoveredEdgeItem == itemId && m_hoveredEdgeSide == side
        && m_hoveredEdgeAnchor == anchorScene)
        return;
    m_hasHoveredEdge = true;
    m_hoveredEdgeItem = itemId;
    m_hoveredEdgeSide = side;
    m_hoveredEdgeAnchor = anchorScene;
    emit requestUpdate();
}

void CanvasScene::clearHoveredEdge()
{
    if (!m_hasHoveredEdge)
        return;
    m_hasHoveredEdge = false;
    m_hoveredEdgeItem = ObjectId{};
    m_hoveredEdgeSide = PortSide::Left;
    m_hoveredEdgeAnchor = QPointF();
    emit requestUpdate();
}

void CanvasScene::setMarqueeRect(const QRectF& sceneRect)
{
    const QRectF normalized = sceneRect.normalized();
    if (m_hasMarquee && m_marqueeSceneRect == normalized)
        return;
    m_hasMarquee = true;
    m_marqueeSceneRect = normalized;
    emit requestUpdate();
}

void CanvasScene::clearMarqueeRect()
{
    if (!m_hasMarquee)
        return;
    m_hasMarquee = false;
    m_marqueeSceneRect = QRectF();
    emit requestUpdate();
}

void CanvasScene::paint(QPainter& p, const ViewState& view) const
{
    drawBackgroundLayer(p);

    p.save();
    applyViewTransform(p, view);
    const QRectF visible = sceneRect(view);
    drawGridFabric(p, visible);
    drawContentLayer(p, visible, view.zoom);
    drawOverlayLayer(p, visible, view.zoom);
    p.restore();
}

QRectF CanvasScene::sceneRect(const ViewState& view) const
{
    const QPointF tl = Tools::viewToScene(QPointF(0.0, 0.0), view.pan, view.zoom);
    const QPointF br = Tools::viewToScene(QPointF(view.size.width(), view.size.height()), view.pan, view.zoom);
    const double left = std::min(tl.x(), br.x());
    const double right = std::max(tl.x(), br.x());
    const double top = std::min(tl.y(), br.y());
    const double bottom = std::max(tl.y(), br.y());
    return QRectF(QPointF(left, top), QPointF(right, bottom));
}

void CanvasScene::drawBackgroundLayer(QPainter& p) const
{
    p.fillRect(p.viewport(), QColor(Constants::CANVAS_BACKGROUND_COLOR));
}

void CanvasScene::applyViewTransform(QPainter& p, const ViewState& view) const
{
    p.scale(view.zoom, view.zoom);
    p.translate(view.pan.x(), view.pan.y());
}

void CanvasScene::drawGridFabric(QPainter& p, const QRectF& visibleScene) const
{
    if (!m_document)
        return;

    m_document->fabric().draw(p, visibleScene, &CanvasDocument::isFabricPointBlockedThunk, m_document);
}

void CanvasScene::drawContentLayer(QPainter& p, const QRectF& visibleScene, double zoom) const
{
    if (!m_document)
        return;

    CanvasRenderContext ctx = buildRenderContext(visibleScene, true, zoom);
    for (const auto& item : m_document->items()) {
        if (item)
            item->draw(p, ctx);
    }
}

void CanvasScene::drawOverlayLayer(QPainter& p, const QRectF& visibleScene, double zoom) const
{
    if (!m_document || !m_controller)
        return;

    if (m_hasHoveredEdge && (m_controller->mode() == CanvasController::Mode::Linking
                             || m_controller->isEndpointDragActive())) {
        p.save();
        CanvasStyle::drawPort(p, m_hoveredEdgeAnchor, m_hoveredEdgeSide, PortRole::Dynamic, zoom, true);
        p.restore();
    }

    if (m_hasMarquee) {
        QColor stroke(Constants::kBlockSelectionColor);
        stroke.setAlphaF(0.8);
        QColor fill(Constants::kBlockSelectionColor);
        fill.setAlphaF(0.15);

        QPen pen(stroke);
        pen.setWidthF(1.0 / std::clamp(zoom, 0.25, 8.0));
        pen.setStyle(Qt::DashLine);
        p.setPen(pen);
        p.setBrush(fill);
        p.drawRect(m_marqueeSceneRect);
    }

    if (m_controller->mode() != CanvasController::Mode::Linking || !m_controller->isLinkingInProgress())
        return;

    CanvasRenderContext ctx = buildRenderContext(visibleScene, false, zoom);

    QPointF aAnchor, aBorder, aFabric;
    QPointF start = m_controller->linkPreviewScene();
    if (ctx.portTerminal(m_controller->linkStartItem(), m_controller->linkStartPort(),
                         aAnchor, aBorder, aFabric)) {
        start = aAnchor;
    }

    const QPointF end = m_controller->linkPreviewScene();

    p.save();
    QPen pen{QColor(Constants::kWireColor)};
    pen.setWidthF(1.5 / std::clamp(zoom, 0.25, 8.0));
    pen.setStyle(Qt::DashLine);
    pen.setCapStyle(Qt::RoundCap);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    p.setOpacity(0.55);
    p.drawLine(start, end);
    p.restore();
}

CanvasRenderContext CanvasScene::buildRenderContext(const QRectF& sceneRect, bool includeHover, double zoom) const
{
    Support::RenderContextSelection selection;
    selection.isSelected = &isSelectedThunk;
    selection.user = const_cast<CanvasScene*>(this);

    Support::RenderContextPortState ports;
    if (includeHover) {
        ports.hasHoveredPort = m_hasHoveredPort;
        ports.hoveredPortItem = m_hoveredItem;
        ports.hoveredPortId = m_hoveredPort;
    }
    if (m_selectionModel && m_selectionModel->hasSelectedPort()) {
        ports.hasSelectedPort = true;
        ports.selectedPortItem = m_selectionModel->selectedPortItem();
        ports.selectedPortId = m_selectionModel->selectedPortId();
    }

    return Support::buildRenderContext(m_document, sceneRect, zoom, selection, ports);
}

} // namespace Canvas
