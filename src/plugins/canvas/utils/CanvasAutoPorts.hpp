#pragma once

#include "canvas/CanvasGlobal.hpp"
#include "canvas/CanvasPorts.hpp"
#include "canvas/CanvasTypes.hpp"

#include <cstddef>
#include <optional>
#include <QtCore/QString>

namespace Canvas {
class CanvasDocument;
struct CanvasPort;
}

namespace Canvas::Support {

struct CANVAS_EXPORT AutoPortRemoval final {
    ObjectId itemId{};
    size_t index = 0;
    CanvasPort port;
};

CANVAS_EXPORT PortSide oppositeSide(PortSide side);
CANVAS_EXPORT QString pairedPortName(const QString& pairKey);
CANVAS_EXPORT bool isPairedPortName(const QString& name);
CANVAS_EXPORT bool isLegacyPairedPortName(const QString& name);
CANVAS_EXPORT std::optional<QString> pairedPortKeyFromName(const QString& name);
CANVAS_EXPORT bool isPairedProducerPort(const CanvasPort& port);
CANVAS_EXPORT std::optional<QString> pairedPortKey(const CanvasPort& port);

CANVAS_EXPORT bool ensureOppositeProducerPort(CanvasDocument& doc,
                                              ObjectId itemId,
                                              PortId portId);

CANVAS_EXPORT std::optional<AutoPortRemoval> removeOppositeProducerPort(CanvasDocument& doc,
                                                                        ObjectId itemId,
                                                                        PortId portId);

} // namespace Canvas::Support
