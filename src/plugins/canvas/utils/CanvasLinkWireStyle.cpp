#include "canvas/utils/CanvasLinkWireStyle.hpp"

#include "canvas/CanvasConstants.hpp"

namespace Canvas::Utils {

LinkWireStyle linkWireStyle(LinkWireRole role)
{
    switch (role) {
        case LinkWireRole::Producer:
            return {QColor(Constants::kLinkWireProducerColor)};
        case LinkWireRole::Consumer:
            return {QColor(Constants::kLinkWireConsumerColor)};
    }
    return {QColor(Constants::kWireColor)};
}

} // namespace Canvas::Utils
