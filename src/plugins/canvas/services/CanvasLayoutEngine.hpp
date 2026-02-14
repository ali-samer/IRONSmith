#pragma once

#include "canvas/CanvasGlobal.hpp"

namespace Canvas {
class CanvasBlock;
class CanvasDocument;
}

namespace Canvas::Services {

class CANVAS_EXPORT CanvasLayoutEngine final
{
public:
    bool arrangeAutoPorts(CanvasDocument& doc, CanvasBlock& block) const;
};

} // namespace Canvas::Services
