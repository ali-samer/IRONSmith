#pragma once

#include "canvas/CanvasGlobal.hpp"

#include <QtGui/QColor>

namespace Canvas::Utils {

enum class LinkWireRole : uint8_t {
    Producer,
    Consumer
};

struct CANVAS_EXPORT LinkWireStyle final {
    QColor color;
};

LinkWireStyle linkWireStyle(LinkWireRole role);

} // namespace Canvas::Utils
