#pragma once

#include "canvas/CanvasGlobal.hpp"
#include "canvas/CanvasTypes.hpp"

#include <QtCore/QString>

namespace Canvas {

enum class CANVAS_EXPORT PortRole : uint8_t {
    Producer,
    Consumer,
    Dynamic
};

enum class CANVAS_EXPORT PortSide : uint8_t {
    Left,
    Right,
    Top,
    Bottom
};

struct CANVAS_EXPORT CanvasPort final {
    PortId id{};
    PortRole role{PortRole::Consumer};
    PortSide side{PortSide::Left};

    double t = 0.5;

    QString name{};
};

} // namespace Canvas
