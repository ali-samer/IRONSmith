#include "canvas/CanvasCommandManager.hpp"

#include "canvas/CanvasCommand.hpp"
#include "canvas/CanvasDocument.hpp"

namespace Canvas {

CanvasCommandManager::CanvasCommandManager(CanvasDocument* doc)
    : m_doc(doc)
{}

bool CanvasCommandManager::execute(std::unique_ptr<CanvasCommand> cmd)
{
    if (!m_doc || !cmd)
        return false;

    if (!cmd->apply(*m_doc))
        return false;

    m_undo.emplace_back(std::move(cmd));
    m_redo.clear();
    return true;
}

bool CanvasCommandManager::canUndo() const noexcept { return !m_undo.empty(); }
bool CanvasCommandManager::canRedo() const noexcept { return !m_redo.empty(); }

bool CanvasCommandManager::undo()
{
    if (!m_doc || m_undo.empty())
        return false;

    auto cmd = std::move(m_undo.back());
    m_undo.pop_back();

    if (!cmd->revert(*m_doc)) {
        return false;
    }

    m_redo.emplace_back(std::move(cmd));
    return true;
}

bool CanvasCommandManager::redo()
{
    if (!m_doc || m_redo.empty())
        return false;

    auto cmd = std::move(m_redo.back());
    m_redo.pop_back();

    if (!cmd->apply(*m_doc))
        return false;

    m_undo.emplace_back(std::move(cmd));
    return true;
}

void CanvasCommandManager::clear()
{
    m_undo.clear();
    m_redo.clear();
}

} // namespace Canvas