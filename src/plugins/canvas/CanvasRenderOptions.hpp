#pragma once

#include <QtCore/QMetaType>

namespace Canvas {

struct CanvasRenderOptions
{
    bool showAnnotations = true;
    bool showFabric = true;
    bool showPortHotspots = true;
};

} // namespace Canvas

Q_DECLARE_METATYPE(Canvas::CanvasRenderOptions)
