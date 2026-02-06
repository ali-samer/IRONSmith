#pragma once

#include "canvas/CanvasCommand.hpp"
#include "canvas/CanvasItem.hpp"
#include "canvas/CanvasPorts.hpp"

#include <QtCore/QPointF>

#include <memory>
#include <optional>

namespace Canvas {

class CanvasItem;

struct SavedPort final {
    ObjectId itemId{};
    size_t index = 0;
    CanvasPort port;
};

class CANVAS_EXPORT MoveItemCommand final : public CanvasCommand
{
public:
    MoveItemCommand(ObjectId itemId, QPointF fromTopLeftScene, QPointF toTopLeftScene);

    QString name() const override;
    bool apply(CanvasDocument& doc) override;
    bool revert(CanvasDocument& doc) override;

private:
    ObjectId m_itemId{};
    QPointF  m_from{};
    QPointF  m_to{};
};

class CANVAS_EXPORT DeleteItemCommand final : public CanvasCommand
{
public:
    explicit DeleteItemCommand(ObjectId itemId);

    QString name() const override;
    bool apply(CanvasDocument& doc) override;
    bool revert(CanvasDocument& doc) override;

private:
    struct SavedItem final {
        ObjectId id{};
        size_t index = 0;
        std::unique_ptr<CanvasItem> item;
    };

    ObjectId m_itemId{};
    bool m_initialized = false;
    std::vector<SavedItem> m_savedItems;
    std::vector<SavedPort> m_savedPorts;
};

class CANVAS_EXPORT CreateItemCommand final : public CanvasCommand
{
public:
    explicit CreateItemCommand(std::unique_ptr<CanvasItem> item);

    QString name() const override;
    bool apply(CanvasDocument& doc) override;
    bool revert(CanvasDocument& doc) override;

private:
    std::unique_ptr<CanvasItem> m_item;
    ObjectId m_itemId{};
    size_t m_index = 0;
    bool m_hasIndex = false;
};

class CANVAS_EXPORT DeletePortCommand final : public CanvasCommand
{
public:
    DeletePortCommand(ObjectId itemId, PortId portId);

    QString name() const override;
    bool apply(CanvasDocument& doc) override;
    bool revert(CanvasDocument& doc) override;

private:
    struct SavedWire final {
        ObjectId id{};
        size_t index = 0;
        std::unique_ptr<CanvasItem> item;
    };

    ObjectId m_itemId{};
    PortId m_portId{};
    bool m_initialized = false;
    std::vector<SavedWire> m_savedWires;
    std::optional<CanvasPort> m_savedPort;
    size_t m_portIndex = 0;
    std::vector<SavedPort> m_savedOrphanPorts;
};

} // namespace Canvas
