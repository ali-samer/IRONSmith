#include "canvas/CanvasSelectionModel.hpp"

namespace Canvas {

CanvasSelectionModel::CanvasSelectionModel(QObject* parent)
    : QObject(parent)
{}

ObjectId CanvasSelectionModel::selectedItem() const noexcept
{
    if (m_selectedItems.size() != 1)
        return ObjectId{};
    return *m_selectedItems.constBegin();
}

void CanvasSelectionModel::setSelectedItem(ObjectId id)
{
    if (!id) {
        clearSelectedItems();
        return;
    }

    QSet<ObjectId> next;
    next.insert(id);
    setSelectedItems(next);
}

void CanvasSelectionModel::setSelectedItems(const QSet<ObjectId>& items)
{
    if (m_selectedItems == items)
        return;

    const ObjectId prevSelected = selectedItem();
    m_selectedItems = items;
    emit selectedItemsChanged();

    const ObjectId nextSelected = selectedItem();
    if (prevSelected != nextSelected)
        emit selectedItemChanged(nextSelected);
}

void CanvasSelectionModel::clearSelectedItems()
{
    if (m_selectedItems.isEmpty())
        return;
    m_selectedItems.clear();
    emit selectedItemsChanged();
    emit selectedItemChanged(ObjectId{});
}

void CanvasSelectionModel::setSelectedPort(ObjectId itemId, PortId portId)
{
    QSet<PortRef> next;
    if (itemId && portId)
        next.insert(PortRef{itemId, portId});
    setSelectedPorts(next);
}

bool CanvasSelectionModel::isPortSelected(ObjectId itemId, PortId portId) const noexcept
{
    return m_selectedPorts.contains(PortRef{itemId, portId});
}

void CanvasSelectionModel::setSelectedPorts(const QSet<PortRef>& ports)
{
    if (m_selectedPorts == ports)
        return;

    const bool hadSelected = m_hasSelectedPort;
    const ObjectId prevItem = m_selectedPortItem;
    const PortId prevPort = m_selectedPortId;

    m_selectedPorts = ports;
    if (m_selectedPorts.isEmpty()) {
        m_hasSelectedPort = false;
        m_selectedPortItem = ObjectId{};
        m_selectedPortId = PortId{};
    } else {
        m_hasSelectedPort = true;
        if (!m_selectedPorts.contains(PortRef{m_selectedPortItem, m_selectedPortId})) {
            const auto it = m_selectedPorts.constBegin();
            m_selectedPortItem = it->itemId;
            m_selectedPortId = it->portId;
        }
    }

    emit selectedPortsChanged();
    if (!m_hasSelectedPort) {
        if (hadSelected)
            emit selectedPortCleared();
        return;
    }

    if (!hadSelected || prevItem != m_selectedPortItem || prevPort != m_selectedPortId)
        emit selectedPortChanged(m_selectedPortItem, m_selectedPortId);
}

void CanvasSelectionModel::addSelectedPort(const PortRef& port)
{
    if (!port.itemId || !port.portId)
        return;
    if (m_selectedPorts.contains(port))
        return;
    QSet<PortRef> next = m_selectedPorts;
    next.insert(port);
    setSelectedPorts(next);
}

void CanvasSelectionModel::clearSelectedPort()
{
    if (m_selectedPorts.isEmpty())
        return;
    setSelectedPorts(QSet<PortRef>{});
}

} // namespace Canvas
