#pragma once

#include "canvas/CanvasGlobal.hpp"
#include "canvas/CanvasCommand.hpp"

#include <memory>
#include <vector>

namespace Canvas {

class CanvasCommand;
class CanvasDocument;

class CANVAS_EXPORT CanvasCommandManager final
{
public:
    explicit CanvasCommandManager(CanvasDocument* doc);

    bool execute(std::unique_ptr<CanvasCommand> cmd);

    bool canUndo() const noexcept;
    bool canRedo() const noexcept;

    bool undo();
    bool redo();

    void clear();

private:
    CanvasDocument* m_doc = nullptr;
    std::vector<std::unique_ptr<CanvasCommand>> m_undo;
    std::vector<std::unique_ptr<CanvasCommand>> m_redo;
};

} // namespace Canvas