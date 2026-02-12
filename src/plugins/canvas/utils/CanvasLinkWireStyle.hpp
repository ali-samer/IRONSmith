#pragma once

#include "canvas/CanvasGlobal.hpp"

#include <QtGui/QColor>

namespace Canvas::Support {

enum class LinkWireRole : uint8_t {
    Producer,
    Consumer
};

struct CANVAS_EXPORT LinkWireStyle final {
    QColor color;
};

CANVAS_EXPORT LinkWireStyle linkWireStyle(LinkWireRole role);

} // namespace Canvas::Support
