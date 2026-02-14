#pragma once

#include "canvas/CanvasGlobal.hpp"
#include "canvas/CanvasTypes.hpp"
#include "canvas/CanvasWire.hpp"

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
    const QSet<PortRef>& selectedPorts() const noexcept { return m_selectedPorts; }
    bool isPortSelected(ObjectId itemId, PortId portId) const noexcept;

    void setSelectedPort(ObjectId itemId, PortId portId);
    void setSelectedPorts(const QSet<PortRef>& ports);
    void addSelectedPort(const PortRef& port);
    void clearSelectedPort();

signals:
    void selectedItemsChanged();
    void selectedItemChanged(Canvas::ObjectId id);
    void selectedPortChanged(Canvas::ObjectId itemId, Canvas::PortId portId);
    void selectedPortCleared();
    void selectedPortsChanged();

private:
    QSet<ObjectId> m_selectedItems;
    bool m_hasSelectedPort = false;
    ObjectId m_selectedPortItem{};
    PortId m_selectedPortId{};
    QSet<PortRef> m_selectedPorts;
};

} // namespace Canvas
