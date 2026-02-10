#pragma once

#include "canvas/CanvasGlobal.hpp"
#include "canvas/CanvasTypes.hpp"

#include <QtCore/QObject>
#include <QtCore/QSet>

namespace Canvas {

class CANVAS_EXPORT CanvasSelectionModel final : public QObject
{
    Q_OBJECT

public:
    explicit CanvasSelectionModel(QObject* parent = nullptr);

    ObjectId selectedItem() const noexcept;
    const QSet<ObjectId>& selectedItems() const noexcept { return m_selectedItems; }
    bool isSelected(ObjectId id) const noexcept { return m_selectedItems.contains(id); }

    void setSelectedItem(ObjectId id);
    void setSelectedItems(const QSet<ObjectId>& items);
    void clearSelectedItems();

    bool hasSelectedPort() const noexcept { return m_hasSelectedPort; }
    ObjectId selectedPortItem() const noexcept { return m_selectedPortItem; }
    PortId selectedPortId() const noexcept { return m_selectedPortId; }

    void setSelectedPort(ObjectId itemId, PortId portId);
    void clearSelectedPort();

signals:
    void selectedItemsChanged();
    void selectedItemChanged(Canvas::ObjectId id);
    void selectedPortChanged(Canvas::ObjectId itemId, Canvas::PortId portId);
    void selectedPortCleared();

private:
    QSet<ObjectId> m_selectedItems;
    bool m_hasSelectedPort = false;
    ObjectId m_selectedPortItem{};
    PortId m_selectedPortId{};
};

} // namespace Canvas
