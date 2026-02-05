#include "canvas/CanvasCommands.hpp"

#include "canvas/CanvasDocument.hpp"
#include "canvas/CanvasItem.hpp"
#include "canvas/CanvasBlock.hpp"
#include "canvas/CanvasWire.hpp"

#include <algorithm>

namespace Canvas {

MoveItemCommand::MoveItemCommand(ObjectId itemId, QPointF fromTopLeftScene, QPointF toTopLeftScene)
    : m_itemId(itemId)
    , m_from(fromTopLeftScene)
    , m_to(toTopLeftScene)
{}

QString MoveItemCommand::name() const
{
    return QStringLiteral("Move Item");
}

bool MoveItemCommand::apply(CanvasDocument& doc)
{
    return doc.setItemTopLeft(m_itemId, m_to);
}

bool MoveItemCommand::revert(CanvasDocument& doc)
{
    return doc.setItemTopLeft(m_itemId, m_from);
}

DeleteItemCommand::DeleteItemCommand(ObjectId itemId)
    : m_itemId(itemId)
{}

QString DeleteItemCommand::name() const
{
    return QStringLiteral("Delete Item");
}

bool DeleteItemCommand::apply(CanvasDocument& doc)
{
    if (!m_itemId)
        return false;

    if (!m_initialized) {
        CanvasItem* item = doc.findItem(m_itemId);
        if (!item)
            return false;

        if (auto* block = dynamic_cast<CanvasBlock*>(item); block && block->isLinkHub()) {
            std::vector<ObjectId> wireIds;
            for (const auto& it : doc.items()) {
                auto* wire = dynamic_cast<CanvasWire*>(it.get());
                if (wire && wire->attachesTo(m_itemId))
                    wireIds.push_back(wire->id());
            }

            for (const auto& id : wireIds) {
                auto removed = doc.removeItem(id);
                if (removed)
                    m_savedItems.push_back(SavedItem{id, removed->index, std::move(removed->item)});
            }
        }

        auto removed = doc.removeItem(m_itemId);
        if (!removed)
            return false;
        m_savedItems.push_back(SavedItem{m_itemId, removed->index, std::move(removed->item)});
        m_initialized = true;
        return true;
    }

    for (const auto& saved : m_savedItems) {
        auto removed = doc.removeItem(saved.id);
        if (!removed)
            return false;
    }
    return true;
}

bool DeleteItemCommand::revert(CanvasDocument& doc)
{
    if (m_savedItems.empty())
        return false;

    std::vector<const SavedItem*> order;
    order.reserve(m_savedItems.size());
    for (const auto& saved : m_savedItems)
        order.push_back(&saved);

    std::sort(order.begin(), order.end(),
              [](const SavedItem* a, const SavedItem* b) { return a->index < b->index; });

    bool ok = true;
    for (const auto* saved : order) {
        auto copy = saved->item->clone();
        ok = doc.insertItem(saved->index, std::move(copy)) && ok;
    }
    return ok;
}

CreateItemCommand::CreateItemCommand(std::unique_ptr<CanvasItem> item)
    : m_item(std::move(item))
{
    if (m_item)
        m_itemId = m_item->id();
}

QString CreateItemCommand::name() const
{
    return QStringLiteral("Create Item");
}

bool CreateItemCommand::apply(CanvasDocument& doc)
{
    if (!m_item)
        return false;

    if (!m_hasIndex) {
        m_index = doc.items().size();
        m_hasIndex = true;
    }

    auto tmp = std::move(m_item);
    if (!doc.insertItem(m_index, std::move(tmp))) {
        m_item = std::move(tmp);
        return false;
    }
    return true;
}

bool CreateItemCommand::revert(CanvasDocument& doc)
{
    auto removed = doc.removeItem(m_itemId);
    if (!removed)
        return false;
    m_index = removed->index;
    m_item = std::move(removed->item);
    return true;
}

} // namespace Canvas
