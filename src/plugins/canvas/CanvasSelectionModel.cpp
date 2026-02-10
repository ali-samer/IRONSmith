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
    if (m_hasSelectedPort && m_selectedPortItem == itemId && m_selectedPortId == portId)
        return;

    m_hasSelectedPort = true;
    m_selectedPortItem = itemId;
    m_selectedPortId = portId;
    emit selectedPortChanged(itemId, portId);
}

void CanvasSelectionModel::clearSelectedPort()
{
    if (!m_hasSelectedPort)
        return;

    m_hasSelectedPort = false;
    m_selectedPortItem = ObjectId{};
    m_selectedPortId = PortId{};
    emit selectedPortCleared();
}

} // namespace Canvas
