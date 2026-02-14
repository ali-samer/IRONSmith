#include "canvas/controllers/CanvasSelectionController.hpp"

#include "canvas/CanvasConstants.hpp"
#include "canvas/CanvasDocument.hpp"
#include "canvas/CanvasSelectionModel.hpp"
#include "canvas/CanvasView.hpp"
#include "canvas/CanvasWire.hpp"

#include <QtCore/QLineF>

namespace Canvas::Controllers {

CanvasSelectionController::CanvasSelectionController(CanvasDocument* doc,
                                                     CanvasView* view,
                                                     CanvasSelectionModel* selection)
    : m_doc(doc)
    , m_view(view)
    , m_selection(selection)
{}

const QSet<ObjectId>& CanvasSelectionController::selectedItems() const noexcept
{
    static const QSet<ObjectId> kEmpty;
    return m_selection ? m_selection->selectedItems() : kEmpty;
}

bool CanvasSelectionController::isSelected(ObjectId id) const noexcept
{
    return m_selection ? m_selection->isSelected(id) : false;
}

bool CanvasSelectionController::hasSelectedPort() const noexcept
{
    return m_selection && m_selection->hasSelectedPort();
}

ObjectId CanvasSelectionController::selectedPortItem() const noexcept
{
    return m_selection ? m_selection->selectedPortItem() : ObjectId{};
}

PortId CanvasSelectionController::selectedPortId() const noexcept
{
    return m_selection ? m_selection->selectedPortId() : PortId{};
}

void CanvasSelectionController::selectItem(ObjectId id)
{
    if (!m_selection)
        return;

    if (!id) {
        clearSelection();
        return;
    }

    QSet<ObjectId> next;
    next.insert(id);
    setSelection(next);
}

void CanvasSelectionController::selectPort(const PortRef& port)
{
    if (!m_selection)
        return;
    m_selection->setSelectedPort(port.itemId, port.portId);
    clearSelection();
}

void CanvasSelectionController::clearSelectedPort()
{
    if (!m_selection || !m_selection->hasSelectedPort())
        return;
    m_selection->clearSelectedPort();
}

void CanvasSelectionController::setSelection(const QSet<ObjectId>& ids)
{
    if (!m_selection)
        return;
    if (m_selection->selectedItems() == ids)
        return;
    m_selection->setSelectedItems(ids);
    if (!ids.isEmpty())
        clearSelectedPort();
}

void CanvasSelectionController::clearSelection()
{
    if (!m_selection || m_selection->selectedItems().isEmpty())
        return;
    m_selection->clearSelectedItems();
}

void CanvasSelectionController::addToSelection(ObjectId id)
{
    if (!m_selection || !id)
        return;
    if (m_selection->selectedItems().contains(id))
        return;

    QSet<ObjectId> next = m_selection->selectedItems();
    next.insert(id);
    setSelection(next);
}

void CanvasSelectionController::toggleSelection(ObjectId id)
{
    if (!m_selection || !id)
        return;

    QSet<ObjectId> next = m_selection->selectedItems();
    if (next.contains(id))
        next.remove(id);
    else
        next.insert(id);
    setSelection(next);
}

void CanvasSelectionController::beginMarqueeSelection(const QPointF& scenePos, Qt::KeyboardModifiers mods)
{
    if (!m_view || !m_doc)
        return;

    m_marqueeBasePorts = m_selection ? m_selection->selectedPorts() : QSet<PortRef>{};

    m_marqueeActive = true;
    m_marqueeStartScene = scenePos;
    m_marqueeStartView = m_view->sceneToView(scenePos);
    m_marqueeRectScene = QRectF(scenePos, scenePos);
    m_marqueeMods = mods;
    m_marqueeBaseSelection = selectedItems();

    if (!mods.testFlag(Qt::ShiftModifier) && !mods.testFlag(Qt::ControlModifier))
        m_marqueeBaseSelection.clear();

    if (!mods.testFlag(Qt::ShiftModifier) && !mods.testFlag(Qt::ControlModifier))
        m_marqueeBasePorts.clear();
    m_view->setMarqueeRect(m_marqueeRectScene);
    updateMarqueeSelection(scenePos);
}

void CanvasSelectionController::updateMarqueeSelection(const QPointF& scenePos)
{
    if (!m_marqueeActive || !m_view || !m_doc)
        return;

    m_marqueeRectScene = QRectF(m_marqueeStartScene, scenePos).normalized();
    m_view->setMarqueeRect(m_marqueeRectScene);

    const QSet<ObjectId> hits = collectItemsInRect(m_marqueeRectScene);
    const QSet<PortRef> portHits = collectPortsInRect(m_marqueeRectScene);
    QSet<ObjectId> next = m_marqueeBaseSelection;
    QSet<PortRef> nextPorts = m_marqueeBasePorts;

    if (m_marqueeMods.testFlag(Qt::ControlModifier)) {
        for (const auto& id : hits) {
            if (next.contains(id))
                next.remove(id);
            else
                next.insert(id);
        }
        for (const auto& port : portHits) {
            if (nextPorts.contains(port))
                nextPorts.remove(port);
            else
                nextPorts.insert(port);
        }
    } else if (m_marqueeMods.testFlag(Qt::ShiftModifier)) {
        next.unite(hits);
        nextPorts.unite(portHits);
    } else {
        next = hits;
        nextPorts = portHits;
    }

    setSelection(next);
    if (m_selection)
        m_selection->setSelectedPorts(nextPorts);
}

void CanvasSelectionController::endMarqueeSelection(const QPointF& scenePos)
{
    if (!m_marqueeActive)
        return;

    const QPointF endView = m_view ? m_view->sceneToView(scenePos) : QPointF();
    const double dist = QLineF(m_marqueeStartView, endView).length();
    if (dist < Canvas::Constants::kMarqueeDragThresholdPx) {
        if (m_marqueeMods.testFlag(Qt::ShiftModifier) || m_marqueeMods.testFlag(Qt::ControlModifier)) {
            setSelection(m_marqueeBaseSelection);
            if (m_selection)
                m_selection->setSelectedPorts(m_marqueeBasePorts);
        } else {
            clearSelection();
            clearSelectedPort();
        }
    } else {
        updateMarqueeSelection(scenePos);
    }

    m_marqueeActive = false;
    if (m_view)
        m_view->clearMarqueeRect();
}

void CanvasSelectionController::clearMarqueeSelection()
{
    if (!m_marqueeActive)
        return;
    m_marqueeActive = false;
    m_marqueeRectScene = QRectF();
    if (m_view)
        m_view->clearMarqueeRect();
}

QSet<ObjectId> CanvasSelectionController::collectItemsInRect(const QRectF& sceneRect) const
{
    QSet<ObjectId> ids;
    if (!m_doc)
        return ids;

    const QRectF rect = sceneRect.normalized();
    for (const auto& it : m_doc->items()) {
        if (!it)
            continue;
        if (rect.intersects(it->boundsScene()))
            ids.insert(it->id());
    }
    return ids;
}

QSet<PortRef> CanvasSelectionController::collectPortsInRect(const QRectF& sceneRect) const
{
    QSet<PortRef> ports;
    if (!m_doc)
        return ports;

    const double half = Constants::kPortHitBoxHalfPx;
    const QRectF rect = sceneRect.normalized().adjusted(-half, -half, half, half);
    for (const auto& it : m_doc->items()) {
        if (!it || !it->hasPorts())
            continue;
        for (const auto& port : it->ports()) {
            const QPointF anchor = it->portAnchorScene(port.id);
            if (rect.contains(anchor))
                ports.insert(PortRef{it->id(), port.id});
        }
    }
    return ports;
}

} // namespace Canvas::Controllers
