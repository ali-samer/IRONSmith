#pragma once

#include "canvas/CanvasGlobal.hpp"

#include <QtCore/QString>
#include <QtGui/QColor>

namespace Canvas::Support {

enum class LinkHubKind : uint8_t {
    Split,
    Join,
    Broadcast
};

struct CANVAS_EXPORT LinkHubStyle final {
    QString symbol;
    QColor fill;
    QColor outline;
    QColor text;
};

LinkHubStyle linkHubStyle(LinkHubKind kind);

} // namespace Canvas::Support
